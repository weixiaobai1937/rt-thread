/*
  ******************************************************************************
  * @file    HAL_Uart.h
  * @version V1.0.0
  * @date    2020
  * @brief   Header file of UART HAL module.
  ******************************************************************************
*/
#ifndef __HAL_FSUSB_H__
#define __HAL_FSUSB_H__  

#include "hal.h"


#define     HIGH_SPEED     1 
#define     FULL_SPEED     0 

#define     USB_EP0        0
#define     USB_EP1        1        
#define     USB_EP2        2      
#define     USB_EP3        3        
#define     USB_EP4        4   
#define     USB_EP5        5        
#define     USB_EP6        6     
#define		MAX_USB_EP_NUM 7

#define     EP_DIR_IN      0x80  
#define     EP_DIR_OUT     0x00   


#define     EP0_MAX_PACKET_SIZE     64
#define     EPX_MAX_PACKET_SIZE     64


#define     MASK_EPX_IN(x)		(1<<(0+4*x))   
#define     MASK_EPX_OUT(x)     (1<<(1+4*x))   
#define     MASK_EPX_ACK(x)     (1<<(2+4*x)) 
#define     MASK_EPX_TIMEOUT(x) (1<<(3+4*x))    

typedef __PACKED_STRUCT _device_request
{
	uint8_t  bmRequestType;
	uint8_t  bRequest;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
} DEVICE_REQUEST;

typedef struct _Usb_Desc_Detail_t
{
    uint8_t	DescType;		//descriptor type
	uint8_t	DescSubType;	//reserved

	uint16_t DescLen;
	uint8_t	*Descriptor;
} Usb_Desc_Detail_t;


typedef struct _Usb_Cfg_Info_t
{
	uint16_t interface;		//for record set interface altsetting
	uint16_t AltSetting;

	uint8_t config_value;
	uint8_t usb_dev_status;  // 远程唤醒和自供电状态	//Bit0-Self powered  Bit1-Remote wakeup
	uint8_t usb_ep_status[MAX_USB_EP_NUM];	//Bit0-In endpoint halt status   Bit4-Out endpoint halt status

	Usb_Desc_Detail_t *desc_tbl;
} Usb_Cfg_Info_t;

typedef void (*Usb_Proc_t)(void *husb);

typedef struct _Usb_Handle_t
{
	//Control and interrupt register
	USB_CTRLTypeDef	*usb_ctrl_reg;
	USB_INTTypeDef	*usb_int_reg;

	//
	Usb_Cfg_Info_t	usb_cfg;
	DEVICE_REQUEST	dev_req;

	//Interrupt process function table of each endpoint
	Usb_Proc_t	*usb_int_proc_cb;

	//User command process function table of each endpoint
	Usb_Proc_t	*usb_user_proc_cb;

	uint32_t	flg_usb_epx_int[MAX_USB_EP_NUM];
	uint8_t	cur_op_in_ep;
	uint8_t	cur_op_out_ep;

	void	*usr_dat;
} Usb_Handle_t;


#define IPGOAL_USB_PHY  
//#define PDI_USB_PHY  

#define USB_BUS_RESET         	0x01
#define USB_SUSPEND           	0x02
#define USB_RESUME           	0x04
#define USB_SOF            		0x08
#define USB_SETUPTOK         	0x10
#define USB_EP0_SETUP_PACKET  	0x20
#define USB_IN_TIMEOUT	    	0x200000
#define USB_SETADDR		    	0x400000
#define USB_CRC_ERR 	    	0x800000
#define USB_MORETHAN_64     	0x1000000  
#define USB_LPM_ENTER_L1        BIT25 
#define USB_NOEOP_ERR      		0x40000000
#define USB_TOGGLE_ERR      	0x80000000  

#define USB_EP0_OUT_PACKET   (BIT1)  
#define USB_EP1_OUT_PACKET   (BIT5)  
#define USB_EP2_OUT_PACKET   (BIT9)  
#define USB_EP3_OUT_PACKET   (BIT13)  
#define USB_EP4_OUT_PACKET   (BIT17)  
#define USB_EP5_OUT_PACKET   (BIT21)  
#define USB_EP6_OUT_PACKET   (BIT25)   

