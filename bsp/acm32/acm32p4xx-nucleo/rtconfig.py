import os
import re
import sys

# toolchains options
ARCH='arm'
CPU='cortex-m33'
CROSS_TOOL='gcc'

# bsp lib config
BSP_LIBRARY_TYPE = None

if os.getenv('RTT_CC'):
    CROSS_TOOL = os.getenv('RTT_CC')
if os.getenv('RTT_ROOT'):
    RTT_ROOT = os.getenv('RTT_ROOT')

# cross_tool provides the cross compiler
# EXEC_PATH is the compiler execute path, for example, CodeSourcery, Keil MDK, IAR

if  CROSS_TOOL == 'gcc':
    PLATFORM     = 'gcc'
    EXEC_PATH     = r'/usr/bin'
elif CROSS_TOOL == 'keil':
    PLATFORM    = 'armclang'
    EXEC_PATH   = r'D:/Keil_v5'
elif CROSS_TOOL == 'iar':
    PLATFORM    = 'iccarm'
    EXEC_PATH   = r'C:/Program Files (x86)/IAR Systems/Embedded Workbench 8.3'

if os.getenv('RTT_EXEC_PATH'):
    EXEC_PATH = os.getenv('RTT_EXEC_PATH')

BUILD = 'debug'
#BUILD = 'release'

if PLATFORM == 'gcc':
    # toolchains
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
    # toolchains
    CC = 'armclang'
    AS = 'armasm'
    AR = 'armar'
    LINK = 'armlink'
    TARGET_EXT = 'axf'

    DEVICE = ' --cpu ' + CPU
    CFLAGS = ' -xc -std=c99 --target=arm-arm-none-eabi -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -c'
    CFLAGS += ' -fno-rtti -funsigned-char -fshort-enums -fshort-wchar -D__MICROLIB -mlittle-endian -ffunction-sections'

    AFLAGS = ' --cpu=Cortex-M33 --fpu=FPv5-SP --li --pd "__MICROLIB SETA 1" --pd "__UVISION_VERSION SETA 531" --pd "ARMCM33_DSP_FP SETA 1"'

    LFLAGS = ' --cpu=Cortex-M33 --info sizes --info totals --info unused --info veneers --list ./build/acm32p4xx.map --scatter ./build/acm32p4xx.sct'

    CXXFLAGS = CFLAGS

    POST_ACTION = 'fromelf --bincombined $TARGET --output rtthread.bin\nfromelf -z $TARGET'

elif PLATFORM == 'iccarm':
    # toolchains
    CC = 'iccarm'
    CXX = 'iccarm'
    AS = 'iasmarm'
    AR = 'iarchive'
    LINK = 'ilinkarm'
    TARGET_EXT = 'out'

    CFLAGS = '--endian=little --cpu=Cortex-M33 --fpu=FPv5-SP -e --diag_suppress=Pa089,Pa082,Pa050,Pa039,Pe188,Pe546,Pe111,Pa153'
    CFLAGS += ' -D__FPU_PRESENT=1 -D__FPU_USED=1'

    AFLAGS = '--cpu Cortex-M33 --fpu FPv5-SP -s+ -M<> -w+ -pl'

    LFLAGS = '--config drivers/linker_scripts/link.icf'

    CXXFLAGS = CFLAGS

# ---------------------------------------------------------------------------
# BSP resource conflict check (DMA unit+channel, pin groups)
# Called from SConstruct before compile.
# ---------------------------------------------------------------------------

def _bsp_parse_rtconfig(path):
    """Parse rtconfig.h -> {name: True | int | str}."""
    cfg = {}
    if not os.path.isfile(path):
        return cfg

    re_def_val = re.compile(r'^\s*#define\s+(\w+)\s+(.+)$')
    re_def_bare = re.compile(r'^\s*#define\s+(\w+)\s*$')
    re_int = re.compile(r'^-?\d+$')

    with open(path, 'r', encoding='utf-8', errors='ignore') as f:
        for line in f:
            line = line.split('/*', 1)[0].split('//', 1)[0].rstrip()
            m = re_def_val.match(line)
            if m:
                name, val = m.group(1), m.group(2).strip()
                cfg[name] = int(val) if re_int.match(val) else val
                continue
            m = re_def_bare.match(line)
            if m:
                cfg[m.group(1)] = True
    return cfg


