/**
 * @file usb_cdc_example.c
 * @brief ACM32P4 USB CDC虚拟串口 Device模式示例
 * @author ACM32P4 SDK Team
 * @version 1.0
 *
 * @details
 * 演示1个场景：
 * 1. USB CDC虚拟串口（回环通讯）
 *    - PC通过USB连接ACM32P4
 *    - 设备枚举为CDC ACM设备（虚拟COM口）
 *    - 收到PC发来的数据后原样回送（回环）
 *    - 同时通过RTT打印调试信息
 *
 * @note
 * CDC端点分配：
 *   EP0    IN/OUT  控制传输（由驱动自动处理）
 *   EP1    IN      CDC通知端点（Interrupt，64B）
 *   EP2    OUT     数据接收（Bulk，512B）
 *   EP3    IN      数据发送（Bulk，512B）
 *
 * GPIO引脚（USB_1，内置HS PHY）：
 *   USB1_DP / USB1_DM — 片上PHY，无需GPIO配置
 *   VBUS               — 根据硬件决定是否接检测引脚
 */

#include "SEGGER_RTT.h"
#include "acm32p4.h"
#include "hardware/clocks.h"
#include "hardware/system.h"
#include "hardware/usb.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

//===========================================
// 配置选项
//===========================================

#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)

/** 使用的USB实例 */
#define CDC_USB_INST USB_1

/** CDC端点编号 */
#define CDC_EP_NOTIFY 1U   ///< 通知端点（IN，Interrupt）
#define CDC_EP_DATA_OUT 2U ///< 数据接收端点（OUT，Bulk）
#define CDC_EP_DATA_IN 3U  ///< 数据发送端点（IN，Bulk）

/** 端点最大包长 */
#define CDC_EP_NOTIFY_MPS 64U
#define CDC_EP_DATA_MPS     64U   ///< 全速(FS) Bulk端点最大包长为64B

/** 接收缓冲区大小 */
#define CDC_RX_BUF_SIZE     512U

//===========================================
// CDC类标准定义
//===========================================

/** USB标准请求 bRequest */
#define USB_REQ_SET_ADDRESS 0x05U
#define USB_REQ_GET_DESCRIPTOR 0x06U
#define USB_REQ_SET_CONFIGURATION 0x09U

/** CDC类请求 bRequest */
#define CDC_REQ_SET_LINE_CODING 0x20U
#define CDC_REQ_GET_LINE_CODING 0x21U
#define CDC_REQ_SET_CONTROL_LINE 0x22U

/** 描述符类型 */
#define USB_DESC_TYPE_DEVICE 0x01U
#define USB_DESC_TYPE_CONFIG 0x02U
#define USB_DESC_TYPE_STRING 0x03U
#define USB_DESC_TYPE_QUALIFIER 0x06U
#define USB_DESC_TYPE_OTHER_SPEED 0x07U

/** SETUP包字段偏移 */
#define SETUP_BMW_TYPE 0U
#define SETUP_BREQUEST 1U
#define SETUP_WVALUE_L 2U
#define SETUP_WVALUE_H 3U
#define SETUP_WINDEX_L 4U
#define SETUP_WINDEX_H 5U
#define SETUP_WLENGTH_L 6U
#define SETUP_WLENGTH_H 7U

//===========================================
// USB描述符
//===========================================

/** 设备描述符（18字节） */
static const uint8_t s_dev_desc[18] = {
    0x12,       /* bLength */
    0x01,       /* bDescriptorType: Device */
    0x00, 0x02, /* bcdUSB: 2.00 */
    0xEF,        /* bDeviceClass: Miscellaneous（IAD必需，让Windows匹配usbser.sys） */
    0x02,        /* bDeviceSubClass: Common Class */
    0x01,        /* bDeviceProtocol: Interface Association Descriptor */
    0x40,       /* bMaxPacketSize0: 64 */
    0x83, 0x04, /* idVendor: 0x0483 (STMicro, 示例用) */
    0x40, 0x57, /* idProduct: 0x5740 */
    0x00, 0x02, /* bcdDevice: 2.00 */
    0x01,       /* iManufacturer: String 1 */
    0x02,       /* iProduct: String 2 */
    0x03,       /* iSerialNumber: String 3 */
    0x01,       /* bNumConfigurations */
};

