/*
 * PollingRoutine.h
 *
 *  Created on: Oct 24, 2023
 *      Author: karl.yamashita
 *
 *
 *      Template
 */

#ifndef INC_POLLINGROUTINE_H_
#define INC_POLLINGROUTINE_H_


/*

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>
#define Nop() asm(" NOP ")

#include "PollingRoutine.h"

*/
#ifndef __weak
#define __weak __attribute__((weak))
#endif


typedef struct
{
	uint32_t baudNew; // the baud rate received from user
	uint32_t baudLast; // the last baud rate before UARTx baud rate was updated. In case of errors, we can revert back.
	uint32_t dummyData;
}BaudRate_t;

void PollingInit(void);
void PollingRoutine(void);

void UART_Parse(UART_DMA_QueueStruct  *msg);
void BaudSetCallback(void);
void BaudRateSet(uint32_t baud);
void BaudRateInit(BaudRate_t *baudInstance, uint32_t baud);
void STM32_Ready(void);


#endif /* INC_POLLINGROUTINE_H_ */
