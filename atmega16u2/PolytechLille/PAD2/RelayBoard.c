/*
             LUFA Library
     Copyright (C) Dean Camera, 2017.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2010  OBinou (obconseil [at] gmail [dot] com)
  Copyright 2017  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Main source file for the RelayBoard program. This file contains the main tasks of
 *  the project and is responsible for the initial application hardware configuration.
 */

#include "RelayBoard.h"

uint8_t data_send;
uint8_t state_send = 1;

ISR(USART1_RX_vect)
{
	data_send = UDR1;
	state_send = 1;
}



void SetupHardware(void)
{
	
	UCSR1B |= (1 << RXCIE1); // Enable the USART Receive Complete interrupt (USART_RXC)
	Serial_Init(9600,false);
	USB_Init();
}

void SendNextReport(void)
{
	if(state_send)
	{
		Endpoint_SelectEndpoint(PAD_IN_EPADDR);

		if (Endpoint_IsReadWriteAllowed())
		{
			Endpoint_Write_Stream_LE(&data_send, sizeof(uint8_t), NULL);
			Endpoint_ClearIN();
		}	
		
		state_send = 0;
	}	
}


/** Reads the next LED status report from the host from the LED data endpoint, if one has been sent. */
void ReceiveNextReport(void)
{
	/* Select the Keyboard LED Report Endpoint */
	Endpoint_SelectEndpoint(PAD_OUT_EPADDR);

	/* Check if Keyboard LED Endpoint contains a packet */
	if (Endpoint_IsOUTReceived()){
		/* Check to see if the packet contains data */
		if (Endpoint_IsReadWriteAllowed()){
			/* Read in the LED report from the host */
			uint8_t LEDReport = Endpoint_Read_8();
			Serial_SendByte(LEDReport);
		}

		Endpoint_ClearOUT();
	}
}

void PAD_Task(void)
{
	
	if (USB_DeviceState != DEVICE_STATE_Configured)
	  return;

	
	SendNextReport();
	ReceiveNextReport();
}

void EVENT_USB_Device_ConfigurationChanged(void)
{
	Endpoint_ConfigureEndpoint(PAD_IN_EPADDR, EP_TYPE_INTERRUPT, PAD_EPSIZE, 1);
	Endpoint_ConfigureEndpoint(PAD_OUT_EPADDR, EP_TYPE_INTERRUPT, PAD_EPSIZE, 1);
	USB_Device_EnableSOFEvents();
}

int main(void)
{
	SetupHardware();

	GlobalInterruptEnable();

	for (;;){
		USB_USBTask();
		PAD_Task();
	}	
}