def _bsp_enabled(cfg, name):
    return name in cfg and cfg[name] is not False and cfg[name] is not None


def _bsp_get_int(cfg, name, default):
    v = cfg.get(name, default)
    if isinstance(v, bool):
        return default
    try:
        return int(v)
    except (TypeError, ValueError):
        return default


def _bsp_resolve_dma_pair(cfg, prefix, n, def_tx, def_rx, alt_flag, alt_tx, alt_rx):
    """CUSTOM ints > ALT map > DEFAULT map > legacy ints > baked defaults."""
    custom = '%s%d_DMA_MAP_CUSTOM' % (prefix, n)
    default_flag = '%s%d_DMA_MAP_DEFAULT' % (prefix, n)

    if _bsp_enabled(cfg, custom):
        tx_u = _bsp_get_int(cfg, '%s%d_TX_DMA_UNIT' % (prefix, n), def_tx[0])
        tx_c = _bsp_get_int(cfg, '%s%d_TX_DMA_CH' % (prefix, n), def_tx[1])
        rx_u = _bsp_get_int(cfg, '%s%d_RX_DMA_UNIT' % (prefix, n), def_rx[0])
        rx_c = _bsp_get_int(cfg, '%s%d_RX_DMA_CH' % (prefix, n), def_rx[1])
    elif alt_flag and _bsp_enabled(cfg, alt_flag):
        tx_u, tx_c = alt_tx
        rx_u, rx_c = alt_rx
    elif _bsp_enabled(cfg, default_flag):
        tx_u, tx_c = def_tx
        rx_u, rx_c = def_rx
    elif '%s%d_TX_DMA_UNIT' % (prefix, n) in cfg:
        tx_u = _bsp_get_int(cfg, '%s%d_TX_DMA_UNIT' % (prefix, n), def_tx[0])
        tx_c = _bsp_get_int(cfg, '%s%d_TX_DMA_CH' % (prefix, n), def_tx[1])
        rx_u = _bsp_get_int(cfg, '%s%d_RX_DMA_UNIT' % (prefix, n), def_rx[0])
        rx_c = _bsp_get_int(cfg, '%s%d_RX_DMA_CH' % (prefix, n), def_rx[1])
    else:
        tx_u, tx_c = def_tx
        rx_u, rx_c = def_rx

    tag = 'UART' if prefix == 'BSP_UART' else 'SPI'
    return [
        ('%s%d_TX' % (tag, n), tx_u, tx_c),
        ('%s%d_RX' % (tag, n), rx_u, rx_c),
    ]


def _bsp_collect_dma(cfg):
    uart_def = {
        1: ((1, 1), (1, 2), 'BSP_UART1_DMA_MAP_DMA2', (2, 1), (2, 2)),
        2: ((1, 3), (2, 0), 'BSP_UART2_DMA_MAP_DMA1', (1, 0), (1, 1)),
        3: ((2, 2), (2, 3), 'BSP_UART3_DMA_MAP_DMA1', (1, 2), (1, 3)),
        4: ((2, 1), (1, 0), 'BSP_UART4_DMA_MAP_DMA1', (1, 2), (1, 3)),
    }
    spi_def = {
        1: ((2, 1), (2, 3), 'BSP_SPI1_DMA_MAP_DMA1', (1, 0), (1, 1)),
        2: ((1, 0), (1, 1), 'BSP_SPI2_DMA_MAP_DMA2', (2, 0), (2, 1)),
        3: ((2, 0), (2, 2), 'BSP_SPI3_DMA_MAP_DMA1', (1, 2), (1, 3)),
        4: ((1, 2), (1, 3), 'BSP_SPI4_DMA_MAP_DMA2', (2, 2), (2, 0)),
    }
    entries = []
    for n, (dtx, drx, alt, atx, arx) in uart_def.items():
        if _bsp_enabled(cfg, 'BSP_USING_UART%d' % n) and \
           _bsp_enabled(cfg, 'BSP_USING_UART%d_DMA' % n):
            entries.extend(_bsp_resolve_dma_pair(
                cfg, 'BSP_UART', n, dtx, drx, alt, atx, arx))
    for n, (dtx, drx, alt, atx, arx) in spi_def.items():
        if _bsp_enabled(cfg, 'BSP_USING_SPI%d' % n) and \
           _bsp_enabled(cfg, 'BSP_USING_SPI%d_DMA' % n):
            entries.extend(_bsp_resolve_dma_pair(
                cfg, 'BSP_SPI', n, dtx, drx, alt, atx, arx))
    if _bsp_enabled(cfg, 'BSP_USING_I2S1'):
        entries.append(('I2S1_TX', 1, 0))
    return entries