#define USB_EP0_IN_TOKEN   (BIT0)  
#define USB_EP1_IN_TOKEN   (BIT4)  
#define USB_EP2_IN_TOKEN   (BIT8)  
#define USB_EP3_IN_TOKEN   (BIT12)  
#define USB_EP4_IN_TOKEN   (BIT16)  
#define USB_EP5_IN_TOKEN   (BIT20)  
#define USB_EP6_IN_TOKEN   (BIT24)   

#define EP_ENABLE         BIT8 
#define EP_SEND_ENABLE    BIT10   
#define EP_RECEIVE_READY  BIT11   
#define EP_DATA_RECEIVED  BIT20   
#define EP_ACK_RECEIVED   BIT24    
#define EP_STOP_SEND      BIT27       

#define ERROR_TIMEOUT            8  // received a same out packet 
#define ERROR_OUT_OUT            4  // received a same out packet 
#define ERROR_IN_OUT             2  // received a pakcet when try to send packet    

#define HAL_FSUSB_Clear_Global_Interrupt(x)        (USBINT->INT_STAT_RAW = (x) )  
#define HAL_FSUSB_GET_Global_Interrupt_Status      (USBINT->INT_STAT_RAW)    
#define HAL_FSUSB_GET_EP_Interrupt_Status          (USBINT->EP_INT_STAT_RAW)  
#define HAL_FSUSB_Clear_EP_Interrupt(x)            (USBINT->EP_INT_STAT_RAW = (x) )  
#define HAL_FSUSB_GET_IN_EP_SENT_Interrupt_Status  (USBINT->EP_INT2_STAT_RAW)  
#define HAL_FSUSB_Clear_IN_EP_SENT_Interrupt(x)    (USBINT->EP_INT2_STAT_RAW = (x) ) 

uint32_t HAL_FSUSB_Init(void);     
void HAL_FSUSB_Read_EP_MEM8(uint8_t *dst, uint32_t length, uint32_t fifo_offset, uint8_t ep_index);  
uint8_t HAL_FSUSB_Send_Data(uint8_t *buffer,uint32_t length,uint8_t ep_index);   
uint32_t HAL_FSUSB_Receive_Data(uint8_t *buffer,uint32_t length,uint8_t ep_index, uint8_t single_packet);  
uint16_t HAL_USB_Get_Stall_Status(uint8_t ep_index, uint8_t ep_dir);  
void HAL_FSUSB_EP0_Send_Empty_Packet(void);  
void HAL_FSUSB_EP0_Send_Stall(void);    
void usb_clear_stall(uint8_t ep_index, uint8_t ep_dir);  
void usb_send_stall(uint8_t ep_index, uint8_t ep_dir);
uint16_t HAL_FSUSB_Get_FIFO_Length(uint8_t ep_index);     

void HAL_FSUSB_Enable_Global_Interrupt(uint32_t Interrupt);    
void HAL_FSUSB_Disable_Global_Interrupt(uint32_t Interrupt);  
void HAL_FSUSB_Enable_EP_Interrupt(uint32_t Interrupt);  
void HAL_FSUSB_Disable_EP_Interrupt(uint32_t Interrupt); 

HAL_StatusTypeDef HAL_FSUSB_Check_Out_Packet(uint32_t ep_index);   

uint32_t HAL_FSUSB_MSP_Init(void); 

void HAL_FSUSB_Bus_Reset(void);  

void HAL_FSUSB_Suspend(void);      

void HAL_FSUSB_Resume(void);     
uint32_t HAL_FSUSB_Is_USB_Connected(void);  
uint32_t HAL_FSUSB_PHY_Suspend(void);  
uint32_t HAL_FSUSB_PHY_Resume(void);  

void HAL_FSUSB_Send_Remote_Wakeup_Signal(void);    

#endif  

