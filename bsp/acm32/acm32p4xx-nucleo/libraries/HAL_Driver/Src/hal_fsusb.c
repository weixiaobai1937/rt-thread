/***********************************************************************
 * Filename    : hal_fsusb.c
 * Description : fsusb driver source file
 * Author(s)   : xwl  
 * version     : V1.1
 * Modify date : 2021-09-10  
 ***********************************************************************/
#include "hal.h"  

static uint16_t ep1_stall[2]= {0};    // EP1 stall״̬
static uint16_t ep2_stall[2]= {0};    // EP2 stall״̬
static uint16_t ep3_stall[2]= {0};    // EP3 stall״̬
static uint16_t ep4_stall[2]= {0};    // EP4 stall״̬     
static uint16_t ep5_stall[2]= {0};    // EP5 stall״̬  
static uint16_t ep6_stall[2]= {0};    // EP6 stall״̬  


   

__weak uint32_t HAL_FSUSB_MSP_Init(void)  
{  
    return HAL_OK;    
}   

void HAL_USB_Delay(uint32_t delayCount)  
{
    volatile uint32_t cnt; 
    
    cnt = delayCount;   
    while(cnt--);
}

uint32_t HAL_FSUSB_Init(void)  
{
    
    USBCTRL->WORKING_MODE = 0x04;  //disconnect usb/ reset USBC    
	HAL_USB_Delay(3000);   
	USBCTRL->WORKING_MODE = 0x09 ;  //auto reset, fullspeed  
    
    USBCTRL->WORKING_MODE |= BIT26;  //LPM enable  
    
    USBCTRL->EPxCSR[0] |= 1<<8;	//enable EP0  
    USBCTRL->EPxCSR[1] |= 1<<8;	//enable EP1  
    USBCTRL->EPxCSR[2] |= 1<<8;	//enable EP2  
    USBCTRL->EPxCSR[3] |= 1<<8;	//enable EP3  
    USBCTRL->EPxCSR[4] |= 1<<8;	//enable EP4  
    USBCTRL->EPxCSR[5] |= 1<<8;	//enable EP5  
    USBCTRL->EPxCSR[6] |= 1<<8;	//enable EP6  
    USBCTRL->EPADDR_CFG = 0x654321;  
    USBCTRL->USB_STATUS_DETECT_CNT = 0x2A6;  
    
    USBINT->INT_STAT_RAW = 0xFFFFFFFFU; 
    USBINT->INT_EN = BIT0|BIT1|BIT2|BIT4|BIT5|BIT25;   // enable Reset, Suspend, Resume, setup token, setup data, LPM enter L1  
    USBINT->EPINT_EN = BIT5|BIT9|BIT13|BIT17|BIT21|BIT25;   // EP1/EP2/EP3/EP4/EP5/EP6 out data received 
     
	USBCTRL->WORKING_MODE  |= (1<<6)|(1<<4);	//connect   
    
    return HAL_OK;   
}  

uint32_t HAL_FSUSB_Is_USB_Connected(void)
{
    if ( (0 != (USBCTRL->WORKING_MODE & (BIT4|BIT6) ) ) && (0 != USBCTRL->USB_ADDR) )  
    {
        return 1;   
    }
    else
    {
        return 0;      
    }  
}

  
uint32_t HAL_FSUSB_PHY_Suspend(void)
{
    USBCTRL->WORKING_MODE |= BIT1; //PHY Suspend
    USBCTRL->WORKING_MODE &= ~BIT6;  
    HAL_SimpleDelay(15); 
    
    SYSCFG->PHYCFG |= (BIT2); 
    
    return 0;  
}



uint32_t HAL_FSUSB_PHY_Resume(void)
{  
#ifdef USB_PHY_IPGOAL    
    uint32_t delay_count;
    
    delay_count = 10000;   // 25ms max, delay_count is related with HCLK       
    
    if (BIT2 == (SYSCFG->PHYCFG & BIT2) ) 
    {
        SYSCFG->PHYCFG &= (~BIT2);   

        while(delay_count--)
        {
            if (SYSCFG->PHYCFG & (BIT19)) // clksel_end flag = 1  
            {
                break;  
            }
        }          
    }

    if (delay_count) 
    {
        USBCTRL->WORKING_MODE &= (~BIT1); // PHY exit suspend mode 
        HAL_SimpleDelay(15);  
        USBCTRL->WORKING_MODE &= ~(1<<6);   // configure as resume resistor 
        return HAL_OK;   
    }
    else
    {
        return HAL_TIMEOUT;     
    }
        
    return HAL_OK;   
#else
    SYSCFG->PHYCFG &= (~BIT2); 
    USBCTRL->WORKING_MODE &= (~BIT1); // PHY exit suspend mode 
    HAL_SimpleDelay(15);  
    USBCTRL->WORKING_MODE &= ~(1<<6);   // configure as resume resistor    
    return HAL_OK;     
#endif 
}


