#include "logic.h"
#include "hal.h"
#include <string.h>

const char *help[] = {  "Commands to use:\r\n",
                        " ? - to print this help\r\n",
                        "spi mode [0|1|2|3] - to get or set spi mode\r\n",
                        "spi 0xXX [0xXX] - write/read spi\r\n",
                        " 0xXX - hexadecimal byte - 1 or 2\r\n"
                        };

char buf_out[28];

void uint8tohex(char *buf, uint8_t val) {
    if ((val & 0xF0) > 0x90) {
        *buf = 'A' + ((val >> 4) - 10);
    } else {
        *buf = '0' + (val >> 4);
    }
    if ((val & 0xF) > 0x09) {
        *(buf + 1) = 'A' + ((val & 0x0F) - 10);
    } else {
        *(buf + 1) = '0' + (val & 0x0F);
    }
}

__bit hextouint8(const char *buf, uint8_t *val) {
    *val = 0;
    uint8_t loop = 1;
    do {
        *val <<= 4;
        if (*buf >= '0' && *buf <= '9') {
            *val |= *buf - '0';
        } else if ((*buf & 0xDF) >= 'A' && (*buf & 0xDF) <= 'F') {
            *val |= (*buf & 0xDF) - 'A' + 10;
        } else {
            return 0;
        }
        buf++;
    } while (loop--);
    return 1;
}

void print_help() {
    //Do not use sizeof - it is unrelaibale and breaks on long lines
    for (uint8_t count = 0; count < 5/*sizeof(help)*/; count++) {
        print(help[count]);
    }
}

void on_cmd(const char *msg) {
    do {
        if (*msg == '?') {
            print_help();
        } else if (!strncmp(msg, "spi", 3)) {
            if (!strncmp(msg + 3, " mode", 5)) {
                if (*(msg + 8) == ' ') {
                    switch(*(msg + 9)) {
                        case '0':
                            spi_set_mode(spi_mode_0);
                            break;
                        case '1':
                            spi_set_mode(spi_mode_1);
                            break;
                        case '2':
                            spi_set_mode(spi_mode_2);
                            break;
                        case '3':
                            spi_set_mode(spi_mode_3);
                            break;
                        default:
                            print_help();
                            break;
                    }
                }
                strcpy(buf_out, "spi mode ");
                switch (spi_get_mode()) {
                    case spi_mode_0:
                        buf_out[9] = '0';
                        break;
                    case spi_mode_1:
                        buf_out[9] = '1';
                        break;
                    case spi_mode_2:
                        buf_out[9] = '2';
                        break;
                    case spi_mode_3:
                        buf_out[9] = '3';
                        break;
                }
                buf_out[10] = 0;
                print(buf_out);
            } else {
                uint8_t w_data[4] = {0,0,0,0};
                uint8_t w_count = 1;
                uint8_t repeat = 1;
                uint8_t seek = 0;
                msg += 3;
                uint8_t len = (uint8_t) strlen(msg);
                if (len >= 5) {
                    if (!strncmp(msg," 0x", 3)) {
                        msg += 3;
                        if (!hextouint8(msg, &w_data[0])) {
                            print_help();
                            break;
                        }    
                        msg +=2;
                        if (*msg) {
                            if (strlen(msg) == 5) {
                                if (!strncmp(msg," 0x", 3)) {
                                    msg += 3;
                                    if (!hextouint8(msg, &w_data[1])) {
                                        print_help();
                                        break;
                                    } else {
                                        repeat = 2;
                                        w_count = 2;
                                    }   
                                }    
                                
                            } else {
                                print_help();
                                break;
                            }
                        }
                    } else {
                        print_help();
                        break;
                    } 
                } else {
                    print_help();
                    break;
                }
                // wait for printer before updating buf_out
                //wait_for_printer();
                char *out_hex = buf_out;
                uint8_t wr = 0;
                uint8_t resp  = 0;

                spi_cs_down();
                
                while(repeat--) {
                    *out_hex = ' ';
                    *(out_hex + 1)= '0';
                    *(out_hex + 2) = 'x';
                    if (seek < w_count) {
                        wr = w_data[seek++];
                    }
                    resp = spi_exchange(wr);
                    uint8tohex(out_hex + 3, resp);
                    out_hex += 5;
                }
                spi_cs_up();
                *out_hex = 0;
                print(buf_out);
            }
        } else {
            print(msg);
        }
    } while(0);
    print("\r\n$");
}