/** 配置描述符集合（配置+接口+端点，共75字节） */
static const uint8_t s_cfg_desc[75] = {
    /* 配置描述符 */
    0x09, /* bLength */
    0x02, /* bDescriptorType: Configuration */
    0x4B,
    0x00, /* wTotalLength: 75 */
    0x02, /* bNumInterfaces: 2 */
    0x01, /* bConfigurationValue */
    0x00, /* iConfiguration */
    0xC0, /* bmAttributes: Bus-powered + Self-powered */
    0x32, /* bMaxPower: 100mA */

    /* IAD — 接口关联描述符（让Windows自动匹配usbser.sys驱动，CDC必需） */
    0x08,        /* bLength */
    0x0B,        /* bDescriptorType: IAD */
    0x00,        /* bFirstInterface: 0 */
    0x02,        /* bInterfaceCount: 2 */
    0x02,        /* bFunctionClass: CDC */
    0x02,        /* bFunctionSubClass: ACM */
    0x01,        /* bFunctionProtocol: V.250 */
    0x00,        /* iFunction */

    /* 接口0：CDC通信接口 */
    0x09, /* bLength */
    0x04, /* bDescriptorType: Interface */
    0x00, /* bInterfaceNumber: 0 */
    0x00, /* bAlternateSetting */
    0x01, /* bNumEndpoints: 1 */
    0x02, /* bInterfaceClass: CDC */
    0x02, /* bInterfaceSubClass: ACM */
    0x01, /* bInterfaceProtocol: V.250 */
    0x00, /* iInterface */

    /* CDC Header功能描述符 */
    0x05,
    0x24,
    0x00,
    0x10,
    0x01,

    /* CDC Call Management功能描述符 */
    0x05,
    0x24,
    0x01,
    0x00,
    0x01,

    /* CDC ACM功能描述符 */
    0x04,
    0x24,
    0x02,
    0x02,

    /* CDC Union功能描述符 */
    0x05,
    0x24,
    0x06,
    0x00,
    0x01,

    /* EP1 IN — 通知端点 */
    0x07, /* bLength */
    0x05, /* bDescriptorType: Endpoint */
    0x81, /* bEndpointAddress: IN EP1 */
    0x03, /* bmAttributes: Interrupt */
    0x40,
    0x00, /* wMaxPacketSize: 64 */
    0x10, /* bInterval: 16ms */

    /* 接口1：CDC数据接口 */
    0x09, /* bLength */
    0x04, /* bDescriptorType: Interface */
    0x01, /* bInterfaceNumber: 1 */
    0x00, /* bAlternateSetting */
    0x02, /* bNumEndpoints: 2 */
    0x0A, /* bInterfaceClass: CDC-Data */
    0x00, /* bInterfaceSubClass */
    0x00, /* bInterfaceProtocol */
    0x00, /* iInterface */

    /* EP2 OUT — 数据接收 */
    0x07,
    0x05,
    0x02, /* bEndpointAddress: OUT EP2 */
    0x02, /* bmAttributes: Bulk */
    0x40,
    0x00, /* wMaxPacketSize: 64（FS Bulk最大包长） */
    0x00, /* bInterval */

    /* EP3 IN — 数据发送 */
    0x07,
    0x05,
    0x83, /* bEndpointAddress: IN EP3 */
    0x02, /* bmAttributes: Bulk */
    0x40,
    0x00, /* wMaxPacketSize: 64（FS Bulk最大包长） */
    0x00, /* bInterval */
};

/** 语言ID字符串描述符 */
static const uint8_t s_str_lang[4] = {
    0x04, 0x03, 0x09, 0x04 /* 英语(美国) */
};

/** 厂商字符串 */
static const uint8_t s_str_mfr[] = {0x12, 0x03, 'A', 0, 'C', 0, 'M', 0, '3', 0,
                                    '2',  0,    'H', 0, '5', 0, ' ', 0};

/** 产品字符串 */
static const uint8_t s_str_prod[] = {0x16, 0x03, 'U', 0, 'S', 0, 'B', 0,
                                     ' ',  0,    'C', 0, 'D', 0, 'C', 0,
                                     ' ',  0,    'D', 0, 'E', 0};