uint16_t HAL_FSUSB_Get_FIFO_Length(uint8_t ep_index)
{
	return USBCTRL->EPxCSR[ep_index]&0xff;             	
}

void HAL_USB_Clear_FIFO(uint8_t ep_index, uint8_t ep_dir)  
{
	USBCTRL->EPxCSR[ep_index] |= 1<<9;	  
} 

uint16_t HAL_USB_Get_Stall_Status(uint8_t ep_index, uint8_t ep_dir) 
{
	switch(ep_index)
	{
		case USB_EP1: 
		{    
			if(ep_dir == EP_DIR_IN)		return ep1_stall[0];	  //in
			else			  			return ep1_stall[1];	  //out	    
		}
		case USB_EP2: 
		{    
			if(ep_dir == EP_DIR_IN)		return ep2_stall[0];	  //in
			else			  			return ep2_stall[1];	  //out	    
		}
		case USB_EP3: 
		{    
			if(ep_dir == EP_DIR_IN)	  	return ep3_stall[0];	  //in
			else			  			return ep3_stall[1];	  //out	    
		}
		case USB_EP4: 
		{    
			if(ep_dir == EP_DIR_IN)	  	return ep4_stall[0];	  //in
			else			  			return ep4_stall[1];	  //out	    
		}

		default:    	 	  return 0xff;
	}	
}

void usb_clear_stall(uint8_t ep_index, uint8_t ep_dir)
{
	switch(ep_index)
	{
		case USB_EP1: 
		{    
			if(ep_dir == EP_DIR_IN)	 	ep1_stall[0]=0x0000;	  //in
			else			  			ep1_stall[1]=0x0000;	  //out	
			break;  
		}
		case USB_EP2: 
		{    
			if(ep_dir == EP_DIR_IN)	  	ep2_stall[0]=0x0000;	  //in
			else			  			ep2_stall[1]=0x0000;	  //out				  
			break;   
		}
		case USB_EP3: 
		{    
			if(ep_dir == EP_DIR_IN)	  	ep3_stall[0]=0x0000;	  //in
			else			  			ep3_stall[1]=0x0000;	  //out		  
			break;   
		}
		case USB_EP4: 
		{    
			if(ep_dir == EP_DIR_IN)	  	ep4_stall[0]=0x0000;	  //in
			else			  			ep4_stall[1]=0x0000;	  //out					  
			break;    
		}

		default: return;
	}

	USBCTRL->EPxCSR[ep_index] = 0x02100;		   //clear in/out toggle,stall,stall status
	USBCTRL->EPxCSR[ep_index] |= (1<<18)|(1<<15);  //enable change
//	flag_clear_stall=0;  		 
}



void usb_send_stall(uint8_t ep_index, uint8_t ep_dir)
{
	switch(ep_index)
	{
		case USB_EP1: 
		{    
			if(ep_dir == EP_DIR_IN)	 	ep1_stall[0]=0x0001;	  //in
			else			  			ep1_stall[1]=0x0001;	  //out	
			break;  
		}
		case USB_EP2: 
		{    
			if(ep_dir == EP_DIR_IN)	  	ep2_stall[0]=0x0001;	  //in
			else			  			ep2_stall[1]=0x0001;	  //out				  
			break;   
		}
		case USB_EP3: 
		{    
			if(ep_dir == EP_DIR_IN)	  	ep3_stall[0]=0x0001;	  //in
			else			  			ep3_stall[1]=0x0001;	  //out		  
			break;   
		}
		case USB_EP4: 
		{    
			if(ep_dir == EP_DIR_IN)	  	ep4_stall[0]=0x0001;	  //in
			else			  			ep4_stall[1]=0x0001;	  //out					  
			break;    
		}

		default: return;
	}

	USBCTRL->EPxCSR[ep_index] |= (1<<12);    
}


void HAL_FSUSB_Read_EP_MEM8(uint8_t *dst, uint32_t length, uint32_t fifo_offset, uint8_t ep_index)
{
	uint8_t *src;

	src = (uint8_t *)(USB_BASE_ADDR+0x200+(ep_index<<6)+fifo_offset);       
	while(length--)
	{
		*dst++ = *src++;
	}		 
}
 