def _bsp_check_dma(cfg):
    errors = []
    infos = []
    entries = _bsp_collect_dma(cfg)
    slot_map = {}

    for owner, unit, ch in entries:
        if unit not in (1, 2):
            errors.append('%s: invalid DMA unit %s (need 1 or 2)' % (owner, unit))
            continue
        if ch not in (0, 1, 2, 3):
            errors.append('%s: invalid DMA channel %s (need 0~3)' % (owner, ch))
            continue
        slot_map.setdefault((unit, ch), []).append(owner)

    for (unit, ch), owners in sorted(slot_map.items()):
        if len(owners) >= 2:
            errors.append('DMA conflict: DMA%d_CH%d used by %s' % (
                unit, ch, ', '.join(owners)))

    if entries:
        lines = ['  DMA map:']
        for owner, unit, ch in entries:
            lines.append('    %-10s -> DMA%d_CH%d' % (owner, unit, ch))
        infos.append('\n'.join(lines))
    return errors, infos


def _bsp_check_timers(cfg):
    """TIM1/2/3/10 must not be shared by clock timer, PWM, capture or encoder."""
    conflicts = []
    groups = [
        ('TIM1', ['BSP_USING_TIM1', 'BSP_USING_PWM1']),
        ('TIM2', ['BSP_USING_TIM2', 'BSP_USING_PWM2',
                  'BSP_USING_CAPTURE2', 'BSP_USING_PULSE_ENCODER2']),
        ('TIM3', ['BSP_USING_TIM3', 'BSP_USING_PWM3',
                  'BSP_USING_CAPTURE3', 'BSP_USING_PULSE_ENCODER3']),
        ('TIM10', ['BSP_USING_TIM10', 'BSP_USING_PWM10']),
    ]
    for name, users in groups:
        enabled = [u for u in users if _bsp_enabled(cfg, u)]
        if len(enabled) >= 2:
            conflicts.append('%s shared by %s' % (name, ', '.join(enabled)))
    return conflicts


# Per-signal pin macro suffix → pin name for conflict detection.
# Order: specific choices first, fallback default last.
_BSP_UART_TX_PINS = {
    1: ['PB6', 'PA14', 'PA9'],
    2: ['PA2', 'PC2', 'PD5'],
    3: ['PD8', 'PC10', 'PB10'],
    4: ['PC10', 'PA0', 'PB9', 'PE9', 'PD1'],
}
_BSP_UART_RX_PINS = {
    1: ['PB7', 'PA13', 'PA10'],
    2: ['PA3', 'PC1', 'PD6'],
    3: ['PD9', 'PC11', 'PB11'],
    4: ['PC11', 'PA1', 'PB8', 'PE10', 'PD0'],
}
_BSP_LPUART_TX_PINS = {
    1: ['PB6', 'PA9'],
    2: ['PE3', 'PC0'],
}
_BSP_LPUART_RX_PINS = {
    1: ['PB7', 'PA10'],
    2: ['PE4', 'PC1'],
}
_BSP_SPI_SCK_PINS = {
    1: ['PA5', 'PB3', 'PE12'],
    2: ['PC7', 'PF8', 'PA9', 'PB13'],
    3: ['PB3', 'PG0', 'PC10'],
    4: ['PB2', 'PB9', 'PE12', 'PE2'],
}
_BSP_SPI_MOSI_PINS = {
    1: ['PA7', 'PB5', 'PE11'],
    2: ['PC9', 'PF0', 'PC1', 'PB15'],
    3: ['PB5', 'PG2', 'PD6', 'PC12'],
    4: ['PB1', 'PB15', 'PE14', 'PB14', 'PE6'],
}
_BSP_SPI_MISO_PINS = {
    1: ['PA6', 'PB4', 'PE10'],
    2: ['PC8', 'PF4', 'PC2', 'PB14'],
    3: ['PB4', 'PG6', 'PC11'],
    4: ['PB6', 'PE13', 'PE5'],
}
_BSP_SPI_CS_PINS = {
    1: ['PA4', 'PA15', 'PE1', 'PE13'],
    2: ['PC6', 'PF6', 'PB9', 'PB12'],
    3: ['PA15', 'PG4', 'PC9', 'PA4'],
    4: ['PA11', 'PB12', 'PE4'],
}

