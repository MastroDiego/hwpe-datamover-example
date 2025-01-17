/*
 * Copyright (C) 2019 ETH Zurich and University of Bologna
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* 
 * Authors:  Francesco Conti <fconti@iis.ee.ethz.ch>
 */

#include <stdint.h>
#include "hal_datamover.h"
#include "tinyprintf.h"
#include "lfsr32.h"

__attribute__ ((section(".heapsram"))) uint32_t src1[DATA_WIDTH/32 * 64];
__attribute__ ((section(".heapsram"))) uint32_t src2[DATA_WIDTH/32 * 64];
__attribute__ ((section(".heapsram"))) uint32_t src3[DATA_WIDTH/32 * 64];
__attribute__ ((section(".heapsram"))) uint32_t dst1[DATA_WIDTH/32 * 64];
__attribute__ ((section(".heapsram"))) uint32_t dst2[DATA_WIDTH/32 * 64];
__attribute__ ((section(".heapsram"))) uint32_t dst3[DATA_WIDTH/32 * 64];

int main() {

  volatile int errors = 0;
  
  int offload_id;

  // Generate pseudo-random data for four different buffers
  generate_random_buffer((unsigned int) src1, (unsigned int) &src1[DATA_WIDTH/32 * 64 - 1], DEFAULT_SEED);
  generate_random_buffer((unsigned int) src2, (unsigned int) &src2[DATA_WIDTH/32 * 64 - 1], DEFAULT_SEED + 1);
  generate_random_buffer((unsigned int) src3, (unsigned int) &src3[DATA_WIDTH/32 * 64 - 1], DEFAULT_SEED + 2);

  // acquire job 
  while ((offload_id = datamover_acquire_job()) < 0);
  
  // First job operates on the first buffer
  datamover_src_addr_set((unsigned int) src1);
  datamover_dst_addr_set((unsigned int) dst1);
  datamover_tot_len_set(64);
  datamover_src_d0_len_set(4);
  datamover_src_d0_stride_set(DATA_WIDTH / 8);
  datamover_src_d1_len_set(8);
  datamover_src_d1_stride_set(DATA_WIDTH / 8 * 8);
  datamover_src_d2_stride_set(DATA_WIDTH / 8 * 4);
  datamover_dst_d0_len_set(4);
  datamover_dst_d0_stride_set(DATA_WIDTH / 8);
  datamover_dst_d1_len_set(8);
  datamover_dst_d1_stride_set(DATA_WIDTH / 8 * 8);
  datamover_dst_d2_stride_set(DATA_WIDTH / 8 * 4);

  // Second job operates on the second buffer
  datamover_src_addr2_set((unsigned int) src2);
  datamover_dst_addr2_set((unsigned int) dst2);
  datamover_tot_len2_set(64);
  datamover_src_d0_len2_set(4);
  datamover_src_d0_stride2_set(DATA_WIDTH / 8);
  datamover_src_d1_len2_set(8);
  datamover_src_d1_stride2_set(DATA_WIDTH / 8 * 8);
  datamover_src_d2_stride2_set(DATA_WIDTH / 8 * 4);
  datamover_dst_d0_len2_set(4);
  datamover_dst_d0_stride2_set(DATA_WIDTH / 8);
  datamover_dst_d1_len2_set(8);
  datamover_dst_d1_stride2_set(DATA_WIDTH / 8 * 8);
  datamover_dst_d2_stride2_set(DATA_WIDTH / 8 * 4);

  // Third job operates on the third buffer
  datamover_src_addr3_set((unsigned int) src3);
  datamover_dst_addr3_set((unsigned int) dst3);
  datamover_tot_len3_set(64);
  datamover_src_d0_len3_set(4);
  datamover_src_d0_stride3_set(DATA_WIDTH / 8);
  datamover_src_d1_len3_set(8);
  datamover_src_d1_stride3_set(DATA_WIDTH / 8 * 8);
  datamover_src_d2_stride3_set(DATA_WIDTH / 8 * 4);
  datamover_dst_d0_len3_set(4);
  datamover_dst_d0_stride3_set(DATA_WIDTH / 8);
  datamover_dst_d1_len3_set(8);
  datamover_dst_d1_stride3_set(DATA_WIDTH / 8 * 8);
  datamover_dst_d2_stride3_set(DATA_WIDTH / 8 * 4);



  // start hwpe operation
  datamover_trigger_job();

  // wait for end of computation
  asm volatile ("wfi" ::: "memory");

  errors += datamover_compare_int(dst1, src1, DATA_WIDTH / 32 * 64);
  errors += datamover_compare_int(dst2, src2, DATA_WIDTH / 32 * 64);
  errors += datamover_compare_int(dst3, src3, DATA_WIDTH / 32 * 64);

  // return errors
  *(int *) 0x80000000 = errors;
  return errors;
}
