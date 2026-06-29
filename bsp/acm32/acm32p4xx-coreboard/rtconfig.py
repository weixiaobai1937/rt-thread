import os

ARCH='arm'
CPU='cortex-m33'
CROSS_TOOL='gcc'
BSP_LIBRARY_TYPE = None

if os.getenv('RTT_CC'):
    CROSS_TOOL = os.getenv('RTT_CC')
if os.getenv('RTT_ROOT'):
    RTT_ROOT = os.getenv('RTT_ROOT')

if  CROSS_TOOL == 'gcc':
    PLATFORM     = 'gcc'
    EXEC_PATH     = r'/usr/bin'
elif CROSS_TOOL == 'keil':
    PLATFORM    = 'armclang'
    EXEC_PATH   = r'C:/Keil_v5/ARM/ARMCLANG/bin'
elif CROSS_TOOL == 'iar':
    PLATFORM    = 'iccarm'
    EXEC_PATH   = r'C:/Program Files (x86)/IAR Systems/Embedded Workbench 8.3'

if os.getenv('RTT_EXEC_PATH'):
    EXEC_PATH = os.getenv('RTT_EXEC_PATH')

BUILD = 'debug'

if PLATFORM == 'gcc':
    PREFIX = 'arm-none-eabi-'
    CC = PREFIX + 'gcc'
    AS = PREFIX + 'gcc'
    AR = PREFIX + 'ar'
    CXX = PREFIX + 'g++'
    LINK = PREFIX + 'gcc'
    TARGET_EXT = 'elf'
    SIZE = PREFIX + 'size'
    OBJDUMP = PREFIX + 'objdump'
    OBJCPY = PREFIX + 'objcopy'

    DEVICE = ' -mcpu=' + CPU + ' -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard -ffunction-sections -fdata-sections'
    CFLAGS = DEVICE
    AFLAGS = ' -c' + DEVICE + ' -x assembler-with-cpp -Wa,-mimplicit-it=thumb '
    LFLAGS = DEVICE + ' -Wl,--gc-sections,-Map=rtthread.map,-cref,-u,Reset_Handler -T drivers/linker_scripts/link.lds'

    CPATH = ''
    LPATH = ''
    if BUILD == 'debug':
        CFLAGS += ' -O0 -gdwarf-2 -g'
        AFLAGS += ' -gdwarf-2'
    else:
        CFLAGS += ' -O2'
    CXXFLAGS = CFLAGS
    POST_ACTION = OBJCPY + ' -O binary $TARGET rtthread.bin\n' + SIZE + ' $TARGET \n'

elif PLATFORM == 'armclang':
    CC = 'armclang'
    AS = 'armasm'
    AR = 'armar'
    LINK = 'armlink'
    TARGET_EXT = 'axf'
    CFLAGS = ' -xc -std=c99 --target=arm-arm-none-eabi -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -c'
    CFLAGS += ' -fno-rtti -funsigned-char -fshort-enums -fshort-wchar -D__MICROLIB -mlittle-endian -ffunction-sections'
    AFLAGS = ' --cpu=Cortex-M33 --fpu=FPv5-SP --li'
    LFLAGS = ' --cpu=Cortex-M33 --info sizes --info totals --info unused --info veneers --list ./build/acm32p4xx.map --scatter ./build/acm32p4xx.sct'
    CXXFLAGS = CFLAGS
    POST_ACTION = 'fromelf --bincombined $TARGET --output rtthread.bin\nfromelf -z $TARGET'

elif PLATFORM == 'iccarm':
    CC = 'iccarm'
    CXX = 'iccarm'
    AS = 'iasmarm'
    AR = 'iarchive'
    LINK = 'ilinkarm'
    TARGET_EXT = 'out'
    CFLAGS = '--endian=little --cpu=Cortex-M33 --fpu=FPv5-SP -e'
    AFLAGS = '--cpu Cortex-M33 --fpu FPv5-SP -s+ -M<> -w+ -pl'
    LFLAGS = '--config drivers/linker_scripts/link.icf'
    CXXFLAGS = CFLAGS