void HAL_FSUSB_Write_EP_MEM8(uint8_t *src, uint32_t length, uint32_t fifo_offset, uint8_t ep_index)
{

	uint8_t *dst;

	dst = (uint8_t *)(USB_BASE_ADDR+0x200+(ep_index<<6)+fifo_offset); 

	while(length--)
	{
		*dst++ = *src++;
	}
}

//
void HAL_FSUSB_Write_EP_FIFO(uint32_t *src, uint32_t length, uint8_t ep_index)
{  
    uint32_t i, word_length; 
    uint32_t * fifo_entry;  
    
    fifo_entry = (uint32_t *) (0x40050100  + ((ep_index - 0) << 2) );  

    word_length = (length + 3)/4;   
    
    for(i = 0; i < word_length; i++)  
    {
        *fifo_entry = *src; 
        src++;   
    }
    
}

void HAL_FSUSB_Read_EP_FIFO(uint32_t *dst, uint32_t length, uint8_t ep_index)
{
    uint32_t * fifo_entry; 
    uint32_t word_length;     
    uint32_t i; 
    
    fifo_entry = (uint32_t *) (0x40050100  + ((ep_index - 0) << 2) );  

    word_length = (length + 3)/4;   
    
    for(i = 0; i < word_length; i++)  
    {
        *dst = *fifo_entry;    
         dst++;     
    }    
}


#define WAIT_IN_TOKEN   

uint8_t HAL_FSUSB_Start_EP_Transfer(uint32_t length,uint8_t ep_index)
{
	uint8_t sof_cnt;
	
	USBCTRL->EPxSENDBN[ep_index]= length;
    
#ifdef WAIT_IN_TOKEN     	
    HAL_FSUSB_Clear_EP_Interrupt(MASK_EPX_IN(ep_index));       
	while(1)
	{
		// if a new out data packet received, return error to caller  
	    if( (HAL_FSUSB_GET_EP_Interrupt_Status & MASK_EPX_OUT(ep_index)) && (HAL_FSUSB_GET_EP_Interrupt_Status & MASK_EPX_ACK(ep_index)) )
		{
            HAL_FSUSB_Clear_EP_Interrupt(MASK_EPX_OUT(ep_index) | MASK_EPX_ACK(ep_index));  
			return ERROR_OUT_OUT;
		}
		// wait for IN token to start transfer 
        if(HAL_FSUSB_GET_EP_Interrupt_Status & MASK_EPX_IN(ep_index) )
        {
            HAL_FSUSB_Clear_EP_Interrupt(MASK_EPX_IN(ep_index));     
			USBCTRL->WORKING_MODE  |= (1<<11);//return NAK when timeout
			USBCTRL->EPxCSR[ep_index] |= (1<<10);//data is ready for tx
			break;
        }			
	}
    
#else
    HAL_FSUSB_Clear_EP_Interrupt(MASK_EPX_ACK(ep_index) | MASK_EPX_IN(ep_index));       
    HAL_FSUSB_Clear_Global_Interrupt(USB_SOF);  
	USBCTRL->EPxCSR[ep_index] |= EP_SEND_ENABLE;//data is ready for tx
#endif 	

    sof_cnt = 0;  
	while(1)
	{
		if( USBCTRL->EPxCSR[ep_index]& (EP_ACK_RECEIVED) ) //received ACK from host 
		{
            HAL_FSUSB_Clear_EP_Interrupt(MASK_EPX_ACK(ep_index) | MASK_EPX_IN(ep_index));     
            printfS("SendOK\n");
			return 0;//pass
		}

#ifdef WAIT_IN_TOKEN   // If WAIT_IN_TOKEN is not defined, these code should be inactive, orelse there may be issue after device send remote wakeup signal then call this function      
    if(HAL_FSUSB_GET_Global_Interrupt_Status & (USB_SOF))				
		{
            sof_cnt++;  
            HAL_FSUSB_Clear_Global_Interrupt(USB_SOF);  
            if (sof_cnt >= 3)
            {
                USBCTRL->EPxCSR[ep_index] |= EP_STOP_SEND;   
                return ERROR_TIMEOUT;   
            }
		}
#endif    
		if(HAL_FSUSB_GET_EP_Interrupt_Status & MASK_EPX_OUT(ep_index))
		{
            USBCTRL->EPxCSR[ep_index] |= EP_STOP_SEND;   
			return ERROR_IN_OUT; 
		}		
	}	
}

//#define USB_FIFO_MODE  

