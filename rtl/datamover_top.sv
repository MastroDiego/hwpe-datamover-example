/*
 * Copyright (C) 2020 ETH Zurich and University of Bologna
 *
 * Copyright and related rights are licensed under the Solderpad Hardware
 * License, Version 0.51 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
 * or agreed to in writing, software, hardware and materials distributed under
 * this License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 */

/*
 * Authors:  Francesco Conti <f.conti@unibo.it>
 */

import hwpe_ctrl_package::*;
import hci_package::*;
import datamover_package::*;

`include "hci_helpers.svh"

module datamover_top #(
  parameter int unsigned ID        = 10,
  parameter int unsigned N_CORES   = 8,
  parameter int unsigned N_CONTEXT = 2,
  parameter hci_size_parameter_t `HCI_SIZE_PARAM(tcdm) = '0
) (
  // global signals
  input  logic                    clk_i,
  input  logic                    rst_ni,
  input  logic                    test_mode_i,
  // events
  output logic [N_CORES-1:0][1:0] evt_o,
  // tcdm master ports
  hci_core_intf.initiator         tcdm,
  // periph slave port
  hwpe_ctrl_intf_periph.slave     periph
);

  localparam int unsigned DW  = `HCI_SIZE_GET_DW(tcdm);
  localparam int unsigned AW  = `HCI_SIZE_GET_AW(tcdm);
  localparam int unsigned BW  = `HCI_SIZE_GET_BW(tcdm);
  localparam int unsigned UW  = `HCI_SIZE_GET_UW(tcdm);
  localparam int unsigned IW  = `HCI_SIZE_GET_IW(tcdm);
  localparam int unsigned EW  = `HCI_SIZE_GET_EW(tcdm);
  localparam int unsigned EHW = `HCI_SIZE_GET_EHW(tcdm);

  // We "sacrifice" 1 word of memory interface bandwidth in order to support
  // realignment at a byte boundary.
  localparam DW_ALIGNED = DW-32;
  
  // State for the FSM declared directly in datamover_top.
  typedef enum { DM_IDLE, DM_STARTING, DM_STARTING2, DM_STARTING3, DM_WORKING, DM_WORKING2, DM_WORKING3, DM_FINISHED, DM_WAIT, DM_WAIT2} dm_state;
  dm_state cs, ns;

  // Software-generated clear signal.
  logic clear;

  // These are the bit fields used to control the streamer.
  ctrl_streamer_v2_t  streamer_ctrl, streamer_ctrl_cfg, streamer_ctrl_cfg2, streamer_ctrl_cfg3;
  flags_streamer_v2_t streamer_flags;

  // These are the bit fields used to propagate flags from/to the peripheral
  // interconnect slave interface.
  ctrl_slave_t slave_ctrl;
  flags_slave_t slave_flags;
  ctrl_regfile_t reg_file;

  // Data in and data out internal HWPE-Streams. Notice that the data width
  // is set to 256 bits by default, 32 bits less than the default external
  // bandwidth. The additional 32 bits of memory bandwidth are used to 
  // support access to non-word-aligned data packets.
  hwpe_stream_intf_stream #(
    .DATA_WIDTH(DW_ALIGNED)
  ) data_in  (
    .clk(clk_i)
  );
  hwpe_stream_intf_stream #(
    .DATA_WIDTH(DW_ALIGNED)
  ) data_out (
    .clk(clk_i)
  );
// The streamer exposes on the memory side a single TCDM 288-bit interface
  // meant to be directly plugged into an Heterogeneous Cluster Interconnect.
  // On the accelerator side, it exposes an outgoing data in stream and
  // an incoming data out HWPE-Streams, each 256-bit wide.
  datamover_streamer #(
    .`HCI_SIZE_PARAM(tcdm) (`HCI_SIZE_PARAM(tcdm))
  ) i_streamer (
    .clk_i      ( clk_i          ),
    .rst_ni     ( rst_ni         ),
    .test_mode_i( test_mode_i    ),
    .enable_i   ( 1'b1           ),
    .clear_i    ( clear          ),
    .data_in    ( data_in        ),
    .data_out   ( data_out       ),
    .tcdm       ( tcdm           ),
    .ctrl_i     ( streamer_ctrl  ),
    .flags_o    ( streamer_flags )
  );

  // The "engine", i.e., the datapath of the HWPE, is as simple as it gets:
  // a FIFO copying the data in stream into the data out one!
  datamover_engine #(
    .FIFO_DEPTH ( 4          ),
    .BW_ALIGNED ( DW_ALIGNED )
  ) i_engine (
    .clk_i      ( clk_i          ),
    .rst_ni     ( rst_ni         ),
    .test_mode_i( test_mode_i    ),
    .enable_i   ( 1'b1           ),
    .clear_i    ( clear          ),
    .data_in    ( data_in        ),
    .data_out   ( data_out       )
  );
  
  // The slave module exposes a peripheral interconnect HWPE-Periph plug;
  // in the default configuration, it provides 2 contexts with 13 registers
  // each, which are exposed into `reg_file.hwpe_params`
  hwpe_ctrl_slave #(
    .N_CORES        ( N_CORES   ),
    .N_CONTEXT      ( N_CONTEXT ),
    .N_IO_REGS      ( 52 ),
    .N_GENERIC_REGS ( 0  ),
    .ID_WIDTH       ( ID )
  ) i_slave (
    .clk_i   ( clk_i       ),
    .rst_ni  ( rst_ni      ),
    .clear_o ( clear       ),
    .cfg     ( periph      ),
    .ctrl_i  ( slave_ctrl  ),
    .flags_o ( slave_flags ),
    .reg_file( reg_file    )
  );

  // Datamover FSM: sequential process.
  logic [1:0] delay_counter;

  always_ff @(posedge clk_i or negedge rst_ni)
begin : fsm_seq
  if (~rst_ni) begin
    cs <= DM_IDLE;
    delay_counter <= 2'b0;
  end else if (clear) begin
    cs <= DM_IDLE;
    delay_counter <= 2'b0;
  end else begin
    cs <= ns;
    if (cs == DM_WAIT || cs == DM_WAIT2 ) 
      delay_counter <= delay_counter + 1;
    else 
      delay_counter <= 2'b0;
  end
end
   

   
  // Datamover FSM: combinational next-state calculation process.
  always_comb
  begin : fsm_ns_comb
    ns = cs;
    case (cs)
    DM_IDLE: begin
      if (slave_flags.start)
        ns = DM_STARTING;
    end
    DM_STARTING: begin
      ns = DM_WAIT;
    end
    DM_WAIT: begin
      if (delay_counter == 2'b01) // 2 cicli completati
        ns = DM_STARTING2;
    end
    DM_STARTING2: begin
      ns = DM_WORKING;
    end
    
    DM_WORKING: begin
      if ((streamer_flags.data_out_sink_flags.done) & streamer_flags.tcdm_fifo_empty)
        ns = DM_WORKING2;
    end
    DM_WORKING2: begin
      if ((streamer_flags.data_out_sink_flags.done) & streamer_flags.tcdm_fifo_empty & streamer_flags.data_in_source_flags.ready & streamer_flags.data_out_sink_flags.ready)
        ns = DM_WAIT2;
    end

    DM_WAIT2: begin
      if (delay_counter == 2'b01) // 2 cicli completati
        ns = DM_STARTING3;
    end

    DM_STARTING3: begin
      ns = DM_WORKING3;
    end

    DM_WORKING3: begin
      if ((streamer_flags.data_out_sink_flags.done) & streamer_flags.tcdm_fifo_empty)
        ns = DM_FINISHED;
    end
    default: begin
      ns = DM_IDLE;
    end
  endcase
  end

  // Datamover FSM: combinational output calculation process.
  always_comb
  begin : fsm_out_comb
    slave_ctrl = '0;
    streamer_ctrl.data_in_source_ctrl.valid = 1'b0;
    streamer_ctrl.data_out_sink_ctrl.valid  = 1'b0;
    if(cs == DM_STARTING) begin
      streamer_ctrl = streamer_ctrl_cfg;
      streamer_ctrl.data_in_source_ctrl.valid = 1'b1;
      streamer_ctrl.data_out_sink_ctrl.valid  = 1'b1;
    end
    if(cs == DM_STARTING2) begin
      streamer_ctrl = streamer_ctrl_cfg2;
      streamer_ctrl.data_in_source_ctrl.valid = 1'b1;
      streamer_ctrl.data_out_sink_ctrl.valid  = 1'b1;
    end
    if(cs == DM_STARTING3) begin
      streamer_ctrl = streamer_ctrl_cfg3;
      streamer_ctrl.data_in_source_ctrl.valid = 1'b1;
      streamer_ctrl.data_out_sink_ctrl.valid  = 1'b1;
    end
    else if (cs == DM_FINISHED) begin
      slave_ctrl.done = 1'b1;
    end
  end

  // Here we bind the register file parameters to the streamer configuration.
  // `streamer_ctrl_cfg` contains the "base" configuration, with null `req_start`.
  // The FSM copies this base configuration into `streamer_ctrl` and sets
  // the `req_start` signals when in state DM_STARTING.
  always_comb
  begin
    streamer_ctrl_cfg = '0;
    streamer_ctrl_cfg.data_in_source_ctrl.addressgen_ctrl.dim_enable_1h = '1;
    streamer_ctrl_cfg.data_out_sink_ctrl.addressgen_ctrl.dim_enable_1h  = '1;
    streamer_ctrl_cfg.data_in_source_ctrl.addressgen_ctrl.base_addr = reg_file.hwpe_params[0];
    streamer_ctrl_cfg.data_out_sink_ctrl.addressgen_ctrl.base_addr  = reg_file.hwpe_params[1];
    streamer_ctrl_cfg.data_in_source_ctrl.addressgen_ctrl.tot_len   = reg_file.hwpe_params[2];
    streamer_ctrl_cfg.data_out_sink_ctrl.addressgen_ctrl.tot_len    = reg_file.hwpe_params[2];
    streamer_ctrl_cfg.data_in_source_ctrl.addressgen_ctrl.d0_len    = reg_file.hwpe_params[3];
    streamer_ctrl_cfg.data_in_source_ctrl.addressgen_ctrl.d0_stride = reg_file.hwpe_params[4];
    streamer_ctrl_cfg.data_in_source_ctrl.addressgen_ctrl.d1_len    = reg_file.hwpe_params[5];
    streamer_ctrl_cfg.data_in_source_ctrl.addressgen_ctrl.d1_stride = reg_file.hwpe_params[6];
    streamer_ctrl_cfg.data_in_source_ctrl.addressgen_ctrl.d2_stride = reg_file.hwpe_params[7];
    streamer_ctrl_cfg.data_out_sink_ctrl.addressgen_ctrl.d0_len     = reg_file.hwpe_params[8];
    streamer_ctrl_cfg.data_out_sink_ctrl.addressgen_ctrl.d0_stride  = reg_file.hwpe_params[9];
    streamer_ctrl_cfg.data_out_sink_ctrl.addressgen_ctrl.d1_len     = reg_file.hwpe_params[10];
    streamer_ctrl_cfg.data_out_sink_ctrl.addressgen_ctrl.d1_stride  = reg_file.hwpe_params[11];
    streamer_ctrl_cfg.data_out_sink_ctrl.addressgen_ctrl.d2_stride  = reg_file.hwpe_params[12];

    streamer_ctrl_cfg2 = '0;
    streamer_ctrl_cfg2.data_in_source_ctrl.addressgen_ctrl.dim_enable_1h = '1;
    streamer_ctrl_cfg2.data_out_sink_ctrl.addressgen_ctrl.dim_enable_1h  = '1;
    streamer_ctrl_cfg2.data_in_source_ctrl.addressgen_ctrl.base_addr = reg_file.hwpe_params[13];
    streamer_ctrl_cfg2.data_out_sink_ctrl.addressgen_ctrl.base_addr  = reg_file.hwpe_params[14];
    streamer_ctrl_cfg2.data_in_source_ctrl.addressgen_ctrl.tot_len   = reg_file.hwpe_params[15];
    streamer_ctrl_cfg2.data_out_sink_ctrl.addressgen_ctrl.tot_len    = reg_file.hwpe_params[15];
    streamer_ctrl_cfg2.data_in_source_ctrl.addressgen_ctrl.d0_len    = reg_file.hwpe_params[16];
    streamer_ctrl_cfg2.data_in_source_ctrl.addressgen_ctrl.d0_stride = reg_file.hwpe_params[17];
    streamer_ctrl_cfg2.data_in_source_ctrl.addressgen_ctrl.d1_len    = reg_file.hwpe_params[18];
    streamer_ctrl_cfg2.data_in_source_ctrl.addressgen_ctrl.d1_stride = reg_file.hwpe_params[19];
    streamer_ctrl_cfg2.data_in_source_ctrl.addressgen_ctrl.d2_stride = reg_file.hwpe_params[20];
    streamer_ctrl_cfg2.data_out_sink_ctrl.addressgen_ctrl.d0_len     = reg_file.hwpe_params[21];
    streamer_ctrl_cfg2.data_out_sink_ctrl.addressgen_ctrl.d0_stride  = reg_file.hwpe_params[22];
    streamer_ctrl_cfg2.data_out_sink_ctrl.addressgen_ctrl.d1_len     = reg_file.hwpe_params[23];
    streamer_ctrl_cfg2.data_out_sink_ctrl.addressgen_ctrl.d1_stride  = reg_file.hwpe_params[24];
    streamer_ctrl_cfg2.data_out_sink_ctrl.addressgen_ctrl.d2_stride  = reg_file.hwpe_params[25];

    streamer_ctrl_cfg3 = '0;
    streamer_ctrl_cfg3.data_in_source_ctrl.addressgen_ctrl.dim_enable_1h = '1;
    streamer_ctrl_cfg3.data_out_sink_ctrl.addressgen_ctrl.dim_enable_1h  = '1;
    streamer_ctrl_cfg3.data_in_source_ctrl.addressgen_ctrl.base_addr = reg_file.hwpe_params[26];
    streamer_ctrl_cfg3.data_out_sink_ctrl.addressgen_ctrl.base_addr  = reg_file.hwpe_params[27];
    streamer_ctrl_cfg3.data_in_source_ctrl.addressgen_ctrl.tot_len   = reg_file.hwpe_params[28];
    streamer_ctrl_cfg3.data_out_sink_ctrl.addressgen_ctrl.tot_len    = reg_file.hwpe_params[28];
    streamer_ctrl_cfg3.data_in_source_ctrl.addressgen_ctrl.d0_len    = reg_file.hwpe_params[29];
    streamer_ctrl_cfg3.data_in_source_ctrl.addressgen_ctrl.d0_stride = reg_file.hwpe_params[30];
    streamer_ctrl_cfg3.data_in_source_ctrl.addressgen_ctrl.d1_len    = reg_file.hwpe_params[31];
    streamer_ctrl_cfg3.data_in_source_ctrl.addressgen_ctrl.d1_stride = reg_file.hwpe_params[32];
    streamer_ctrl_cfg3.data_in_source_ctrl.addressgen_ctrl.d2_stride = reg_file.hwpe_params[33];
    streamer_ctrl_cfg3.data_out_sink_ctrl.addressgen_ctrl.d0_len     = reg_file.hwpe_params[34];
    streamer_ctrl_cfg3.data_out_sink_ctrl.addressgen_ctrl.d0_stride  = reg_file.hwpe_params[35];
    streamer_ctrl_cfg3.data_out_sink_ctrl.addressgen_ctrl.d1_len     = reg_file.hwpe_params[36];
    streamer_ctrl_cfg3.data_out_sink_ctrl.addressgen_ctrl.d1_stride  = reg_file.hwpe_params[37];
    streamer_ctrl_cfg3.data_out_sink_ctrl.addressgen_ctrl.d2_stride  = reg_file.hwpe_params[38];

  end

  // Bind the output event, which is propagated to the event unit and used
  // to implement HWPE datamover barriers.
  assign evt_o = slave_flags.evt[N_CORES-1:0];

endmodule // datamover_top
