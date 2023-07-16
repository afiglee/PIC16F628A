/*
 * File:   main.c
 * Author: m2tx9
 *
 * Created on July 14, 2023, 7:20 PM
 */


#include <xc.h>
#include <stdint.h>
#include "hal.h"
#include "logic.h"

const char *version = "SPI Utility\r\n"
                      " version 1.6\r\n"
                      " Type ? for help and press enter\r\n"
                      "$";

void main(void) {
    setup();
    print(version);
   
    while (1) {
        if (event_tick()) {
            
        }
        if (event_button()) {
            
        }
        const char *cmd = event_cmd();
        if (cmd) {
            on_cmd(cmd);
            enable_reception();
        }
    }
}
