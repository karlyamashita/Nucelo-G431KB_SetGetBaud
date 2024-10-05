/*
 * PollingRoutine.c
 *
 *  Created on: Oct 24, 2023
 *      Author: karl.yamashita
 *
 *
 *      Template for projects.
 *
 *      The object of this PollingRoutine.c/h files is to not have to write code in main.c which already has a lot of generated code.
 *      It is cumbersome having to scroll through all the generated code for your own code and having to find a USER CODE section so your code is not erased when CubeMX re-generates code.
 *      
 *      Direction: Call PollingInit before the main while loop. Call PollingRoutine from within the main while loop
 * 
 *      Example;
        // USER CODE BEGIN WHILE
        PollingInit();
        while (1)
        {
            PollingRoutine();
            // USER CODE END WHILE

            // USER CODE BEGIN 3
        }
        // USER CODE END 3

 */


#include "main.h"

extern UART_HandleTypeDef huart2;
extern TimerCallbackStruct timerCallback;

UART_DMA_QueueStruct uart2_msg =
{
	.huart = &huart2,
	.rx.queueSize = UART_DMA_QUEUE_SIZE,
	.tx.queueSize = UART_DMA_QUEUE_SIZE,
};

#define BAUD_SELECTION_SIZE 5
uint32_t baudSelection[BAUD_SELECTION_SIZE] = {9600, 19200, 28800, 57600, 115200};
BaudRate_t uart2_baudRate = {0};
uint32_t errorCode = 0;

char notifyStr[64] = {0};

void PollingInit(void)
{
	UART_DMA_EnableRxInterrupt(&uart2_msg);

	TimerCallbackRegisterOnly(&timerCallback, BaudSetCallback);

	STM32_Ready();
}

void PollingRoutine(void)
{
	TimerCallbackCheck(&timerCallback);

	UART_Parse(&uart2_msg);
}

void UART_Parse(UART_DMA_QueueStruct  *msg)
{
	UART_DMA_Data *ptr;

	if(UART_DMA_MsgRdy(msg))
	{
		ptr = msg->rx.msgToParse;
		RemoveSpaces((char*)ptr->data);
		ToLower((char*)ptr->data);

		if(strncmp((char*)ptr->data, "setbaud:", strlen("setbaud:")) == 0)
		{
			uart2_baudRate.baudNew = atoi((char*)ptr->data + strlen("setbaud:"));
			sprintf(notifyStr, "Setting baud rate to %ld in 1 seconds", uart2_baudRate.baudNew);
			UART_DMA_NotifyUser(msg, notifyStr, strlen(notifyStr), true);
			TimerCallbackTimerStart(&timerCallback, BaudSetCallback, 2000, TIMER_NO_REPEAT);
		}
		else if(strncmp((char*)ptr->data, "getbaud", strlen("getbaud")) == 0)
		{
			sprintf(notifyStr, "Baud = %ld", msg->huart->Init.BaudRate);
			UART_DMA_NotifyUser(msg, notifyStr, strlen(notifyStr), true);
		}
		else if(strncmp((char*)ptr->data, "setdummy:", strlen("setdummy:")) == 0)
		{
			uart2_baudRate.dummyData = atoi((char*)ptr->data + strlen("setdummy:"));

		}
		else if(strncmp((char*)ptr->data, "getdummy", strlen("getdummy")) == 0)
		{
			sprintf(notifyStr, "dummy = %ld", uart2_baudRate.dummyData);
			UART_DMA_NotifyUser(msg, notifyStr, strlen(notifyStr), true);
		}
	}
}

/*
 * Description: This is called from a timer callback after 1 second.
 * 				This allows time to send user a message before we change the baud rate.
 */
void BaudSetCallback(void)
{
	BaudRateSet(uart2_baudRate.baudNew);
}

/*
 * Description: DeInit UART before Initializing with new baud.
 *
 */
void BaudRateSet(uint32_t baud)
{
	if(HAL_UART_DeInit(uart2_msg.huart) == HAL_OK)
	{
		uart2_msg.huart->Init.BaudRate = baud;
		if (HAL_UART_Init(uart2_msg.huart) == HAL_OK)
		{
			UART_DMA_EnableRxInterrupt(&uart2_msg);
		}
		else
		{
			Error_Handler(); // turn on LED
		}
	}
	else
	{
		Error_Handler();
	}
}

void STM32_Ready(void)
{
	sprintf(notifyStr, "STM32 Ready: %ld", uart2_msg.huart->Init.BaudRate);
	UART_DMA_NotifyUser(&uart2_msg, notifyStr, strlen(notifyStr), true);
}

//************** HAL callbacks ***************

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if(huart == uart2_msg.huart)
	{
		RingBuff_Ptr_Input(&uart2_msg.rx.ptr, uart2_msg.rx.queueSize);
		UART_DMA_EnableRxInterrupt(&uart2_msg);
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart == uart2_msg.huart)
	{
		uart2_msg.tx.txPending = false;
		UART_DMA_SendMessage(&uart2_msg);
	}
}

/*
 * Description: This is probably called due to framing error cause by mismatched baud rate
 * 				between the STM32 and device sending data. Calling SetBaudRate will reset the UART peripheral
 * 				and use the last working baud rate
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{


	if(huart == uart2_msg.huart)
	{
		errorCode = HAL_UART_GetError(huart);
		if(errorCode == USART_ISR_FE || USART_ISR_NE) // two most common errors due to baud mismatch. Framing and Noise errors.
		{
			BaudRateSet(baudSelection[uart2_baudRate.baudPtr]); // try new baud rate
			if(++uart2_baudRate.baudPtr == BAUD_SELECTION_SIZE)
			{
				uart2_baudRate.baudPtr = 0;
			}
		}
	}
}



