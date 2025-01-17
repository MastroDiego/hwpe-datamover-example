/*
 * Copyright (C) 2018-2019 ETH Zurich and University of Bologna
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

#ifndef __HAL_DATAMOVER_H__
#define __HAL_DATAMOVER_H__

/* REGISTER MAP */
#define ARCHI_CL_EVT_ACC0 0
#define ARCHI_CL_EVT_ACC1 1
#define __builtin_bitinsert(a,b,c,d) (a | (((b << (32-c)) >> (32-c)) << d))

#define DATA_WIDTH  256

#define DATAMOVER_TRIGGER          0x00
#define DATAMOVER_ACQUIRE          0x04
#define DATAMOVER_FINISHED         0x08
#define DATAMOVER_STATUS           0x0c
#define DATAMOVER_RUNNING_JOB      0x10
#define DATAMOVER_SOFT_CLEAR       0x14

#define DATAMOVER_REG_SRC_ADDR      0x20
#define DATAMOVER_REG_DST_ADDR      0x24
#define DATAMOVER_REG_TOT_LEN       0x28
#define DATAMOVER_REG_SRC_D0_LEN    0x2C
#define DATAMOVER_REG_SRC_D0_STRIDE 0x30
#define DATAMOVER_REG_SRC_D1_LEN    0x34
#define DATAMOVER_REG_SRC_D1_STRIDE 0x38
#define DATAMOVER_REG_SRC_D2_STRIDE 0x3C
#define DATAMOVER_REG_DST_D0_LEN    0x40
#define DATAMOVER_REG_DST_D0_STRIDE 0x44
#define DATAMOVER_REG_DST_D1_LEN    0x48
#define DATAMOVER_REG_DST_D1_STRIDE 0x4C
#define DATAMOVER_REG_DST_D2_STRIDE 0x50

/* Duplicated Registers */
#define DATAMOVER_REG_SRC_ADDR2      0x54
#define DATAMOVER_REG_DST_ADDR2      0x58
#define DATAMOVER_REG_TOT_LEN2       0x5C
#define DATAMOVER_REG_SRC_D0_LEN2    0x60
#define DATAMOVER_REG_SRC_D0_STRIDE2 0x64
#define DATAMOVER_REG_SRC_D1_LEN2    0x68
#define DATAMOVER_REG_SRC_D1_STRIDE2 0x6C
#define DATAMOVER_REG_SRC_D2_STRIDE2 0x70
#define DATAMOVER_REG_DST_D0_LEN2    0x74
#define DATAMOVER_REG_DST_D0_STRIDE2 0x78
#define DATAMOVER_REG_DST_D1_LEN2    0x7C
#define DATAMOVER_REG_DST_D1_STRIDE2 0x80
#define DATAMOVER_REG_DST_D2_STRIDE2 0x84

/* Third Set of Registers */
#define DATAMOVER_REG_SRC_ADDR3      0x88
#define DATAMOVER_REG_DST_ADDR3      0x8C
#define DATAMOVER_REG_TOT_LEN3       0x90
#define DATAMOVER_REG_SRC_D0_LEN3    0x94
#define DATAMOVER_REG_SRC_D0_STRIDE3 0x98
#define DATAMOVER_REG_SRC_D1_LEN3    0x9C
#define DATAMOVER_REG_SRC_D1_STRIDE3 0xA0
#define DATAMOVER_REG_SRC_D2_STRIDE3 0xA4
#define DATAMOVER_REG_DST_D0_LEN3    0xA8
#define DATAMOVER_REG_DST_D0_STRIDE3 0xAC
#define DATAMOVER_REG_DST_D1_LEN3    0xB0
#define DATAMOVER_REG_DST_D1_STRIDE3 0xB4
#define DATAMOVER_REG_DST_D2_STRIDE3 0xB8


/* LOW-LEVEL HAL */
#define DATAMOVER_ADDR_BASE 0x100000
#define DATAMOVER_ADDR_SPACE 0x00000100

#define HWPE_WRITE(value, offset) *(volatile int *)(DATAMOVER_ADDR_BASE + offset) = value
#define HWPE_READ(offset) *(volatile int *)(DATAMOVER_ADDR_BASE + offset)

static inline void datamover_src_addr_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_SRC_ADDR);
}