_BSP_ETH_PINS = ('PA1', 'PA2', 'PA7', 'PB11', 'PB12', 'PB13', 'PC1', 'PC4', 'PC5')

_BSP_PWM_PINS = {
    'PWM1_CH1': ['PE9', 'PA8'],
    'PWM1_CH2': ['PE11', 'PA9'],
    'PWM1_CH3': ['PE13', 'PA10'],
    'PWM1_CH4': ['PE14', 'PA11'],
    'PWM2_CH1': ['PA0', 'PA5'],
    'PWM2_CH2': ['PA1', 'PB3'],
    'PWM2_CH3': ['PA2', 'PB10'],
    'PWM2_CH4': ['PA3', 'PB11'],
    'PWM3_CH1': ['PA6', 'PB4', 'PC6'],
    'PWM3_CH2': ['PA7', 'PB5', 'PC7'],
    'PWM3_CH3': ['PB0', 'PC8'],
    'PWM3_CH4': ['PB1', 'PC9'],
    'PWM10_CH1': ['PF7', 'PA4', 'PB8', 'PE3'],
}

_BSP_I2C_PINS = {
    'I2C1_SCL': ['PA13', 'PF11', 'PB6'],
    'I2C1_SDA': ['PA14', 'PF12', 'PB7'],
    'I2C2_SCL': ['PB10', 'PG10', 'PE1'],
    'I2C2_SDA': ['PB11', 'PG11', 'PE0'],
}

_BSP_FDCAN_PINS = {
    'FDCAN1_TX': ['PB9', 'PA12', 'PD1'],
    'FDCAN1_RX': ['PB8', 'PA11', 'PD0'],
    'FDCAN2_TX': ['PD12', 'PE6'],
    'FDCAN2_RX': ['PD11', 'PE5'],
}

_BSP_I2S_PINS = {
    'I2S1_WS': ['PA15', 'PA4'],
    'I2S1_CK': ['PC10', 'PB3', 'PA5'],
    'I2S1_SDI': ['PC11', 'PB4', 'PA6'],
    'I2S1_SDO': ['PB5', 'PD7', 'PC12'],
    'I2S1_MCK': ['PC7', 'PC4'],
}

_BSP_SDMMC_PINS = ('PC8', 'PC9', 'PC10', 'PC11', 'PC12', 'PD2')


def _bsp_pin_index_to_name(idx):
    """RT-Thread pin index -> 'PXn' (PA0=0, PB0=16, ...)."""
    try:
        idx = int(idx)
    except (TypeError, ValueError):
        return None
    if idx < 0:
        return None
    port = idx // 16
    pin = idx % 16
    if port > 7:
        return None
    return 'P%c%d' % (chr(ord('A') + port), pin)


def _bsp_scan_pin(cfg, prefix, suffixes):
    """Scan per-signal macro like BSP_UART1_TX_PA9 using suffix list.
    Returns the first matching pin name (e.g. 'PA9'), or default (last) entry."""
    for s in suffixes:
        if _bsp_enabled(cfg, prefix + s):
            return s
    return suffixes[-1] if suffixes else None