/** 序列号字符串 */
static const uint8_t s_str_sn[] = {0x1A, 0x03, '0', 0, '0', 0, '0', 0, '0', 0,
                                   '0',  0,    '0', 0, '0', 0, '1', 0, '2', 0,
                                   '3',  0,    '4', 0, '5', 0};

//===========================================
// CDC Line Coding 默认值
//===========================================

/** 线路编码（115200, 8N1） */
static uint8_t s_line_coding[7] = {
    0x00, 0xC2, 0x01, 0x00, /* dwDTERate: 115200 bps */
    0x00,                   /* bCharFormat: 1停止位 */
    0x00,                   /* bParityType: None */
    0x08,                   /* bDataBits: 8 */
};

//===========================================
// 全局状态
//===========================================

/** 设备是否已枚举完成 */
static volatile bool s_configured = false;

/** 当前枚举速度 */
static volatile usb_speed_t s_speed = USB_SPEED_FULL;

/** EP0控制传输发送缓冲区 */
static uint8_t s_ep0_tx_buf[64];

/** CDC接收缓冲区 */
static uint8_t s_cdc_rx_buf[CDC_RX_BUF_SIZE];

/** 待回环发送标志 */
static volatile bool s_cdc_tx_pending = false;
static volatile uint32_t s_cdc_tx_len = 0U;
static volatile bool s_cdc_tx_in_flight = false;
static volatile uint32_t s_cdc_rx_count = 0U;   ///< 本次on_data_out实际收到的字节数

//===========================================
// 辅助函数
//===========================================

/**
 * @brief 打印分隔线
 */
static void print_separator(void) {
  printf("==========================================\n");
}

/**
 * @brief 十六进制打印数据
 *
 * @param data 数据缓冲区
 * @param len  字节数
 */
static void print_hex(const uint8_t *data, uint32_t len) {
  for (uint32_t i = 0U; i < len; i++) {
    printf("%02X ", data[i]);
    if ((i + 1U) % 16U == 0U) {
      printf("\n");
    }
  }
  if (len % 16U != 0U) {
    printf("\n");
  }
}

//===========================================
// CDC端点配置辅助函数
//===========================================

/**
 * @brief 打开CDC所有端点
 *
 * 在SetConfiguration完成后调用，激活EP1/EP2/EP3。
 */
static void cdc_ep_open(void) {
  usb_ep_config_t ep = {0};

  /* EP1 IN — 通知（Interrupt，64B） */
  ep.ep_num = CDC_EP_NOTIFY;
  ep.direction = USB_EP_DIR_IN;
  ep.type = USB_EP_TYPE_INTR;
  ep.max_packet_size = CDC_EP_NOTIFY_MPS;
  usb_config_ep_activate(CDC_USB_INST, &ep);
  usb_ep_int_enable(CDC_USB_INST, 0x80U | CDC_EP_NOTIFY);

  /* EP2 OUT — 数据接收（Bulk，512B） */
  ep.ep_num = CDC_EP_DATA_OUT;
  ep.direction = USB_EP_DIR_OUT;
  ep.type = USB_EP_TYPE_BULK;
  ep.max_packet_size = CDC_EP_DATA_MPS;
  usb_config_ep_activate(CDC_USB_INST, &ep);
  usb_ep_int_enable(CDC_USB_INST, CDC_EP_DATA_OUT);

  /* EP3 IN — 数据发送（Bulk，512B） */
  ep.ep_num = CDC_EP_DATA_IN;
  ep.direction = USB_EP_DIR_IN;
  ep.type = USB_EP_TYPE_BULK;
  ep.max_packet_size = CDC_EP_DATA_MPS;
  usb_config_ep_activate(CDC_USB_INST, &ep);
  usb_ep_int_enable(CDC_USB_INST, 0x80U | CDC_EP_DATA_IN);

  /* 准备第一次接收 */
  usb_ep_read_start(CDC_USB_INST, CDC_EP_DATA_OUT, s_cdc_rx_buf,
                    CDC_RX_BUF_SIZE);

  printf("  [CDC] 端点已打开，等待数据...\n");
}

/**
 * @brief 关闭CDC所有端点
 *
 * 在总线复位或断开时调用。
 */
