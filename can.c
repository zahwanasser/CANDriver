/*
 * File name: can.c
 *
 *  Created on: Sep 5, 2021
 *      Author: Zahwa Nasser
 */
#include "can.h"
/*******************************************************************************
 *                      Functions Definitions                                  *
 *******************************************************************************/
/*
 * Description : Function to initialize the CAN driver
 *  1. configure the GPIO resgisters
 *     enable the clk to the port, set the alternative function, set the peripheral
 *     disable analog function and enable digital function.
 *  2.  Set the CANBIT register which contains of:
 *      1. baud rate prescalar
 *      2. tTSeg1 = tProp + tPhase1-1
 *      3. tTSeg2 = tPhase2-1
 *      4. sjw= sjw-1
 *  3. enable interrupts
 *
 *  Arguments: pointer to structure holding the required info
 *  Returns: returns false if the calculated values are not in the right range
 *           returns true if else
 */
bool can_init(const can_configStruct* configPtr)
{
    float32 bitTime, tq;
    uint8 baudRatePrescalar, m_tprop,  tphase, tphase1, tphase2, tsync, TSEG1, TSEG2, tSJW=4;
    //calculating bit time
    bitTime= 1/(configPtr->bitRate);
    //quantum time calculation
    tq= bitTime/(configPtr->n);
    //baud rate calculation
    baudRatePrescalar=tq*(configPtr->Fsys);
    //calculating tsync
    tsync=1;
    //calculating how many quantas is the delay
    m_tprop=round((configPtr->delays)/tq);
    //calculating how many quantas in tphase
    tphase=(configPtr->n)-m_tprop-1;
    if(tphase%2==0) //tphase is an even multiple of tq
    {
        tphase1=tphase/2;
        tphase2=tphase1;
    }
    else
    {
        tphase1=tphase/2;
        tphase2=tphase1+1;
    }
    //calculating values for CANBIT register
    TSEG1=m_tprop+tphase1-tsync;
    TSEG2=tphase2-1;
    //i need to make sure that tSJW value is less than phase 1 and phase 2
    while (tSJW> tphase1 || tSJW > tphase2)
    {
        tSJW--;
    }
    tSJW=tSJW-1;
    baudRatePrescalar= baudRatePrescalar-1;
    //check if the calculated values are in the correct range
    if (tSJW <0 || tSJW >=4 || tphase1 <1 || tphase1 >8 || tphase2 <1 || tphase2 >8 ||
            m_tprop <1 || m_tprop >8)
    {
        return FALSE;
    }

    if (configPtr->module==0)
    {   //initialize clk for can0
        SYSCTL_RCGC0_R |= SYSCTL_RCGC0_CAN0;

        //GPIO Configuration
        //for CAN0 we will use port B
        //first we need to enable the clk for the port
        SYSCTL_RCGC2_R |=SYSCTL_RCGC2_GPIOB;
        //set the alternate function
        GPIO_PORTB_AFSEL_R |=0x30;
        //set the function to can0
        GPIO_PORTB_PCTL_R &=~0X00FF0000;
        GPIO_PORTB_PCTL_R |=0X00880000;
        //disable the analog function and enable the digital function
        GPIO_PORTB_AMSEL_R &= ~0x30;
        GPIO_PORTB_DEN_R |= 0x30;
        // set init to enter initialization state and CCE bits to access CANBIT register
        //set CANBRPE register to be able to configure the baud rate prescalar
        CAN0_CTL_R |=0x41;
        CAN0_BRPE_R=0XF;
        //setting the values in CANBIT register
        CAN0_BIT_R |=baudRatePrescalar;
        CAN0_BIT_R |=tSJW<<6;
        CAN0_BIT_R |=TSEG1<<8;
        CAN0_BIT_R |=TSEG2<<12;

        //clear init bit to exit initialization state
        CAN0_CTL_R &=~0x41;
        //ENABLE INTERRUPTS
        CAN0_CTL_R |=0x06;

    }
    if (configPtr ->module==1)
    {
        //initialize clk for can1
        SYSCTL_RCGC0_R |= SYSCTL_RCGC0_CAN1;
        //GPIO configuration
        //first we need to enable the clk for the port
        SYSCTL_RCGC2_R |=SYSCTL_RCGC2_GPIOA;
        //set the alternate function
        GPIO_PORTA_AFSEL_R |=0x03;
        //set the function to can0
        GPIO_PORTA_PCTL_R &=~0X000000FF;
        GPIO_PORTA_PCTL_R |=0X00000088;
        //diable the analog function and enable the digital function
        GPIO_PORTA_AMSEL_R &= ~0x03;
        GPIO_PORTA_DEN_R |= 0x03;
        // set init and CCE bits to access CANBIT register
        //set CANBRPE register to be able to configure the baud rate prescalar
        CAN1_CTL_R |=0x41;
        CAN1_BRPE_R=0XF;
        //setting the values in CANBIT register
        CAN1_BIT_R |=baudRatePrescalar;
        CAN1_BIT_R |=tSJW;
        CAN1_BIT_R |=TSEG1;
        CAN1_BIT_R |=TSEG2;
        //clear init bit to exit initialization state
        CAN1_CTL_R &=~0x41;
 //ENABLE INTERRUPTS
        CAN1_CTL_R |=0x06;


    }
  
    return TRUE;


}
/*
 * Description : Function to transmit data objects
 *
 *  Arguments: pointer to structure holding the required info which are:
 *                  can_Interface interface; //CANIF1 or CANIF2
                    can_Module module; //can0 or can1
                    can_frameType frameType; //DATA OR REMOTE
                    can_IdType ID_type;    //normal or extended
                    uint32 ID_mask; //mask for acceptance filtering
                    uint32 ID; //ID OF THE MESSAGE
                    uint8 bytesNum; //no of bytes to be sent
                    uint64 Data; //data to be sent
                    uint8 messageNum; //data object number in the can ram
 *  Returns: void
 */