def _bsp_check_pins(cfg):
    errors = []
    infos = []
    pin_map = {}
    info_lines = []

    def add(owner, pins):
        for p in pins:
            if p:
                pin_map.setdefault(p, []).append(owner)

    # UART1-4: TX + RX per-signal conflict check
    for n in [1, 2, 3, 4]:
        if not _bsp_enabled(cfg, 'BSP_USING_UART%d' % n):
            continue
        tx = _bsp_scan_pin(cfg, 'BSP_UART%d_TX_' % n, _BSP_UART_TX_PINS[n])
        rx = _bsp_scan_pin(cfg, 'BSP_UART%d_RX_' % n, _BSP_UART_RX_PINS[n])
        if tx:
            add('UART%d_TX' % n, [tx])
        if rx:
            add('UART%d_RX' % n, [rx])
        info_lines.append('    UART%d   %s %s' % (n, tx or '?', rx or '?'))

    # LPUART1-2: TX + RX per-signal conflict check
    for n in [1, 2]:
        if not _bsp_enabled(cfg, 'BSP_USING_LPUART%d' % n):
            continue
        tx = _bsp_scan_pin(cfg, 'BSP_LPUART%d_TX_' % n, _BSP_LPUART_TX_PINS[n])
        rx = _bsp_scan_pin(cfg, 'BSP_LPUART%d_RX_' % n, _BSP_LPUART_RX_PINS[n])
        if tx:
            add('LPUART%d_TX' % n, [tx])
        if rx:
            add('LPUART%d_RX' % n, [rx])
        info_lines.append('    LPUART%d %s %s' % (n, tx or '?', rx or '?'))

    # SPI1-4: SCK + MOSI + MISO + CS per-signal conflict check
    for n in [1, 2, 3, 4]:
        if not _bsp_enabled(cfg, 'BSP_USING_SPI%d' % n):
            continue
        sck  = _bsp_scan_pin(cfg, 'BSP_SPI%d_SCK_'  % n, _BSP_SPI_SCK_PINS[n])
        mosi = _bsp_scan_pin(cfg, 'BSP_SPI%d_MOSI_' % n, _BSP_SPI_MOSI_PINS[n])
        miso = _bsp_scan_pin(cfg, 'BSP_SPI%d_MISO_' % n, _BSP_SPI_MISO_PINS[n])
        cs   = _bsp_scan_pin(cfg, 'BSP_SPI%d_CS_'   % n, _BSP_SPI_CS_PINS[n])
        if sck:
            add('SPI%d_SCK' % n, [sck])
        if mosi:
            add('SPI%d_MOSI' % n, [mosi])
        if miso:
            add('SPI%d_MISO' % n, [miso])
        if cs:
            add('SPI%d_CS' % n, [cs])
        info_lines.append('    SPI%d    %s %s %s %s' %
                          (n, sck or '?', mosi or '?', miso or '?', cs or '?'))

    # ETH pins
    if _bsp_enabled(cfg, 'BSP_USING_ETH'):
        eth_pins = list(_BSP_ETH_PINS)
        rst = _bsp_pin_index_to_name(cfg.get('BSP_ETH_PHY_RST_PIN', 30))
        if rst:
            eth_pins.append(rst)
        add('ETH', eth_pins)
        info_lines.append('    ETH      (RMII board pins + PHY_RST=%s)' %
                          (rst if rst else '?'))

    # PWM1-3/10 pins
    for unit, enabled_name in [('PWM1', 'BSP_USING_PWM1'),
                               ('PWM2', 'BSP_USING_PWM2'),
                               ('PWM3', 'BSP_USING_PWM3'),
                               ('PWM10', 'BSP_USING_PWM10')]:
        if not _bsp_enabled(cfg, enabled_name):
            continue
        pins = []
        for ch in range(1, 5):
            key = '%s_CH%d' % (unit, ch)
            if key not in _BSP_PWM_PINS:
                continue
            if not _bsp_enabled(cfg, 'BSP_USING_%s_CH%d' % (unit, ch)):
                continue
            pin = _bsp_scan_pin(cfg, 'BSP_%s_CH%d_' % (unit, ch),
                                _BSP_PWM_PINS[key])
            if pin:
                add('%s_CH%d' % (unit, ch), [pin])
                pins.append('%s=%s' % (ch, pin))
        if pins:
            info_lines.append('    %s     %s' % (unit, ' '.join(pins)))

    # I2C1/2 pins
    for n in [1, 2]:
        if not _bsp_enabled(cfg, 'BSP_USING_I2C%d' % n):
            continue
        scl = _bsp_scan_pin(cfg, 'BSP_I2C%d_SCL_' % n, _BSP_I2C_PINS['I2C%d_SCL' % n])
        sda = _bsp_scan_pin(cfg, 'BSP_I2C%d_SDA_' % n, _BSP_I2C_PINS['I2C%d_SDA' % n])
        if scl:
            add('I2C%d_SCL' % n, [scl])
        if sda:
            add('I2C%d_SDA' % n, [sda])
        info_lines.append('    I2C%d    %s %s' % (n, scl or '?', sda or '?'))

    # FDCAN1/2 pins
    for n in [1, 2]:
        if not _bsp_enabled(cfg, 'BSP_USING_FDCAN%d' % n):
            continue
        tx = _bsp_scan_pin(cfg, 'BSP_FDCAN%d_TX_' % n, _BSP_FDCAN_PINS['FDCAN%d_TX' % n])
        rx = _bsp_scan_pin(cfg, 'BSP_FDCAN%d_RX_' % n, _BSP_FDCAN_PINS['FDCAN%d_RX' % n])
        if tx:
            add('FDCAN%d_TX' % n, [tx])
        if rx:
            add('FDCAN%d_RX' % n, [rx])
        info_lines.append('    FDCAN%d %s %s' % (n, tx or '?', rx or '?'))

    # I2S1 pins
    if _bsp_enabled(cfg, 'BSP_USING_I2S1'):
        pins = []
        for key, label in [('I2S1_WS', 'WS'), ('I2S1_CK', 'CK'),
                           ('I2S1_SDI', 'SDI'), ('I2S1_SDO', 'SDO'),
                           ('I2S1_MCK', 'MCK')]:
            pin = _bsp_scan_pin(cfg, 'BSP_%s_' % key, _BSP_I2S_PINS[key])
            if pin:
                add('I2S1_%s' % label, [pin])
                pins.append('%s=%s' % (label, pin))
        if pins:
            info_lines.append('    I2S1     %s' % ' '.join(pins))

    # SDMMC fixed pin group
    if _bsp_enabled(cfg, 'BSP_USING_SDMMC1'):
        add('SDMMC1', _BSP_SDMMC_PINS)
        info_lines.append('    SDMMC1   %s' % ' '.join(_BSP_SDMMC_PINS))

    # Check conflicts
    for pin, owners in sorted(pin_map.items()):
        uniq = []
        for o in owners:
            if o not in uniq:
                uniq.append(o)
        if len(uniq) >= 2:
            errors.append('Pin conflict: %s used by %s' % (pin, ', '.join(uniq)))

    if pin_map:
        lines = ['  Pin groups:'] + info_lines
        infos.append('\n'.join(lines))

    return errors, infos


