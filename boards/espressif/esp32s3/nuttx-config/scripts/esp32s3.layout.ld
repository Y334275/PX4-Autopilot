/****************************************************************************
 * boards/xtensa/esp32s3/common/scripts/flat_memory.ld
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * ESP32-S3 Linker Script Memory Layout
 *
 * This file describes the memory layout (memory blocks) as virtual
 * memory addresses.
 *
 * <legacy/mcuboot>_sections.ld contains output sections to link compiler
 * output into these memory blocks.
 *
 ****************************************************************************/

MEMORY
{
  /* The 0x20 offset is a convenience for the app binary image generation.
   * Flash cache has 64KB pages. The .bin file which is flashed to the chip
   * has a 0x18 byte file header, and each segment has a 0x08 byte segment
   * header. Setting this offset makes it simple to meet the flash cache MMU's
   * constraint that (paddr % 64KB == vaddr % 64KB).
   */

  ROM (RX) :             org = 0x20,
                         len = 0x400000 - ORIGIN(ROM)

  /* Below values assume the flash cache is on, and have the blocks this
   * uses subtracted from the length of the various regions. The 'data access
   * port' dram/drom regions map to the same iram/irom regions but are
   * connected to the data port of the CPU and eg allow bytewise access.
   */

  /* IRAM for CPU */

  iram0_0_seg (RX) :                 org = (0x40370000 + 0x4000), len = (((0x403c0000 - (0x40378000 - 0x3fc88000)) - 0x3fc88000) + 0x8000 - 0x4000)

  /* Flash mapped instruction data. */

  /* (See ROM segment above for meaning of 0x20 offset.) */
  irom0_0_seg (RX) :                 org = 0x42000020, len = 0x400000 - 0x20

  /* Shared data RAM, excluding memory reserved for bootloader and ROM
   * bss/data/stack.
   */

  dram0_0_seg (RW) :                 org = (0x3fc88000), len = ((0x403c0000 - (0x40378000 - 0x3fc88000)) - 0x3fc88000)

  /* Flash mapped constant data */

  /* (See ROM segment above for meaning of 0x20 offset.) */
  drom0_0_seg (R) :                  org = 0x3c000020, len = 0x400000 - 0x20

  /* RTC fast memory (executable). Persists over deep sleep. */

  rtc_iram_seg(RWX) :                org = 0x600fe000, len = 0x2000 - 24

  /* RTC fast memory (same block as above), viewed from data bus */

  rtc_data_seg(RW)  :                org = 0x600fe000, len = 0x2000 - 24

  /* We reduced the size of rtc_iram_seg by 24 value.
     It reserves the amount of RTC fast memory that we use for this memory segment.
     This segment is intended for keeping:
       - (lower addr) rtc timer data (s_rtc_timer_retain_mem, see esp_clk.c files).
       - (higher addr) bootloader rtc data (s_bootloader_retain_mem, when a Kconfig option is on).
     The aim of this is to keep data that will not be moved around and have a fixed address.
  */
  rtc_reserved_seg(RW) :             org = 0x600fe000 + 0x2000 - 24, len = 24

  /* RTC slow memory (data accessible). Persists over deep sleep.
   * Start of RTC slow memory is reserved for ULP co-processor code + data,
   * if enabled.
   */

  rtc_slow_seg(RW)  :    org = 0x50000000 + 0,
                         len = 0x2000 - 0

  /* `extern_ram_seg` and `drom0_0_seg` share the same bus and the address region.
   * A dummy section is used to avoid overlap. See `.ext_ram.dummy` in `esp32s3_sections.ld
   */

}

  REGION_ALIAS("default_rodata_seg", drom0_0_seg);
  REGION_ALIAS("default_code_seg", irom0_0_seg);

/* Mark the end of the RTC heap (top of the RTC region) */

_ertcheap = ORIGIN(rtc_slow_seg) + LENGTH(rtc_slow_seg);
EXTERN(board_get_manifest)