void can_transmit(const can_transmitStruct* transmitPtr)
{
    if (transmitPtr->module == 0) //CAN0
    {
        if(transmitPtr->interface == 1) //CANIF1

        {
            while(CAN0_IF1CRQ_R & 0x00008000); //wait while can0-if1 is busy

            CAN0_IF1CMSK_R |=0XF3; /* WRNRD=1 to write in the register
                                      MASK, ARB, CONTROL, DATA A, DATA B=1 to
                                      to transfer them to the interface registers to be
                                      able to access them by the cpu */
            //in order for bits to be used for acceptance filtering we have to enable

            // CAN0_IF1MCTL_R |=0x1000;
            if(transmitPtr->ID_type == 0) // 11 bit id
            {   /* if we use 11 bit id we have to shift the value by 2 because
                    the value is stored in bits 2:12 in the following registers */
                CAN0_IF1MSK2_R |= (transmitPtr->ID_mask)<<2; // acceptance filter mask
                CAN0_IF1ARB2_R  |=(transmitPtr->ID)<<2;      // message id
                CAN0_IF1ARB2_R  |= 0xA000;      //to set direction of transmission and validation bit
                CAN0_IF1ARB2_R &=~0x4000;       //clear the xtd bit


            }
            else //29 bit id
            {   /* for 29 bit id, the first register stores the first 16 bit of the id mask or id
                    and the other register stores the rest so we have to shift the other bits */
                CAN0_IF1MSK1_R |= (transmitPtr->ID_mask);
                CAN0_IF1MSK2_R |= (transmitPtr->ID_mask)>>16;
                CAN0_IF1ARB1_R  |=(transmitPtr->ID);
                CAN0_IF1ARB2_R  |=(transmitPtr->ID)>>16;
                CAN0_IF1ARB2_R  |= 0xE000; //set the direction, validation, extended frame bit.

            }
            CAN0_IF1MCTL_R |= 0x3080; //enable interrupt for this message object, not using fifo, unmask bit
            CAN0_IF1MCTL_R |= (transmitPtr->bytesNum); //the number of bytes to be transmitted
            //  while (!( CAN0_IF1MCTL_R & 0x0100); //if no message request
            CAN0_IF1DA1_R |= transmitPtr->Data;     //first 2 bytes of data
            if(transmitPtr->bytesNum > 2 )
            {
                CAN0_IF1DA2_R |= (transmitPtr->Data)>>16;
            }
            if(transmitPtr->bytesNum > 4 )
            {
                CAN0_IF1DB1_R |= (transmitPtr->Data)>>16;
            }
            if(transmitPtr->bytesNum > 6)
            {
                CAN0_IF1DB2_R |= (transmitPtr->Data)>>16;
            }
            if(transmitPtr->frameType == 1) //remote frame
            {
                CAN0_IF1MCTL_R |= 0x0200; //when receiving a remote frame start transmitting automatically
            }
            CAN0_IF1CRQ_R |= transmitPtr->messageNum; //select the message num in the can ram
            CAN0_IF1MCTL_R |= 0x0100; //set the TXRQST bit to start transmission
        }
        else if(transmitPtr->interface == 2)

        {
            while(CAN0_IF2CRQ_R & 0x00008000); //wait while can0-if1 is busy

            CAN0_IF2CMSK_R |=0XF3;
            //in order for these bits to be used for acceptance filtering
            //CAN0_IF2MCTL_R |=0x1000;
            if(transmitPtr->ID_type == 0) // 11 bit id
            {
                CAN0_IF2MSK2_R |= (transmitPtr->ID_mask)<<2;
                CAN0_IF2ARB2_R  |=(transmitPtr->ID)<<2;
                CAN0_IF2ARB2_R  |= 0xA000; //to set direction of transmission and validation bit
                CAN0_IF2ARB2_R &=~0x4000; //clear the xtd bit


            }
            else //29 bit id
            {
                CAN0_IF2MSK1_R |= (transmitPtr->ID_mask);
                CAN0_IF2MSK2_R |= (transmitPtr->ID_mask)>>16;
                CAN0_IF2ARB1_R  |=(transmitPtr->ID);
                CAN0_IF2ARB2_R  |=(transmitPtr->ID)>>16;
                CAN0_IF2ARB2_R  |= 0xE000;

            }
            CAN0_IF2MCTL_R |= 0x3080;
            CAN0_IF2MCTL_R |= (transmitPtr->bytesNum);
            //  while (!( CAN0_IF1MCTL_R & 0x0100); //if no message request
            CAN0_IF2DA1_R |= transmitPtr->Data;
            if(transmitPtr->bytesNum > 2 )
            {
                CAN0_IF2DA2_R |= (transmitPtr->Data)>>16;
            }
            if(transmitPtr->bytesNum > 4 )
            {
                CAN0_IF2DB1_R |= (transmitPtr->Data)>>16;
            }
            if(transmitPtr->bytesNum > 6)
            {
                CAN0_IF2DB2_R |= (transmitPtr->Data)>>16;
            }
            if(transmitPtr->frameType == 1)
            {
                CAN0_IF1MCTL_R |= 0x0200;
            }
            CAN0_IF2CRQ_R |= transmitPtr->messageNum;
            CAN0_IF2MCTL_R |= 0x0100;
        }



    }
    else  if (transmitPtr->module == 1)
    {
        if(transmitPtr->interface == 1)

        {
            while(CAN1_IF1CRQ_R & 0x00008000); //wait while can0-if1 is busy

            CAN1_IF1CMSK_R |=0XF3;
            //in order for these bits to be used for acceptance filtering
            //CAN1_IF1MCTL_R |=0x1000;
            if(transmitPtr->ID_type == 0) // 11 bit id
            {
                CAN1_IF1MSK2_R |= (transmitPtr->ID_mask)<<2;
                CAN1_IF1ARB2_R  |=(transmitPtr->ID)<<2;
                CAN1_IF1ARB2_R  |= 0xA000; //to set direction of transmission and validation bit
                CAN1_IF1ARB2_R &=~0x4000; //clear the xtd bit


            }
            else //29 bit id
            {   CAN1_IF1MSK1_R |= (transmitPtr->ID_mask);
            CAN1_IF1MSK2_R |= (transmitPtr->ID_mask)>>16;
            CAN1_IF1ARB1_R  |=(transmitPtr->ID);
            CAN1_IF1ARB2_R  |=(transmitPtr->ID)>>16;
            CAN1_IF1ARB2_R  |= 0xE000;

            }
            CAN1_IF1MCTL_R |= 0x3080;
            CAN1_IF1MCTL_R |= (transmitPtr->bytesNum);
            //  while (!( CAN0_IF1MCTL_R & 0x0100); //if no message request
            CAN1_IF1DA1_R |= transmitPtr->Data;
            if(transmitPtr->bytesNum > 2 )
            {
                CAN1_IF1DA2_R |= (transmitPtr->Data)>>16;
            }
            if(transmitPtr->bytesNum > 4 )
            {
                CAN1_IF1DB1_R |= (transmitPtr->Data)>>16;
            }
            if(transmitPtr->bytesNum > 6)
            {
                CAN1_IF1DB2_R |= (transmitPtr->Data)>>16;
            }
            if(transmitPtr->frameType == 1)
            {
                CAN0_IF1MCTL_R |= 0x0200;
            }
            CAN1_IF1CRQ_R |= transmitPtr->messageNum;
            CAN1_IF1MCTL_R |= 0x0100;
        }
        else if(transmitPtr->interface == 2)

        {
            while(CAN1_IF2CRQ_R & 0x00008000); //wait while can0-if1 is busy

            CAN1_IF2CMSK_R |=0XF3;
            //in order for these bits to be used for acceptance filtering
            // CAN1_IF2MCTL_R |=0x1000;
            if(transmitPtr->ID_type == 0) // 11 bit id
            {
                CAN1_IF2MSK2_R |= (transmitPtr->ID_mask)<<2;
                CAN1_IF2ARB2_R  |=(transmitPtr->ID)<<2;
                CAN1_IF2ARB2_R  |= 0xA000; //to set direction of transmission and validation bit
                CAN1_IF2ARB2_R &=~0x4000; //clear the xtd bit


            }
            else //29 bit id
            {   CAN1_IF2MSK1_R |= (transmitPtr->ID_mask);
            CAN1_IF2MSK2_R |= (transmitPtr->ID_mask)>>16;
            CAN1_IF2ARB1_R  |=(transmitPtr->ID);
            CAN1_IF2ARB2_R  |=(transmitPtr->ID)>>16;
            CAN1_IF2ARB2_R  |= 0xE000;

            }
            CAN1_IF2MCTL_R |= 0x3080;
            CAN1_IF2MCTL_R |= (transmitPtr->bytesNum);
            //  while (!( CAN0_IF1MCTL_R & 0x0100); //if no message request
            CAN1_IF2DA1_R |= transmitPtr->Data;
            if(transmitPtr->bytesNum > 2 )
            {
                CAN1_IF2DA2_R |= (transmitPtr->Data)>>16;
            }
            if(transmitPtr->bytesNum > 4 )
            {
                CAN1_IF2DB1_R |= (transmitPtr->Data)>>16;
            }
            if(transmitPtr->bytesNum > 6)
            {
                CAN1_IF2DB2_R |= (transmitPtr->Data)>>16;
            }
            if(transmitPtr->frameType == 1)
            {
                CAN0_IF1MCTL_R |= 0x0200;
            }
            CAN1_IF2CRQ_R |= transmitPtr->messageNum;
            CAN1_IF2MCTL_R |= 0x0100;
        }



    }
}
/*
 * Description : Function to update existing data objects
 *
 *  Arguments: pointer to structure holding the required info which are:
 *                  can_Interface interface;
                    can_Module module; //can0 or can1
                    uint8 bytesNum;//bytes that need to be updated
                    uint64 Data;
                    uint8 messageNum;
     Returns: void
 */