def _bsp_c_escape(s):
    """Escape a string for use inside a C string literal."""
    return s.replace('\\', '\\\\').replace('"', '\\"')


# Marker block inside rtconfig.h (single place, no extra headers).
_BSP_RES_BEGIN = '/* BEGIN_BSP_RESOURCE_CHECK - auto by SCons, do not edit */'
_BSP_RES_END = '/* END_BSP_RESOURCE_CHECK */'
_BSP_RES_BLOCK_RE = re.compile(
    r'\n?' + re.escape(_BSP_RES_BEGIN) + r'.*?' + re.escape(_BSP_RES_END) + r'\n?',
    re.DOTALL)


def _bsp_build_resource_section(conflicts, infos):
    """Build the text block injected into rtconfig.h."""
    lines = [_BSP_RES_BEGIN]

    if infos:
        lines.append('/* ---- resource map ----')
        for info in infos:
            for ln in info.splitlines():
                lines.append(' * %s' % ln)
        lines.append(' */')

    if conflicts:
        lines.append('/* ---- conflicts: fix menuconfig DMA / pin groups ---- */')
        for c in conflicts:
            msg = _bsp_c_escape(c)
            lines.append('#warning "BSP: %s"' % msg)
        lines.append('#ifdef BSP_RESOURCE_CHECK_FATAL')
        for c in conflicts:
            msg = _bsp_c_escape(c)
            lines.append('#error "BSP: %s"' % msg)
        lines.append('#endif')
    else:
        lines.append('/* BSP resource check OK (no DMA / pin conflicts) */')

    lines.append(_BSP_RES_END)
    lines.append('')
    return '\n'.join(lines)


