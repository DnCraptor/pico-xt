/*
  Fake86: A portable, open-source 8086 PC emulator.
  Copyright (C)2010-2013 Mike Chambers
            (C)2020      Gabor Lenart "LGB"

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/* ssource.c: functions to emulate the Disney Sound Source's 16-byte FIFO buffer. */
#include "../emulator.h"
// https://archive.org/details/dss-programmers-guide/page/n1/mode/2up
// https://groups.google.com/g/comp.sys.ibm.pc.games/c/gsz2CLJZsx4
#define COVOX_BUF_SZ 16
static uint8_t ssourcebuf[COVOX_BUF_SZ] = { 0 };
static volatile uint8_t ssourceptrIn = 0;
static volatile uint8_t ssourceptrOut = 0;
static volatile uint8_t free_buff_sz = COVOX_BUF_SZ;

int16_t tickssource() { // core #1
    if (ssourceptrIn == ssourceptrOut) { // no bytes in buffer
        return 0;
    }
    port379 = 0; // ready for next byte
    register int16_t res = ssourcebuf[ssourceptrOut++];
    free_buff_sz++;
    return res;
}

inline static void putssourcebyte(uint8_t value) { // core #0
    if (free_buff_sz >= COVOX_BUF_SZ) { // ignore input, no free space in buffer
        return;
    }
    ssourcebuf[ssourceptrIn++] = value;
    if (ssourceptrIn >= 16) {
        ssourceptrIn = 0;
    }
    free_buff_sz--;
    if (free_buff_sz == 0) {
        port379 = 0x40; // Buffer is full
    }
}

static inline uint8_t ssourcefull() {
    return free_buff_sz == 0 ? 0x40 : 0x00;
}

void outsoundsource(uint16_t portnum, uint8_t value) {
    static uint8_t last37a = 0;
    switch (portnum) {
        case 0x378:
            putssourcebyte(value);
            break;
        case 0x37A:
            if ((value & 4) && !(last37a & 4))
                putssourcebyte(port378);
            last37a = value;
            break;
    }
}

uint8_t insoundsource(uint16_t portnum) {
    return ssourcefull();
}