void can_updateMessage(const can_updateStruct* updatePtr)
{
    if(updatePtr->module == 0)
    {
        if(updatePtr->interface == 1)
        {
            while(CAN0_IF1CRQ_R & 0x00008000); //wait while can0-if1 is busy

            CAN0_IF1CMSK_R |=0X83; //set wren, dataa, datab bits
            CAN0_IF1DA1_R |= updatePtr->Data; //update data
            if(updatePtr->bytesNum > 2 )
            {
                CAN0_IF1DA2_R |= (updatePtr->Data)>>16;
            }
            if(updatePtr->bytesNum > 4 )
            {
                CAN0_IF1DB1_R |= (updatePtr->Data)>>16;
            }
            if(updatePtr->bytesNum > 6)
            {
                CAN0_IF1DB2_R |= (updatePtr->Data)>>16;
            }

            CAN0_IF1CRQ_R |= updatePtr->messageNum; //select the message num in the can ram
            CAN0_IF1MCTL_R |= 0x0100; //set the TXRQST bit to start transmission
        }
        else
        {
            while(CAN0_IF2CRQ_R & 0x00008000); //wait while can0-if1 is busy

            CAN0_IF2CMSK_R |=0X83;
            CAN0_IF2DA1_R |= updatePtr->Data;
            if(updatePtr->bytesNum > 2 )
            {
                CAN0_IF2DA2_R |= (updatePtr->Data)>>16;
            }
            if(updatePtr->bytesNum > 4 )
            {
                CAN0_IF2DB1_R |= (updatePtr->Data)>>16;
            }
            if(updatePtr->bytesNum > 6)
            {
                CAN0_IF2DB2_R |= (updatePtr->Data)>>16;
            }

            CAN0_IF2CRQ_R |= updatePtr->messageNum; //select the message num in the can ram
            CAN0_IF2MCTL_R |= 0x0100; //set the TXRQST bit to start transmission
        }
    }
    if(updatePtr->module == 1)
    {
        if(updatePtr->interface == 1)
        {
            while(CAN1_IF1CRQ_R & 0x00008000); //wait while can0-if1 is busy

            CAN1_IF1CMSK_R |=0X83;
            CAN1_IF1DA1_R |= updatePtr->Data;
            if(updatePtr->bytesNum > 2 )
            {
                CAN1_IF1DA2_R |= (updatePtr->Data)>>16;
            }
            if(updatePtr->bytesNum > 4 )
            {
                CAN1_IF1DB1_R |= (updatePtr->Data)>>16;
            }
            if(updatePtr->bytesNum > 6)
            {
                CAN1_IF1DB2_R |= (updatePtr->Data)>>16;
            }

            CAN0_IF1CRQ_R |= updatePtr->messageNum; //select the message num in the can ram
            CAN1_IF1MCTL_R |= 0x0100; //set the TXRQST bit to start transmission
        }
        else
        {
            while(CAN1_IF2CRQ_R & 0x00008000); //wait while can0-if1 is busy

            CAN1_IF2CMSK_R |=0X83;
            CAN1_IF2DA1_R |= updatePtr->Data;
            if(updatePtr->bytesNum > 2 )
            {
                CAN1_IF2DA2_R |= (updatePtr->Data)>>16;
            }
            if(updatePtr->bytesNum > 4 )
            {
                CAN1_IF2DB1_R |= (updatePtr->Data)>>16;
            }
            if(updatePtr->bytesNum > 6)
            {
                CAN1_IF2DB2_R |= (updatePtr->Data)>>16;
            }

            CAN1_IF2CRQ_R |= updatePtr->messageNum; //select the message num in the can ram
            CAN1_IF2MCTL_R |= 0x0100; //set the TXRQST bit to start transmission
        }
    }
}
/*
 * Description : Function to configure received data objects
 *
 *  Arguments: pointer to structure holding the required info which are:
 *                  can_Interface interface; //CANIF1 or CANIF2
                    can_Module module; //can0 or can1
                    can_IdType ID_type;    //normal or extended
                    uint32 ID_mask; //mask for acceptance filtering
                    uint32 ID; //ID OF THE MESSAGE
                    uint8 bytesNum; //no of bytes to be sent
                    uint64 Data; //data to be sent
                    uint8 messageNum; //data object number in the can ram
 *  Returns: void
 */