def _bsp_update_rtconfig_h(rtconfig_path, conflicts, infos):
    """
    Inject / refresh resource-check block inside rtconfig.h (before final #endif).
    Returns True if file content changed.
    """
    try:
        with open(rtconfig_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
    except OSError:
        return False

    # Drop previous auto block (if any)
    content = _BSP_RES_BLOCK_RE.sub('\n', content)
    if not content.endswith('\n'):
        content += '\n'

    section = _bsp_build_resource_section(conflicts, infos)

    # Prefer insert before the last top-level #endif of the include guard
    idx = content.rfind('\n#endif')
    if idx < 0:
        idx = content.rfind('#endif')
        if idx >= 0:
            new_content = content[:idx] + section + content[idx:]
        else:
            new_content = content + section
    else:
        new_content = content[:idx + 1] + section + content[idx + 1:]

    if new_content == content and _BSP_RES_BEGIN in content:
        # content already had same section removed incorrectly; compare full rewrite
        pass

    # Detect no-op: re-read would match if section already identical
    try:
        with open(rtconfig_path, 'r', encoding='utf-8', errors='ignore') as f:
            old = f.read()
    except OSError:
        old = None

    # Rebuild expected full file for comparison
    base = _BSP_RES_BLOCK_RE.sub('\n', old if old is not None else content)
    if not base.endswith('\n'):
        base += '\n'
    bidx = base.rfind('\n#endif')
    if bidx >= 0:
        expected = base[:bidx + 1] + section + base[bidx + 1:]
    else:
        expected = base + section

    if old == expected:
        return False

    with open(rtconfig_path, 'w', encoding='utf-8', newline='\n') as f:
        f.write(expected)
    return True


def check_bsp_resources(rtconfig_path=None, fatal=False, quiet=False, show_map=True):
    """
    Check DMA unit+channel and pin-group conflicts from rtconfig.h.

    Behavior:
      - Print resource map + conflicts (unless quiet).
      - Do not abort SCons by default (fatal=False).
      - Inject a marked block into rtconfig.h with #warning / optional #error
        so the compiler reports conflicts (rtconfig.h is already included
        everywhere; no extra headers).

    Call from SConstruct before PrepareBuilding.
    After menuconfig regenerates rtconfig.h, re-run SCons to refresh the block.
    """
    bsp_dir = os.path.dirname(os.path.abspath(__file__))
    if rtconfig_path is None:
        rtconfig_path = os.path.join(bsp_dir, 'rtconfig.h')

    cfg = _bsp_parse_rtconfig(rtconfig_path)
    if not cfg:
        msg = 'check_bsp_resources: cannot read %s' % rtconfig_path
        if not quiet:
            print('[BSP WARNING] ' + msg)
        return [msg], []

    conflicts = []
    infos = []

    e, w = _bsp_check_dma(cfg)
    conflicts.extend(e)
    if show_map:
        infos.extend(w)

    e, w = _bsp_check_pins(cfg)
    conflicts.extend(e)
    if show_map:
        infos.extend(w)

    conflicts.extend(_bsp_check_timers(cfg))

    if not quiet:
        for info in infos:
            print('[BSP INFO]\n' + info)
        if conflicts:
            print('[BSP WARNING] resource conflicts detected (%d):' % len(conflicts))
            for err in conflicts:
                print('[BSP WARNING]   ' + err)
            print('[BSP WARNING] SCons continues; #warning injected into rtconfig.h')
        else:
            print('[BSP INFO] resource check OK (DMA + pin groups)')

    written = _bsp_update_rtconfig_h(rtconfig_path, conflicts, infos)
    if not quiet and written:
        print('[BSP INFO] updated resource-check block in %s' % rtconfig_path)

    if conflicts and fatal:
        print('[BSP ERROR] fatal=True: aborting SCons due to resource conflicts')
        sys.exit(1)

    return conflicts, infos
