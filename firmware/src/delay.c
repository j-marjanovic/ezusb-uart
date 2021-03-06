/***************************************************************************
 *   Copyright (C) 2011 by Martin Schmoelzer                               *
 *   <martin.schmoelzer@student.tuwien.ac.at>                              *
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

#include "delay.h"

#define NOP {__asm nop __endasm;}

void delay_5us(void) {
  NOP;
}

void delay_1ms(void) {
  uint16_t i;

  for (i = 0; i < 598; i++) {
    NOP;
  }
}

void delay_us(uint16_t delay) {
  uint16_t i;
  uint16_t maxcount = (delay / 5);

  for (i = 0; i < maxcount; i++) {
    delay_5us();
  }
}

void delay_ms(uint16_t delay) {
  uint16_t i;

  for (i = 0; i < delay; i++) {
    delay_1ms();
  }
}