uint8_t HAL_FSUSB_Send_Data(uint8_t *buffer,uint32_t length,uint8_t ep_index)
{
    uint32_t length_save;  
	uint8_t ret;
    
	length_save = length;  
    
#if 1   
	while(length>=EPX_MAX_PACKET_SIZE)
	{    
#ifndef USB_FIFO_MODE	// when use FIFO Mode, the address of buffer should be word aligned  	
		HAL_FSUSB_Write_EP_MEM8(buffer, EPX_MAX_PACKET_SIZE, 0, ep_index);   
#else          
        HAL_FSUSB_Write_EP_FIFO((uint32_t *)buffer, EPX_MAX_PACKET_SIZE, ep_index);      
#endif        
		ret = HAL_FSUSB_Start_EP_Transfer(EPX_MAX_PACKET_SIZE, ep_index);  
		if(ret == ERROR_OUT_OUT)
		{
		    if( USBCTRL->EPxCSR[ep_index] & ( 1<< 19) )//Toggle error
		    {
		        USBCTRL->EPxCSR[ep_index] ^= (1<<17);  //out toggle want
			    USBCTRL->EPxCSR[ep_index] |= (1<<18);  //update want toggle;			 			             			
		    }
			USBCTRL->EPxCSR[ep_index] |= 1<<11; //set rx ready  	
            continue;	// received a same packet, has processed this packet, just fill respoonse to fifo and send it to host 		
		}
		else if(ret != 0)
		{
			return 1; // send data fail, exit with error code to let caller know 
		}
	 	length -= EPX_MAX_PACKET_SIZE;
	  	buffer += EPX_MAX_PACKET_SIZE;		
	} 
#endif 

	// remaining data, less than EPX_MAX_PACKET_SIZE  
	while(length>0)  
	{        
        HAL_FSUSB_Write_EP_MEM8(buffer,length,0,ep_index);    
        
		ret = HAL_FSUSB_Start_EP_Transfer(length,ep_index);
		if(ret == ERROR_OUT_OUT)
		{
		    if( USBCTRL->EPxCSR[ep_index] & ( 1<< 19) )//Toggle error
		    {
		        USBCTRL->EPxCSR[ep_index] ^= (1<<17); //out toggle want
			    USBCTRL->EPxCSR[ep_index] |= (1<<18); //update want toggle;			 			             			
		    }
			USBCTRL->EPxCSR[ep_index] |= 1<<11; //set rx ready  
            continue;				
		}
		else if(ret != 0)
		{
			return 1; // send data fail, exit with error code to let caller know   
		}
	 	length -= length;
	  	buffer += length;
	}
    
    // add by fw, 20220614  
    if (0 == ep_index) 
    {
        if (0 == (length_save % EP0_MAX_PACKET_SIZE))
        {
            HAL_FSUSB_EP0_Send_Empty_Packet();  
        }
    }
	
	return 0;  
}

/*=================================================================================================================================
  * @brief  receive length of data and save them into buffer by ep_index 
  * @param  buffer, address of destination      
  * @param  length, length of data to receive, if you don't know the specific value, you can use max vuale of one packet=64     
  * @param  ep_index, index of endpoint 
  * @param  single_packet, 0, receive wanted data as length; other value, receive one packet  
  * @retval actual length of readout; if the high 16 bits is greater than low 16 bits, it indicates error.     
=====================================================================================================================================*/        
uint32_t HAL_FSUSB_Receive_Data(uint8_t *buffer,uint32_t length,uint8_t ep_index, uint8_t single_packet)  
{
	volatile uint32_t fifo_len;
    volatile uint32_t read_length;  
    
    read_length = length;  
    
	while(read_length > 0)	 
	{			        
        while (0 == (USBCTRL->EPxCSR[ep_index] & EP_DATA_RECEIVED)) // out valid 
        {
            // wait 
        }

        HAL_FSUSB_Clear_EP_Interrupt(MASK_EPX_OUT(ep_index) | MASK_EPX_ACK(ep_index));    
		USBCTRL->EPxCSR[ep_index] |= EP_DATA_RECEIVED; // clear out valid 
        
		if( USBCTRL->EPxCSR[ep_index] & ( 1<< 19) )//Toggle error 
		{
		    USBCTRL->EPxCSR[ep_index] ^= (1<<17); //out toggle want
			USBCTRL->EPxCSR[ep_index] |= (1<<18); //update want toggle; 
			USBCTRL->EPxCSR[ep_index] |= 1<<11;   //set rx ready, wait for a new packet 
			continue;  //discard this packet          			
		}		
	
		fifo_len = HAL_FSUSB_Get_FIFO_Length(ep_index);
        if (read_length < fifo_len)
        {
            return ((fifo_len << 16) | read_length);     // error, app should read all data saved in fifo    
        }     
#ifndef USB_FIFO_MODE	 // when use FIFO Mode, the address of buffer should be word aligned  	          
		HAL_FSUSB_Read_EP_MEM8(buffer, fifo_len, 0, ep_index);	 
#else        
        HAL_FSUSB_Read_EP_FIFO( (uint32_t *)buffer, fifo_len, ep_index);   
#endif                 
		read_length -= fifo_len;
		buffer += fifo_len;  
        
		USBCTRL->EPxCSR[ep_index] |= EP_RECEIVE_READY; //set rx ready to wait next packet 
        if (single_packet)
        {
            return fifo_len;   
        }
        
	}	

    return length;   
} 