static void cdc_ep_close(void) {
  usb_config_ep_deactivate(CDC_USB_INST, CDC_EP_NOTIFY, USB_EP_DIR_IN);
  usb_config_ep_deactivate(CDC_USB_INST, CDC_EP_DATA_OUT, USB_EP_DIR_OUT);
  usb_config_ep_deactivate(CDC_USB_INST, CDC_EP_DATA_IN, USB_EP_DIR_IN);

  usb_ep_int_disable(CDC_USB_INST, 0x80U | CDC_EP_NOTIFY);
  usb_ep_int_disable(CDC_USB_INST, CDC_EP_DATA_OUT);
  usb_ep_int_disable(CDC_USB_INST, 0x80U | CDC_EP_DATA_IN);
}

//===========================================
// EP0控制传输处理
//===========================================

/**
 * @brief 通过EP0 IN发送数据
 *
 * @param data 发送数据（不超过 wLength 字节）
 * @param len  实际发送字节数
 */
static void ep0_send(const uint8_t *data, uint32_t len) {
  usb_ep_write(CDC_USB_INST, 0U, data, len);
}

/**
 * @brief EP0发送空包（状态阶段握手）
 */
static void ep0_send_zlp(void) { usb_ep_write(CDC_USB_INST, 0U, NULL, 0U); }

/**
 * @brief 处理SETUP包 — 标准设备请求
 *
 * @param setup 8字节SETUP数据包
 */
static void handle_standard_request(const uint8_t *setup) {
  uint8_t b_request = setup[SETUP_BREQUEST];
  uint8_t w_value_h = setup[SETUP_WVALUE_H];
  uint8_t w_value_l = setup[SETUP_WVALUE_L];
  uint16_t w_length = (uint16_t)setup[SETUP_WLENGTH_L] |
                      ((uint16_t)setup[SETUP_WLENGTH_H] << 8);

  switch (b_request) {
  case USB_REQ_GET_DESCRIPTOR:
    switch (w_value_h) {
    case USB_DESC_TYPE_DEVICE:
      printf("  [EP0] GET_DESCRIPTOR: Device\n");
      {
        uint32_t send_len =
            (w_length < sizeof(s_dev_desc)) ? w_length : sizeof(s_dev_desc);
        ep0_send(s_dev_desc, send_len);
      }
      break;
    case USB_DESC_TYPE_CONFIG:
      printf("  [EP0] GET_DESCRIPTOR: Config\n");
      {
        uint32_t send_len =
            (w_length < sizeof(s_cfg_desc)) ? w_length : sizeof(s_cfg_desc);
        ep0_send(s_cfg_desc, send_len);
      }
      break;
    case USB_DESC_TYPE_QUALIFIER:
      /* 高速设备在全速下的 Device Qualifier，此处 STALL */
      usb_ep_stall_set(CDC_USB_INST, 0U, USB_EP_DIR_IN);
      break;
    case USB_DESC_TYPE_STRING:
      switch (w_value_l) {
      case 0U:
        ep0_send(s_str_lang, (uint32_t)s_str_lang[0]);
        break;
      case 1U:
        ep0_send(s_str_mfr, (uint32_t)s_str_mfr[0]);
        break;
      case 2U:
        ep0_send(s_str_prod, (uint32_t)s_str_prod[0]);
        break;
      case 3U:
        ep0_send(s_str_sn, (uint32_t)s_str_sn[0]);
        break;
      default:
        usb_ep_stall_set(CDC_USB_INST, 0U, USB_EP_DIR_IN);
        break;
      }
      break;
    default:
      usb_ep_stall_set(CDC_USB_INST, 0U, USB_EP_DIR_IN);
      break;
    }
    break;

  case USB_REQ_SET_ADDRESS:
    printf("  [EP0] SET_ADDRESS: %u\n", w_value_l);
    usb_device_set_address(CDC_USB_INST, w_value_l);
    ep0_send_zlp();
    break;

  case USB_REQ_SET_CONFIGURATION:
    printf("  [EP0] SET_CONFIGURATION: %u\n", w_value_l);
    if (w_value_l == 1U) {
      cdc_ep_open();
      s_configured = true;
    } else {
      cdc_ep_close();
      s_configured = false;
    }
    ep0_send_zlp();
    break;

  default:
    printf("  [EP0] 未知标准请求: 0x%02X\n", b_request);
    usb_ep_stall_set(CDC_USB_INST, 0U, USB_EP_DIR_IN);
    break;
  }
}

