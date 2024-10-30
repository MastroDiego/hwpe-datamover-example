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

__attribute__ ((section(".heapsram"))) uint32_t src[DATA_WIDTH/32 * 64];
__attribute__ ((section(".heapsram"))) uint32_t dst[DATA_WIDTH/32 * 64];

int main() {

  volatile int errors = 0;
  
  int offload_id;

  // Generate pseudo-random data
  generate_random_buffer((unsigned int) src,(unsigned int) &src[DATA_WIDTH/32*64-1], DEFAULT_SEED);

  // acquire job 
  while((offload_id = datamover_acquire_job()) < 0);
  
  // job-dependent registers
  datamover_src_addr_set((unsigned int) src);
  datamover_dst_addr_set((unsigned int) dst);
  datamover_tot_len_set(64);
  datamover_src_d0_len_set(4);
  datamover_src_d0_stride_set(DATA_WIDTH/8);
  datamover_src_d1_len_set(8);
  datamover_src_d1_stride_set(DATA_WIDTH/8 * 8);
  datamover_src_d2_stride_set(DATA_WIDTH/8 * 4);
  datamover_dst_d0_len_set(4);
  datamover_dst_d0_stride_set(DATA_WIDTH/8);
  datamover_dst_d1_len_set(8);
  datamover_dst_d1_stride_set(DATA_WIDTH/8 * 8);
  datamover_dst_d2_stride_set(DATA_WIDTH/8 * 4);

  // start hwpe operation
  datamover_trigger_job();

  // wait for end of computation
  asm volatile ("wfi" ::: "memory");

  errors = datamover_compare_int(dst, src, DATA_WIDTH/32 * 64);

  // return errors
  *(int *) 0x80000000 = errors;
  return errors;
}
