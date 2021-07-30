/***************************************************************************
 *   Copyright (C) 2012 by Johann Glaser <Johann.Glaser@gmx.at>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "commands.h"
#include "common.h"
#include "usb.h"
#include "i2c.h"
#include "io.h"
#include "delay.h"

// local copy of the information we got in the SETUPDAT packet
volatile uint8_t  Command;
volatile uint16_t CmdIndex;
volatile uint16_t CmdValue;

extern int led_state;

/****************************************************************************/
/***  GetVersion  ***********************************************************/
/****************************************************************************/

/**
 * Alias IN0BUF to variable Version
 */
volatile __xdata __at 0x7F00 /*IN0BUF*/ TGetVersion Version;

/**
 * Command: GetVersion
 *
 * Return firmware version, ...
 *
 * Fills IN0BUF and arms EP0IN.
 */
void GetVersion() {
  // fill Version
  Version.Firmware = FIRMWARE_VERSION;
  // ... fill other fields as declared in TGetVersion in commands.h ...
  IN0BC = sizeof(Version);
}

/****************************************************************************/
/***  GetVersionString  *****************************************************/
/****************************************************************************/

const char __code const * VersionString = "EZ-USB Firmware 0.1";

/**
 * Command: GetVersionString
 *
 * Return firmware version string
 *
 * Fills IN0BUF and arms EP0IN.
 */
void GetVersionString() {
  uint8_t b;
  __code char* Src;
  __xdata char* Dst;

  // copy version string
  Src = VersionString;
  Dst = IN0BUF;
  b = 0;
  while (*Src) {
    *Dst++ = *Src++;
    b++;
  }
  // arm endpoint
  IN0BC = b;
}

/****************************************************************************/
/***  GetStatus  ************************************************************/
/****************************************************************************/

/**
 * Alias IN0BUF to variable Status
 */
volatile __xdata __at 0x7F00 /*IN0BUF*/ TGetStatus Status;

/**
 * Command: GetStatus
 *
 * Return status of EndCount input and the current value of ReadCount.
 *
 * Fills IN0BUF and arms EP0IN.
 */
void GetStatus() {
  // fill Status
  Status.MyStatus = 1;
  // ... fill other fields as declared in TGetStatus in commands.h ...
  IN0BC = sizeof(Status);
}

int idx = 0;

uint8_t uart_tx_data_size;
uint8_t uart_tx_data_pos;
uint8_t uart_tx_data[64];
uint8_t uart_tx_active;

void GetBuffer() {
  uint8_t b;
  char* Src;
  char* Dst;

  Src = uart_tx_data; // TODO
  Dst = IN2BUF;
  b = 0;
  while (*Src) {
    *Dst++ = *Src++;
    b++;
  }

  IN2BC = b;
}

void SetBuffer() {
  char* Src = OUT2BUF;
  char* data = uart_tx_data;

  if (OUT2BC <= 0) {
    return;
  }

  uint8_t cmd = *Src++;
  uint8_t len = *Src++;

  if (cmd == 0xd0) {
    led_state = 10;
    for (int i = 0; i < len; i++){
      *data++ = *Src++;
    }

    uart_tx_data_size = len;
    uart_tx_data_pos = 0;
  }

}

/****************************************************************************/
/***  Command Handler  ******************************************************/
/****************************************************************************/

/**
 * Command Handler
 *
 * This function is executed from command_loop() if its semaphore is set.
 */
void HandleCmd() {
  // save command
  Command  = setup_data.bRequest;
  CmdIndex = setup_data.wIndex;
  CmdValue = setup_data.wValue;
  switch (Command) {
    case CMD_GET_VERSION: { // Get Version ////////////////////////////////////
      GetVersion();
      break;
    }
    case CMD_GET_VERSION_STRING: { // Get Version String //////////////////////
      GetVersionString();
      break;
    }
    case CMD_GET_STATUS: {  // return current status //////////////////////////
      GetStatus();
      break;
    }
    // ... add further commands here ...
    default: {
      break;
    }
  }
}

void HandleOut() {
  uint8_t cmd = *(uint8_t*)OUT2BUF;

  switch (cmd) {
    case 0xd0: 
      SetBuffer();
      break;
    case 0xd1:
      GetBuffer();
      break;
  }
  OUT2BC = 0;
}

/**
 * Main command loop
 *
 * This function has an infinite loop and does not return.
 *
 */
void command_loop(void) {
  // arm EP2OUT for the first time so we are ready for JTAG commands
  //OUT2BC = 0;
  // command loop
  while (true) {
    // got a command packet?
    if (Semaphore_Command) {
      HandleCmd();
      Semaphore_Command = false;
    }
    // got an EP2 IN interrupt?
    if (Semaphore_EP2_in) {
      // ... handle ...
      Semaphore_EP2_in = false;
    }
    // got an EP2 OUT interrupt?
    if (Semaphore_EP2_out) {
      led_state = 10;
      HandleOut();
      Semaphore_EP2_out = false;
    }
    if ((uart_tx_data_size > 0) && ((!uart_tx_active) || (uart_tx_active && TI_0))) {
      SBUF0 = uart_tx_data[uart_tx_data_pos];
      uart_tx_data_pos++;
      uart_tx_data_size--;
      uart_tx_active = 1;
      TI_0 = 0;
    }
    if ((uart_tx_data_size == 0) && uart_tx_active && TI_0) {
      uart_tx_active;
    }
  }
}
