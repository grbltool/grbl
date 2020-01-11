/*
  psoc5.c - code forspecial festures of the proc5 board
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

#include "grbl.h"


void fault_init()
{
    FAULT_STATUS_REG_InterruptEnable();
    ISR_FAULT_StartEx(isr_fault_handler);

}


// Disables fault pins.
void fault_disable()
{
  FAULT_STATUS_REG_InterruptDisable();
  ISR_FAULT_Stop();
}


// Returns fault state as a bit-wise uint8 variable. Each bit indicates an axis fault, where
// occured is 1 and not occured is 0.
// number in bit position, i.e. Z_AXIS is (1<<2) or bit 2, and Y_AXIS is (1<<1) or bit 1.
uint8_t fault_get_state()
{
  uint8_t fault_state = FAULT_STATUS_REG_Read();
  return fault_state;
}


// This is the Fault Pin Change Interrupt.
CY_ISR(isr_fault_handler)
{
  if (sys.state != STATE_ALARM) {
    if (!(sys_rt_exec_alarm)) {
      mc_reset(); // Initiate system kill.
      system_set_exec_alarm(EXEC_ALARM_STEPPER_DRIVER_FAULT); // Indicate hard limit critical event
    }
  }
}