void can_receive(const can_receiveStruct* receivePtr)
{
    if (receivePtr->module == 0) //CAN0
    {
        if(receivePtr->interface == 1) //CANIF1

        {
            while(CAN0_IF1CRQ_R & 0x00008000); //wait while can0-if1 is busy

            CAN0_IF1CMSK_R |=0XF3; /* WRNRD=1 to write in the register
                                          MASK, ARB, CONTROL, DATA A, DATA B=1 to
                                          to transfer them to the interface registers to be
                                          able to access them by the cpu */
            //in order for bits to be used for acceptance filtering we have to enable

            // CAN0_IF1MCTL_R |=0x1000;
            if(receivePtr->ID_type == 0) // 11 bit id
            {   /* if we use 11 bit id we have to shift the value by 2 because
                        the value is stored in bits 2:12 in the following registers */
                CAN0_IF1MSK2_R |= (receivePtr->ID_mask)<<2; // acceptance filter mask
                CAN0_IF1ARB2_R  |=(receivePtr->ID)<<2;      // message id
                CAN0_IF1ARB2_R  |= 0x8000;      //to set direction of transmission and validation bit
                CAN0_IF1ARB2_R &=~0x4000;       //clear the xtd bit


            }
            else //29 bit id
            {   /* for 29 bit id, the first register stores the first 16 bit of the id mask or id
                        and the other register stores the rest so we have to shift the other bits */
                CAN0_IF1MSK1_R |= (receivePtr->ID_mask);
                CAN0_IF1MSK2_R |= (receivePtr->ID_mask)>>16;
                CAN0_IF1ARB1_R  |=(receivePtr->ID);
                CAN0_IF1ARB2_R  |=(receivePtr->ID)>>16;
                CAN0_IF1ARB2_R  |= 0xC000; //set the direction, validation, extended frame bit.

            }
            CAN0_IF1MCTL_R |= 0x3080; // not using fifo,
            if( CAN0_IF1MCTL_R &0x8000) //If NEWDAT bit is set i.e. there is new data in the data registers
            { CAN0_IF1MCTL_R |= (receivePtr->bytesNum); //the number of bytes to be transmitted
            //  while (!( CAN0_IF1MCTL_R & 0x0100); //if no message request
            CAN0_IF1CRQ_R |= receivePtr->messageNum; //when matching occurs the data is received

            CAN0_IF1DA1_R |= receivePtr->Data;     //first 2 bytes of data
            CAN0_IF1DA2_R |= (receivePtr->Data)>>16;
            CAN0_IF1DB1_R |= (receivePtr->Data)>>16;
            CAN0_IF1DB2_R |= (receivePtr->Data)>>16;

            CAN0_IF1CMSK_R |= 0x04;//Set NEWDAT in CMSK to Clear the NEWDAT bit in MCTL

            }
            if(CAN0_IF1MCTL_R& 0x4000) //If MSGLST bit is set i.e. there was a message lost
            {
                CAN0_IF1MCTL_R &= ~0x4000; //Clear the MSGLST bit

            }
        }
        if(receivePtr->interface == 2)

        {
            while(CAN0_IF2CRQ_R & 0x00008000); //wait while can0-if1 is busy

            CAN0_IF2CMSK_R |=0XF3; /* WRNRD=1 to write in the register
                                                      MASK, ARB, CONTROL, DATA A, DATA B=1 to
                                                      to transfer them to the interface registers to be
                                                      able to access them by the cpu */
            //in order for bits to be used for acceptance filtering we have to enable

            // CAN0_IF1MCTL_R |=0x1000;
            if(receivePtr->ID_type == 0) // 11 bit id
            {   /* if we use 11 bit id we have to shift the value by 2 because
                                    the value is stored in bits 2:12 in the following registers */
                CAN0_IF2MSK2_R |= (receivePtr->ID_mask)<<2; // acceptance filter mask
                CAN0_IF2ARB2_R  |=(receivePtr->ID)<<2;      // message id
                CAN0_IF2ARB2_R  |= 0x8000;      //to set direction of transmission and validation bit
                CAN0_IF2ARB2_R &=~0x4000;       //clear the xtd bit


            }
            else //29 bit id
            {   /* for 29 bit id, the first register stores the first 16 bit of the id mask or id
                                    and the other register stores the rest so we have to shift the other bits */
                CAN0_IF2MSK1_R |= (receivePtr->ID_mask);
                CAN0_IF2MSK2_R |= (receivePtr->ID_mask)>>16;
                CAN0_IF2ARB1_R  |=(receivePtr->ID);
                CAN0_IF2ARB2_R  |=(receivePtr->ID)>>16;
                CAN0_IF2ARB2_R  |= 0xC000; //set the direction, validation, extended frame bit.

            }
            CAN0_IF2MCTL_R |= 0x3080; // not using fifo,
            CAN0_IF2MCTL_R |= (receivePtr->bytesNum); //the number of bytes to be transmitted
            if ( CAN0_IF2MCTL_R & 0x8000) //if no message request
            {
                CAN0_IF2CRQ_R |= receivePtr->messageNum; //when matching occurs the data is received
                CAN0_IF2DA1_R |= receivePtr->Data;     //first 2 bytes of data
                CAN0_IF2DA2_R |= (receivePtr->Data)>>16;
                CAN0_IF2DB1_R |= (receivePtr->Data)>>16;
                CAN0_IF2DB2_R |= (receivePtr->Data)>>16;

                CAN0_IF2CMSK_R |= 0x04;//Set NEWDAT in CMSK to Clear the NEWDAT bit in MCTL

            }
            if(CAN0_IF2MCTL_R& 0x4000) //If MSGLST bit is set i.e. there was a message lost
            {
                CAN0_IF2MCTL_R &= ~0x4000; //Clear the MSGLST bit


            }
            // CAN0_IF1MCTL_R |= 0x0100; //set the TXRQST bit to start transmission
        }
    }
    else if (receivePtr->module == 1) //CAN1
    {
        if(receivePtr->interface == 1) //CANIF1

        {
            while(CAN1_IF1CRQ_R & 0x00008000); //wait while can0-if1 is busy

            CAN1_IF1CMSK_R |=0XF3; /* WRNRD=1 to write in the register
                                              MASK, ARB, CONTROL, DATA A, DATA B=1 to
                                              to transfer them to the interface registers to be
                                              able to access them by the cpu */
            //in order for bits to be used for acceptance filtering we have to enable

            // CAN0_IF1MCTL_R |=0x1000;
            if(receivePtr->ID_type == 0) // 11 bit id
            {   /* if we use 11 bit id we have to shift the value by 2 because
                            the value is stored in bits 2:12 in the following registers */
                CAN1_IF1MSK2_R |= (receivePtr->ID_mask)<<2; // acceptance filter mask
                CAN1_IF1ARB2_R  |=(receivePtr->ID)<<2;      // message id
                CAN1_IF1ARB2_R  |= 0x8000;      //to set direction of transmission and validation bit
                CAN1_IF1ARB2_R &=~0x4000;       //clear the xtd bit


            }
            else //29 bit id
            {   /* for 29 bit id, the first register stores the first 16 bit of the id mask or id
                            and the other register stores the rest so we have to shift the other bits */
                CAN1_IF1MSK1_R |= (receivePtr->ID_mask);
                CAN1_IF1MSK2_R |= (receivePtr->ID_mask)>>16;
                CAN1_IF1ARB1_R  |=(receivePtr->ID);
                CAN1_IF1ARB2_R  |=(receivePtr->ID)>>16;
                CAN1_IF1ARB2_R  |= 0xC000; //set the direction, validation, extended frame bit.

            }
            CAN1_IF1MCTL_R |= 0x3080; // not using fifo,
            CAN1_IF1MCTL_R |= (receivePtr->bytesNum); //the number of bytes to be transmitted
            //  while (!( CAN0_IF1MCTL_R & 0x0100); //if no message request
            CAN1_IF1CRQ_R |= receivePtr->messageNum; //when matching occurs the data is received
            if ( CAN1_IF1MCTL_R & 0x8000)
            { CAN1_IF1DA1_R |= receivePtr->Data;     //first 2 bytes of data

            CAN1_IF1DA2_R |= (receivePtr->Data)>>16;


            CAN1_IF1DB1_R |= (receivePtr->Data)>>16;


            CAN1_IF1DB2_R |= (receivePtr->Data)>>16;
            CAN1_IF1CMSK_R |= 0x04;//Set NEWDAT in CMSK to Clear the NEWDAT bit in MCTL
            }
            if(CAN1_IF1MCTL_R & 0x4000) //If MSGLST bit is set i.e. there was a message lost
            {
                CAN1_IF1MCTL_R &= ~0x4000; //Clear the MSGLST bit


            }
            // CAN0_IF1MCTL_R |= 0x0100; //set the TXRQST bit to start transmission
        }
        if(receivePtr->interface == 2) //CANIF1

        {
            while(CAN1_IF2CRQ_R & 0x00008000); //wait while can0-if1 is busy

            CAN1_IF2CMSK_R |=0XF3; /* WRNRD=1 to write in the register
                                                          MASK, ARB, CONTROL, DATA A, DATA B=1 to
                                                          to transfer them to the interface registers to be
                                                          able to access them by the cpu */
            //in order for bits to be used for acceptance filtering we have to enable

            // CAN0_IF1MCTL_R |=0x1000;
            if(receivePtr->ID_type == 0) // 11 bit id
            {   /* if we use 11 bit id we have to shift the value by 2 because
                                        the value is stored in bits 2:12 in the following registers */
                CAN1_IF2MSK2_R |= (receivePtr->ID_mask)<<2; // acceptance filter mask
                CAN1_IF2ARB2_R  |=(receivePtr->ID)<<2;      // message id
                CAN1_IF2ARB2_R  |= 0x8000;      //to set direction of transmission and validation bit
                CAN1_IF2ARB2_R &=~0x4000;       //clear the xtd bit


            }
            else //29 bit id
            {   /* for 29 bit id, the first register stores the first 16 bit of the id mask or id
                                        and the other register stores the rest so we have to shift the other bits */
                CAN1_IF2MSK1_R |= (receivePtr->ID_mask);
                CAN1_IF2MSK2_R |= (receivePtr->ID_mask)>>16;
                CAN1_IF2ARB1_R  |=(receivePtr->ID);
                CAN1_IF2ARB2_R  |=(receivePtr->ID)>>16;
                CAN1_IF2ARB2_R  |= 0xC000; //set the direction, validation, extended frame bit.

            }
            CAN1_IF2MCTL_R |= 0x3080; // not using fifo,
            CAN1_IF2MCTL_R |= (receivePtr->bytesNum); //the number of bytes to be received
            if ( CAN1_IF2MCTL_R & 0x8000) //if no message request
            {  CAN1_IF2CRQ_R |= receivePtr->messageNum; //when matching occurs the data is received
            CAN1_IF2DA1_R |= receivePtr->Data;     //first 2 bytes of data

            CAN1_IF2DA2_R |= (receivePtr->Data)>>16;


            CAN1_IF2DB1_R |= (receivePtr->Data)>>16;


            CAN1_IF2DB2_R |= (receivePtr->Data)>>16;

            CAN1_IF2CMSK_R |= 0x04;//Set NEWDAT in CMSK to Clear the NEWDAT bit in MCTL
            }
            if(CAN1_IF2MCTL_R& 0x4000) //If MSGLST bit is set i.e. there was a message lost
            {
                CAN1_IF2MCTL_R &= ~0x4000; //Clear the MSGLST bit


            }

            // CAN0_IF1MCTL_R |= 0x0100; //set the TXRQST bit to start transmission
        }
    }

}
/*
 * Description : Function to enable test mode
 *
 *  Arguments: pointer to structure holding the required info which are:
 *
                    can_Module module; //can0 or can1
                    can_testingType mode;
 *  Returns: void
 */