/**
 * @brief 处理SETUP包 — CDC类请求
 *
 * @param setup 8字节SETUP数据包
 */
static void handle_cdc_request(const uint8_t *setup) {
  uint8_t b_request = setup[SETUP_BREQUEST];
  uint16_t w_length = (uint16_t)setup[SETUP_WLENGTH_L] |
                      ((uint16_t)setup[SETUP_WLENGTH_H] << 8);

  switch (b_request) {
  case CDC_REQ_GET_LINE_CODING:
    printf("  [CDC] GET_LINE_CODING\n");
    {
      uint32_t send_len =
          (w_length < sizeof(s_line_coding)) ? w_length : sizeof(s_line_coding);
      ep0_send(s_line_coding, send_len);
    }
    break;

  case CDC_REQ_SET_LINE_CODING:
    /* 数据在下一个OUT事务中收到，此处先发ZLP，
     * 实际 line coding 更新在 data_out 回调中完成 */
    printf("  [CDC] SET_LINE_CODING（数据在下一OUT事务）\n");
    usb_ep_read_start(CDC_USB_INST, 0U, s_ep0_tx_buf, sizeof(s_line_coding));
    break;

  case CDC_REQ_SET_CONTROL_LINE:
    /* DTR/RTS 控制线状态，无需操作，直接握手 */
    printf("  [CDC] SET_CONTROL_LINE_STATE: 0x%04X\n",
           (uint16_t)setup[SETUP_WVALUE_L] |
               ((uint16_t)setup[SETUP_WVALUE_H] << 8));
    ep0_send_zlp();
    break;

  default:
    printf("  [CDC] 未知类请求: 0x%02X\n", b_request);
    usb_ep_stall_set(CDC_USB_INST, 0U, USB_EP_DIR_IN);
    break;
  }
}

//===========================================
// USB Device 回调函数
//===========================================

/**
 * @brief USB总线复位回调
 *
 * @param inst USB实例
 */
static void on_reset(usb_inst_t inst) {
  (void)inst;
  printf("\n[USB] 总线复位\n");

  /* 关闭已打开端点 */
  if (s_configured) {
    cdc_ep_close();
  }

  s_configured = false;
  s_cdc_tx_pending = false;
  s_cdc_tx_in_flight = false;

  /* 准备EP0 SETUP接收 */
  usb_ep0_out_start(CDC_USB_INST);
}

/**
 * @brief 枚举完成回调
 *
 * @param inst  USB实例
 * @param speed 枚举速度
 */
static void on_enum_done(usb_inst_t inst, usb_speed_t speed) {
  (void)inst;
  s_speed = speed;

  const char *speed_str = (speed == USB_SPEED_FULL) ? "全速(FS)"
                                                      : "低速(LS)";
  printf("[USB] 枚举完成，速度: %s\n", speed_str);

  /* 枚举完成后激活EP0 SETUP接收 */
  usb_activate_setup(CDC_USB_INST);
}

/**
 * @brief SETUP阶段完成回调
 *
 * @param inst USB实例
 * @param buf  8字节SETUP数据包
 */
static void on_setup_stage(usb_inst_t inst, const uint8_t *buf) {
  (void)inst;

  uint8_t bmRequestType = buf[SETUP_BMW_TYPE];
  uint8_t type = (bmRequestType >> 5) & 0x03U; /* 0=标准, 1=类, 2=厂商 */

  if (type == 0x00U) {
    handle_standard_request(buf);
  } else if (type == 0x01U) {
    handle_cdc_request(buf);
  } else {
    printf("[EP0] 不支持的请求类型: 0x%02X\n", bmRequestType);
    usb_ep_stall_set(CDC_USB_INST, 0U, USB_EP_DIR_IN);
  }
}

/**
 * @brief IN端点传输完成回调
 *
 * @param inst   USB实例
 * @param ep_num 端点编号
 */
