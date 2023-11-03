/* ports.c - handles port I/O for Fake86 CPU core. it's ugly, will fix up later. */
#include "emulator.h"

#if !PICO_ON_DEVICE
#include <windows.h>

unsigned long millis() {
    static unsigned long start_time = 0;
    static bool timer_initialized = false;
    SYSTEMTIME current_time;
    FILETIME file_time;
    unsigned long long time_now;

    if (!timer_initialized) {
        GetSystemTime(&current_time);
        SystemTimeToFileTime(&current_time, &file_time);
        start_time =
                (((unsigned long long) file_time.dwHighDateTime) << 32) | (unsigned long long) file_time.dwLowDateTime;
        timer_initialized = true;
    }

    GetSystemTime(&current_time);
    SystemTimeToFileTime(&current_time, &file_time);
    time_now = (((unsigned long long) file_time.dwHighDateTime) << 32) | (unsigned long long) file_time.dwLowDateTime;

    return (unsigned long) ((time_now - start_time) / 10000);
}

#else

#include "vga.h"
#include "pico/time.h"

#define millis() (time_us_64())
#endif

uint16_t pit0counter = 65535;
uint32_t speakercountdown, latch42, pit0latch, pit0command, pit0divisor;
uint16_t portram[256];
uint8_t crt_controller_idx, crt_controller[256];
uint16_t port3DA;
uint16_t port3D9;
uint16_t port3D8;

void portout(uint16_t portnum, uint16_t value) {
    switch (portnum) {
        case 0x20:
        case 0x21: //i8259
            out8259(portnum, value);
            return;
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x43: //i8253
            out8253(portnum, value);
            break;
        case 0x3D4:
            crt_controller_idx = value;
            break;
        case 0x3D5:
            crt_controller[crt_controller_idx] = value;
            if ((crt_controller_idx == 0x0E) || (crt_controller_idx == 0x0F)) {
                //setcursor(((uint16_t)crt_controller[0x0E] << 8) | crt_controller[0x0F]);
            }
            break;
            // CGA mode  switch
        case 0x3D8:
            printf("wr pr3D8 %x\r\n", value);
            port3D8 = value;
/*            if (value == 0x28) {
                setVGAmode(VGA640x480_text_40_30);
            }*/
/*            switch (value) {
                case 0x2C: videomode = 0; break;
                case 0x28: videomode = 1; break;
                case 0x2D: videomode = 2; break;
                case 0x29: videomode = 3; break;
                case 0x0E: videomode = 4; break;
                case 0x0A: videomode = 5; break;
                case 0x1E: videomode = 6; break;
                default: videomode = 3; break;
            }*/
            break;
        case 0x3DA:
            break;

        case 0x3D9:
            port3D9 = value;
#if PICO_ON_DEVICE
            uint32_t usepal = (value >> 5) & 1;
            uint32_t intensity = ((value >> 4) & 1) << 3;
            for (int i = 0; i < 16; ++i) {
                setVGA_color_palette(i, cga_color(i * 2 + usepal + intensity));
            }
#endif
            break;
        default:
            if (portnum < 256) portram[portnum] = value;
    }
}

uint16_t portin(uint16_t portnum) {
    switch (portnum) {
        case 0x20:
        case 0x21: //i8259
            return in8259(portnum);
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x43: //i8253
            return in8253(portnum);
        case 0x60:
        case 0x64:
            return portram[portnum];
        case 0x3D4:
            return crt_controller_idx;
        case 0x3D5:
            return crt_controller[crt_controller_idx];
        case 0x3D8:
            switch (videomode) {
                case 0:
                    return (0x2C);
                case 1:
                    return (0x28);
                case 2:
                    return (0x2D);
                case 3:
                    return (0x29);
                case 4:
                    return (0x0E);
                case 5:
                    return (0x0A);
                case 6:
                    return (0x1E);
                default:
                    return (0x29);
            }

        case 0x3D9:
            return port3D9;

        case 0x3DA:
            port3DA ^= 1;
            if (!(port3DA & 1)) port3DA ^= 8;
            //port3da = random(256);
            return port3DA;
        default:
            return 0xFF;
    }
}

__inline void portout16(uint16_t portnum, uint16_t value) {
#ifdef DEBUG_PORT_TRAFFIC
    printf("IO: writing WORD port %Xh with data %04Xh\n", portnum, value);
#endif
    portout(portnum, (uint8_t) value);
    portout(portnum + 1, (uint8_t) (value >> 8));
}

__inline uint16_t portin16(uint16_t portnum) {
    uint16_t ret = (uint16_t) portin(portnum);
    ret |= ((uint16_t) portin(portnum + 1) << 8);
#ifdef DEBUG_PORT_TRAFFIC
    printf("IO: reading WORD port %Xh with result of data %04Xh\n", portnum, ret);
#endif
    return ret;
}