void can_enableTestMode(const can_testingStruct* testingPtr)
{
    if(testingPtr->module == 0)
    {
        CAN0_CTL_R |= 0X80;
        if(testingPtr->mode ==0)
        {
            CAN0_TST_R |=0X80;
        }
        else if (testingPtr->mode ==1)
        {
            CAN0_TST_R |=0X20;
        }
        else if (testingPtr->mode ==2)
        {
            CAN0_TST_R |=0X60;
        }
        else if (testingPtr->mode ==2)
        {
            CAN0_TST_R |=0X40;
        }
    }
    else if(testingPtr->module == 1)
    {
        CAN1_CTL_R |= 0X80;
        if(testingPtr->mode ==0)
        {
            CAN1_TST_R |=0X80;
        }
        else if (testingPtr->mode ==1)
        {
            CAN1_TST_R |=0X20;
        }
        else if (testingPtr->mode ==2)
        {
            CAN1_TST_R |=0X60;
        }
        else if (testingPtr->mode ==2)
        {
            CAN1_TST_R |=0X40;
        }
    }
}
/*
 * Description : Function to enable silent mode
 *
 *  Arguments: pointer to enum holding the required info which are:
 *
                    can_Module module; //can0 or can1

 *  Returns: void
 */
void can_enableSilentMode(const can_Module* module)
{
    if(module ==0)
    {    CAN0_CTL_R |= 0X80;
        CAN0_TST_R |=0X08;
    }
    else
    {    CAN1_CTL_R |= 0X80;
        CAN1_TST_R |=0X08;
    }
}
/*
 * Description : Function to enable loopback mode
 *
 *  Arguments: pointer to enum holding the required info which are:
 *
                    can_Module module; //can0 or can1

 *  Returns: void
 */
void can_enableLoopBackMode(const can_Module* module)
{
    if(module ==0)
    {
        CAN0_CTL_R |= 0X80;
        CAN0_TST_R |=0X10;
    }
    else
    {
        CAN1_CTL_R |= 0X80;
        CAN1_TST_R |=0X10;
    }
}

