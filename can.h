/*
 * File name: can.h
 *
 *  Created on: Sep 5, 2021
 *      Author: Zahwa Nasser
 */

#ifndef CAN_H_
#define CAN_H_
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "std_types.h"
/*******************************************************************************
 *                         Types Declaration                                   *
 *******************************************************************************/
//assume we use CANIF1 for transmission and CANIF2 for receiving
typedef enum {
    module0, module1
}can_Module;
typedef enum {
    interface1=1, interface2=2
}can_Interface;
typedef enum {
    normal, extended
}can_IdType;
typedef enum {
    data, remote
}can_frameType;
typedef enum
{
 receive,bitTiming, physicalHigh, physicalLow
}can_testingType;
typedef struct {
    can_Module module;
    uint64 bitRate; //bit time needs to be an integer multiple of the can clk
    uint8 n;    //needs to be between 4 and 25
    uint64 Fsys;
    float32 delays;

}can_configStruct;
typedef struct {
    can_Interface interface; //CANIF1 or CANIF2
    can_Module module; //can0 or can1
    can_frameType frameType; //DATA OR REMOTE
    can_IdType ID_type;    //normal or extended
    uint32 ID_mask; //mask for acceptance filtering
    uint32 ID; //ID OF THE MESSAGE
    uint8 bytesNum; //no of bytes to be sent
    uint64 Data; //data to be sent
    uint8 messageNum; //data object number in the can ram
}can_transmitStruct;
typedef struct{
    can_Interface interface;
    can_Module module; //can0 or can1
    uint8 bytesNum;//bytes that need to be updated
    uint64 Data;
    uint8 messageNum;
}can_updateStruct;
typedef struct{
    can_Interface interface;
    can_Module module; //can0 or can1
    can_IdType ID_type;    //normal or extended
    uint32 ID_mask; //mask for acceptance filtering
    uint32 ID; //ID OF THE MESSAGE
    uint8 bytesNum; //no of bytes to be sent
    uint64 Data; //data to be sent
    uint8 messageNum; //data object number in the can ram

}can_receiveStruct;
typedef struct
{
     can_Module module; //can0 or can1
     can_testingType mode;

}can_testingStruct;

/*******************************************************************************
 *                      Functions Prototypes                                   *
 *******************************************************************************/
bool can_init(const can_configStruct* configPtr);
void can_transmit(const can_transmitStruct* transmitPtr);
void can_updateMessage(const can_updateStruct* updatePtr);
void can_receive(const can_receiveStruct* receivePtr);
void can_enableTestMode(const can_testingStruct* testingPtr);
void can_enableSilentMode(const can_Module* module);
void can_enableLoopBackMode(const can_Module* module);




#endif /* CAN_H_ */