static void on_data_in(usb_inst_t inst, uint8_t ep_num) {
  (void)inst;

  if (ep_num == CDC_EP_DATA_IN) {
    /* 上一次回环发送完成，重新准备下次接收 */
    s_cdc_tx_in_flight = false;
    printf("  [CDC] TX完成，%u字节已送达PC\n", (unsigned)s_cdc_tx_len);
    usb_ep_read_start(CDC_USB_INST, CDC_EP_DATA_OUT,
                      s_cdc_rx_buf, CDC_RX_BUF_SIZE);
  }
}

/**
 * @brief OUT端点数据接收完成回调
 *
 * @param inst   USB实例
 * @param ep_num 端点编号
 */
static void on_data_out(usb_inst_t inst, uint8_t ep_num) {
  (void)inst;

  if (ep_num == CDC_EP_DATA_OUT) {
    /* 从驱动获取本次实际收到的字节数 */
    s_cdc_rx_count = usb_ep_get_rx_count(CDC_USB_INST, CDC_EP_DATA_OUT);
    s_cdc_tx_pending = true;
    s_cdc_tx_in_flight = false;
  } else if (ep_num == 0U) {
    /* EP0 OUT：收到 SET_LINE_CODING 的数据阶段 */
    memcpy(s_line_coding, s_ep0_tx_buf, sizeof(s_line_coding));
    uint32_t baud =
        (uint32_t)s_line_coding[0] | ((uint32_t)s_line_coding[1] << 8) |
        ((uint32_t)s_line_coding[2] << 16) | ((uint32_t)s_line_coding[3] << 24);
    printf("  [CDC] SET_LINE_CODING: %u bps, %uN%u\n", (unsigned)baud,
           s_line_coding[6], s_line_coding[4] + 1U);
    ep0_send_zlp();
  }
}

/**
 * @brief 挂起回调
 *
 * @param inst USB实例
 */
static void on_suspend(usb_inst_t inst) {
  (void)inst;
  printf("[USB] 总线挂起\n");
}

/**
 * @brief 恢复回调
 *
 * @param inst USB实例
 */
static void on_resume(usb_inst_t inst) {
  (void)inst;
  printf("[USB] 总线恢复\n");
}

/**
 * @brief 连接回调
 *
 * @param inst USB实例
 */
static void on_connect(usb_inst_t inst) {
  (void)inst;
  printf("[USB] 连接检测到VBUS\n");
}

/**
 * @brief 断开回调
 *
 * @param inst USB实例
 */
static void on_disconnect(usb_inst_t inst) {
  (void)inst;
  printf("[USB] 断开连接\n");

  s_configured = false;
  s_cdc_tx_pending = false;
  s_cdc_tx_in_flight = false;
}

//===========================================
// 场景1：CDC虚拟串口回环
//===========================================

/**
 * @brief 场景1：USB CDC虚拟串口（回环通讯）
 *
 * 初始化流程：
 *   1. 注册回调
 *   2. 配置FIFO
 *   3. 初始化USB Device
 *   4. 使能中断
 *   5. 软连接（挂DP上拉）
 *   6. 主循环：将收到的数据原样回送
 */
