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

#include "PAD.h"

/** Current Idle period. This is set by the host via a Set Idle HID class request to silence the device's reports
 *  for either the entire idle duration, or until the report status changes (e.g. the user presses a key).
 */
static uint16_t IdleCount = 500;

/** Current Idle period remaining. When the IdleCount value is set, this tracks the remaining number of idle
 *  milliseconds. This is separate to the IdleCount timer and is incremented and compared as the host may request
 *  the current idle period via a Get Idle HID class request, thus its value must be preserved.
 */
static uint16_t IdleMSRemaining = 0;


/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();

	GlobalInterruptEnable();

	for (;;){
		//PAD_Task();
	  	USB_USBTask();
	}
}

/** Configures the board hardware and chip peripherals for the project's functionality. */
void SetupHardware(void)
{
	Serial_Init(9600,false);
}




/** Sends the next HID report to the host, via the keyboard data endpoint. */
void SendNextReport(void)
{
	static char PrevPADReportData;
	char PADReportData = Serial_ReceiveByte();
	bool SendReport = false;

	/* Create the next keyboard report for transmission to the host */
	
	/* Check if the idle period is set and has elapsed */
	if (IdleCount && (!(IdleMSRemaining))){
		/* Reset the idle time remaining counter */
		IdleMSRemaining = IdleCount;
		/* Idle period is set and has elapsed, must send a report to the host */
		SendReport = true;
	}
	else{
		/* Check to see if the report data has changed - if so a report MUST be sent */
		SendReport = (memcmp(&PrevPADReportData, &PADReportData, sizeof(char)) != 0);
	}

	/* Select the Keyboard Report Endpoint */
	Endpoint_SelectEndpoint(PAD_IN_EPADDR);
	
	/* Check if PAD Endpoint Ready for Read/Write and if we should send a new report */
	if (Endpoint_IsReadWriteAllowed() && SendReport){
		/* Save the current report data for later comparison to check for changes */
		PrevPADReportData = PADReportData;
		/* Write Keyboard Report Data */
		Endpoint_Write_8(PADReportData);
		/* Finalize the stream transfer to send the last packet */
		Endpoint_ClearIN();
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
			if(LEDReport == 0x0F){ //Extinction LED
				Serial_SendByte(40);	
			}
			else if(LEDReport == 0xF0){ //Allumage LED
				Serial_SendByte(41);
			}
		}

		/* Handshake the OUT Endpoint - clear endpoint and ready for next report */
		Endpoint_ClearOUT();
	}
}

void PAD_Task(void)
{
	/* Device must be connected and configured for the task to run */
	if (USB_DeviceState != DEVICE_STATE_Configured)
	  return;

	/* Send the next keypress report to the host */
	if(Serial_IsCharReceived()){SendNextReport();}

	/* Process the LED report sent from the host */
	ReceiveNextReport();
}