static inline void datamover_dst_addr_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_DST_ADDR);
}

static inline void datamover_tot_len_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_TOT_LEN);
}

static inline void datamover_src_d0_len_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_SRC_D0_LEN);
}

static inline void datamover_src_d0_stride_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_SRC_D0_STRIDE);
}

static inline void datamover_src_d1_len_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_SRC_D1_LEN);
}

static inline void datamover_src_d1_stride_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_SRC_D1_STRIDE);
}

static inline void datamover_src_d2_stride_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_SRC_D2_STRIDE);
}

static inline void datamover_dst_d0_len_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_DST_D0_LEN);
}

static inline void datamover_dst_d0_stride_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_DST_D0_STRIDE);
}

static inline void datamover_dst_d1_len_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_DST_D1_LEN);
}

static inline void datamover_dst_d1_stride_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_DST_D1_STRIDE);
}

static inline void datamover_dst_d2_stride_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_DST_D2_STRIDE);
}

/* New Register Functions */
static inline void datamover_src_addr2_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_SRC_ADDR2);
}
static inline void datamover_dst_addr2_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_DST_ADDR2);
}
static inline void datamover_tot_len2_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_TOT_LEN2);
}
static inline void datamover_src_d0_len2_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_SRC_D0_LEN2);
}
static inline void datamover_src_d0_stride2_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_SRC_D0_STRIDE2);
}
static inline void datamover_src_d1_len2_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_SRC_D1_LEN2);
}
static inline void datamover_src_d1_stride2_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_SRC_D1_STRIDE2);
}
static inline void datamover_src_d2_stride2_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_SRC_D2_STRIDE2);
}
static inline void datamover_dst_d0_len2_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_DST_D0_LEN2);
}
static inline void datamover_dst_d0_stride2_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_DST_D0_STRIDE2);
}
static inline void datamover_dst_d1_len2_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_DST_D1_LEN2);
}
static inline void datamover_dst_d1_stride2_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_DST_D1_STRIDE2);
}
static inline void datamover_dst_d2_stride2_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_DST_D2_STRIDE2);
}

/* Third Set of Functions */
static inline void datamover_src_addr3_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_SRC_ADDR3);
}
static inline void datamover_dst_addr3_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_DST_ADDR3);
}
static inline void datamover_tot_len3_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_TOT_LEN3);
}
static inline void datamover_src_d0_len3_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_SRC_D0_LEN3);
}
static inline void datamover_src_d0_stride3_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_SRC_D0_STRIDE3);
}
static inline void datamover_src_d1_len3_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_SRC_D1_LEN3);
}
static inline void datamover_src_d1_stride3_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_SRC_D1_STRIDE3);
}
static inline void datamover_src_d2_stride3_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_SRC_D2_STRIDE3);
}
static inline void datamover_dst_d0_len3_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_DST_D0_LEN3);
}
static inline void datamover_dst_d0_stride3_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_DST_D0_STRIDE3);
}
static inline void datamover_dst_d1_len3_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_DST_D1_LEN3);
}
static inline void datamover_dst_d1_stride3_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_DST_D1_STRIDE3);
}
static inline void datamover_dst_d2_stride3_set(unsigned int value) {
  HWPE_WRITE(value, DATAMOVER_REG_DST_D2_STRIDE3);
}



static inline void datamover_trigger_job() {
  HWPE_WRITE(0, DATAMOVER_TRIGGER);
}

static inline int datamover_acquire_job() {
  return HWPE_READ(DATAMOVER_ACQUIRE);
}

static inline unsigned int datamover_get_status() {
  return HWPE_READ(DATAMOVER_STATUS);
}

static inline void datamover_soft_clear() {
  HWPE_WRITE(0, DATAMOVER_SOFT_CLEAR);
}

int datamover_compare_int(uint32_t *actual_out, uint32_t *golden_out, int len) {
  uint32_t actual_word = 0;
  uint32_t golden_word = 0;
  uint32_t actual = 0;
  uint32_t golden = 0;

  int errors = 0;
  int non_zero_values = 0;

  for (int i=0; i<len; i++) {
    actual_word = actual_out[i];
    golden_word = golden_out[i];

    int error = (actual_word != golden_word);
    errors += (actual_word != golden_word);
  }
  return errors;
}

#endif /* __HAL_DATAMOVER_H__ */