void scenario_1_cdc_loopback(void) {
  print_separator();
  printf("=== 场景1：USB CDC虚拟串口（回环通讯） ===\n\n");

  /* -------------------------------------------------------
   * 步骤1：注册Device模式回调集合
   * ------------------------------------------------------- */
  printf("步骤1：注册Device模式回调\n");
  static const usb_device_callbacks_t s_dev_cbs = {
      .reset = on_reset,
      .enum_done = on_enum_done,
      .setup_stage = on_setup_stage,
      .data_in = on_data_in,
      .data_out = on_data_out,
      .sof = NULL,
      .suspend = on_suspend,
      .resume = on_resume,
      .connect = on_connect,
      .disconnect = on_disconnect,
  };
  usb_device_callback_set(CDC_USB_INST, &s_dev_cbs);
  printf("  完成\n");

  /* -------------------------------------------------------
   * 步骤2：配置FIFO（必须在 usb_init_device 之后调用）
   * ------------------------------------------------------- */
  printf("\n步骤2：初始化USB Device模式（内置HS PHY，全速）\n");
  static const usb_device_config_t s_dev_cfg = {
      .phy = {
        .type = USB_PHY_EMBEDDED,
        .interface = USB_PHY_IF_16BIT_30MHZ,
        .refclk_12m = true,
        .pll_en = true,
      },
      .speed = USB_SPEED_FULL, /* 使用全速，与大多数主机兼容 */
      .ep_count = 3U,          /* EP1/EP2/EP3 */
      .lpm_enable = false,
      .dma_enable = false, /* FIFO模式，方便调试 */
  };
  usb_init_device(CDC_USB_INST, &s_dev_cfg);
  printf("  USB核心初始化完成\n");

  /* -------------------------------------------------------
   * 步骤3：使能NVIC中断
   * ------------------------------------------------------- */
  printf("\n步骤3：使能USB中断（优先级2）\n");
  NVIC_SetPriority(USB_IRQn, 2U);
  NVIC_EnableIRQ(USB_IRQn);
  printf("  USB_IRQn (13) 已使能\n");

  /* -------------------------------------------------------
   * 步骤4：软连接（使设备对主机可见）
   * ------------------------------------------------------- */
  printf("\n步骤4：软连接（DP上拉）\n");
  usb_device_connect(CDC_USB_INST);
  printf("  已挂DP上拉，等待PC枚举...\n");

  /* -------------------------------------------------------
   * 步骤5：主循环 — 回环转发
   * ------------------------------------------------------- */
  printf("\n步骤5：进入主循环（回环模式）\n");
  printf("  请用PC打开对应COM口，发送任意数据，设备将原样回送\n\n");

  uint32_t frame_cnt = 0U;

  while (true) {
    delay_ms(1);
    frame_cnt++;

    /* 每5秒打印一次心跳 */
    if (frame_cnt % 5000U == 0U) {
      if (s_configured) {
        printf("  [心跳] 已配置，帧号=%u\n",
               (unsigned)usb_get_frame_number(CDC_USB_INST));
      } else {
        printf("  [心跳] 等待枚举...\n");
      }
    }

    /* 有待发送的数据（来自on_data_out），且上一次IN传输已完成 */
    if (s_cdc_tx_pending && !s_cdc_tx_in_flight && s_configured) {
      s_cdc_tx_pending = false;

      uint32_t len = s_cdc_rx_count;  /* 驱动报告的实际接收字节数 */

      if (len == 0U) {
        /* 空包（ZLP），直接重新提交接收 */
        usb_ep_read_start(CDC_USB_INST, CDC_EP_DATA_OUT,
                          s_cdc_rx_buf, CDC_RX_BUF_SIZE);
      } else {
        s_cdc_tx_in_flight = true;
        s_cdc_tx_len = len;

        printf("  [CDC] RX %u字节，回环发送中...\n", (unsigned)len);
        if (len <= 32U) {
          printf("    数据: ");
          print_hex(s_cdc_rx_buf, len);
        }

        /* 发送回环数据；接收重新提交在 on_data_in 回调中完成 */
        usb_ep_write(CDC_USB_INST, CDC_EP_DATA_IN, s_cdc_rx_buf, len);
      }
    }
  }
}

//===========================================
// 入口函数
//===========================================

/**
 * @brief USB CDC示例入口
 *
 * 在 main.c 中调用此函数启动场景。
 *
 * @code
 * // main.c 中：
 * usb_cdc_example_run();
 * @endcode
 */
int main(void) {
  system_init();
  SEGGER_RTT_Init();
  clock_periph_enable(CLK_SYSCFG);
  /* 使能GPIOA时钟 */
  clock_periph_enable(CLK_GPIOA);

  /* PA11 (DM) / PA12 (DP)：AF0，推挽，8mA驱动，施密特触发器使能 */
  /* gpio_set_function 内部已自动设置 MD=AF 模式 */
  gpio_set_function(PA11, GPIO_AF_0);
  gpio_set_drive_strength(PA11, GPIO_DRIVE_8MA);
  gpio_set_schmitt(PA11, true);

  gpio_set_function(PA12, GPIO_AF_0);
  gpio_set_drive_strength(PA12, GPIO_DRIVE_8MA);
  gpio_set_schmitt(PA12, true);
  scenario_1_cdc_loopback();
}
