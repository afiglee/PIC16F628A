/* 
 * File:   hal.h
 * Author: m2tx9
 *
 * Created on July 16, 2023, 10:43 AM
 */
#include <xc.h>

#ifndef HAL_H
#define	HAL_H


#ifdef	__cplusplus
extern "C" {
#endif

    // Hardware setup.
    // Must be called before anything else
    void setup();
    
    // Blocks until previous message is printed
    // Otherwise starts printing and returns
    void print(const char* m);

#ifdef TICK
    // Events to be processed in the main loop
    // Tick 1 ms event
    __bit event_tick();
#endif    
    
    // Command in USART is in, if not NULL
    // If not NULL, after processing command,
    // Reception must be enabled
    // With enable_reception();
    const char *event_cmd();
    
#ifdef BUTTON    
    // Button was pressed
    __bit event_button();
#endif
    
    //To be called after command received and processed
    void enable_reception();
    
#ifdef	__cplusplus
}
#endif

#endif	/* HAL_H */

