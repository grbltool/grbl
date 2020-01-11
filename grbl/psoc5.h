/*
  psoc5.h - code forspecial festures of the proc5 board
  Part of Grbl

  Copyright (c) 2019 Andreas Jungierek

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef psoc5_h
#define psoc5_h


#include <stdint.h>
#include "config.h"

#ifdef PSOC
  #include "cytypes.h"
  CY_ISR_PROTO(isr_fault_handler);
#endif
  
// Initialize the fault module
void fault_init();

// disable fault status
void fault_disable();

// Returns fault state as a bit-wise uint8 variable.
uint8_t fault_get_state();

#endif
