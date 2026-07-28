#ifndef __BOARD_MEM_H__
#define __BOARD_MEM_H__

#include "rtconfig.h"

#ifdef DATA_IN_ExtSRAM

#if defined(__ARMCC_VERSION)
#define PSRAM_BSS   __attribute__((section(".psram_bss")))
#define PSRAM_DATA  __attribute__((section(".psram_data")))
#elif defined(__GNUC__)
#define PSRAM_BSS   __attribute__((section(".psram_bss")))
#define PSRAM_DATA  __attribute__((section(".psram_data")))
#else
#define PSRAM_BSS
#define PSRAM_DATA
#endif

#else
#define PSRAM_BSS
#define PSRAM_DATA
#endif

#endif