//ep_index��ʾ�˵���
void HAL_FSUSB_EP0_Send_Empty_Packet(void)
{
	HAL_FSUSB_Start_EP_Transfer(0,USB_EP0);    
} 
  
void HAL_FSUSB_EP0_Send_Stall(void)
{
	USBCTRL->EPxCSR[0] |= (1<<12);
	while(!(USBCTRL->EPxCSR[0] &0x2000));
	USBCTRL->EPxCSR[0] |= 0x2000;    
} 

void HAL_FSUSB_Enable_Global_Interrupt(uint32_t Interrupt)
{
    USBINT->INT_EN |= Interrupt;   
}

void HAL_FSUSB_Disable_Global_Interrupt(uint32_t Interrupt)
{
    USBINT->INT_EN &= (~Interrupt);     
}  

void HAL_FSUSB_Enable_EP_Interrupt(uint32_t Interrupt)
{
    USBINT->EPINT_EN |= Interrupt;   
}

void HAL_FSUSB_Disable_EP_Interrupt(uint32_t Interrupt)
{
    USBINT->EPINT_EN &= (~Interrupt);      
} 

HAL_StatusTypeDef HAL_FSUSB_Check_Out_Packet(uint32_t ep_index)
{
    if(USBCTRL->EPxCSR[ep_index] & ( 1<< 19) )//Toggle error 
    {
        USBCTRL->EPxCSR[ep_index] ^= (1<<17); //out toggle want
        USBCTRL->EPxCSR[ep_index] |= (1<<18); //update want toggle; 
        USBCTRL->EPxCSR[ep_index] |= 1<<11;   //set rx ready, wait for a new packet    
        
        return HAL_ERROR; //not process this frame. device has sent response before     				
    }		
    
    return HAL_OK;    
}  


void HAL_FSUSB_Bus_Reset(void)  
{
    USBCTRL->WORKING_MODE &= ~(1<<6);	//Res:transmit

	if(!(USBCTRL->WORKING_MODE&0x08)) 	  
	{
		USBCTRL->WORKING_MODE  |=1<<2;
		USBCTRL->WORKING_MODE  &=~(1<<2);    
	}	

	USBCTRL->EPxCSR[1] = 0x02100;			 //clear in/out toggle,stall,stall status
	USBCTRL->EPxCSR[1]  |= (1<<18)|(1<<15); //enable change
	USBCTRL->EPxCSR[2]  = 0x02100;	
	USBCTRL->EPxCSR[2] |= (1<<18)|(1<<15);
	USBCTRL->EPxCSR[3]  = 0x02100;
	USBCTRL->EPxCSR[3] |= (1<<18)|(1<<15);
	USBCTRL->EPxCSR[4] = 0x02100;
	USBCTRL->EPxCSR[4] |= (1<<18)|(1<<15);   
    USBCTRL->EPxCSR[5] = 0x02100;
	USBCTRL->EPxCSR[5] |= (1<<18)|(1<<15);  
    USBCTRL->EPxCSR[6] = 0x02100;
	USBCTRL->EPxCSR[6] |= (1<<18)|(1<<15);  
    
    USBCTRL->WORKING_MODE |= BIT26;  // enable LPM Feature  
    
    
}    


void HAL_FSUSB_Suspend(void)  
{
    USBCTRL->WORKING_MODE |= (1<<6)|(1<<4);	//Res:idle     
}  

void HAL_FSUSB_Resume(void)  
{
    USBCTRL->WORKING_MODE &= ~(1<<6);	//Res:transmit	       
}


void HAL_FSUSB_Send_Remote_Wakeup_Signal(void)  
{
    USBCTRL->WORKING_MODE |= (1UL<<20);	
    HAL_USB_Delay(6072);                       // at least 1ms    
    USBCTRL->WORKING_MODE &= (~(1UL<<20) );	        
}






