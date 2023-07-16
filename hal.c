
#include "hal.h"
#include <stdint.h>

#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator: High-speed crystal/resonator on RA6/OSC2/CLKOUT and RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = ON       // Power-up Timer Enable bit (PWRT enabled)
#pragma config MCLRE = ON       // RA5/MCLR/VPP Pin Function Select bit (RA5/MCLR/VPP pin function is MCLR)
#pragma config BOREN = OFF      // Brown-out Detect Enable bit (BOD disabled)
#pragma config LVP = ON         // Low-Voltage Programming Enable bit (RB4/PGM pin has PGM function, low-voltage programming enabled)
#pragma config CPD = OFF        // Data EE Memory Code Protection bit (Data memory code protection off)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)

#ifndef INPUT_BUFFER_SIZE
#define INPUT_BUFFER_SIZE     16
#endif

#define FLAG_USART_TX   0x01
#ifdef TICK
#define FLAG_TICK_1MS   0x04
#endif

#define FLAG_CMD        0x08
#ifdef BUTTON
#define FLAG_BUTTON     0x10
#endif

#ifdef TICK
#define MILLISEC_TICK   65535 - 1825 // For 7.3728 MHz clock / 4
#endif

// Pins assignment
#ifdef TICK
#define OUT_MS          PORTAbits.RA0
#endif

#define CHAR_IN         PORTAbits.RA1
#define USART_OVERFLOW  PORTAbits.RA2

static uint8_t events;
static volatile uint8_t in;
static char input[INPUT_BUFFER_SIZE];
static uint8_t input_seek;
static const char *msg;

void __interrupt() _Interrupt(void)
{
    if (PIR1bits.RCIF) {
        in = RCREG;
        CHAR_IN = !CHAR_IN;
        if (in == '\r' || in == '\n') {
            events |= FLAG_CMD;
            input[input_seek++] = 0;
            PIE1bits.RCIE = 0;
        } else {
            input[input_seek++] = in;
            if (input_seek == INPUT_BUFFER_SIZE) {
                USART_OVERFLOW = 0; 
                PIE1bits.RCIE = 0;
            }
        }
    } 
    if (PIR1bits.TXIF) {
        if (*msg == 0) {
            PIE1bits.TXIE = 0;
            events &= ~FLAG_USART_TX;
        } else {
            TXREG = *msg++;
        }
    } 
#ifdef TICK    
    if (INTCONbits.TMR0IF) {
        TMR0 = 256 - 233;
        OUT_MS = !OUT_MS;
        INTCONbits.TMR0IF = 0;
        events |= FLAG_TICK_1MS;
    } 
#endif    
#ifdef BUTTON    
    if (INTCONbits.INTF) {
        INTCONbits.INTF = 0;
        events |= FLAG_BUTTON;
    } 
#endif    
    /*if (PIR1bits.TMR2IF) {
        PIR1bits.TMR2IF = 0;
    }*/
}

void setup() {
    // Setup output
    CMCONbits.CM = 7;       // Turn comparator off, all digital pins
    
    // Set PORTAbits.RA0 as output - clock 1 ms
    
    TRISAbits.TRISA0 = 0;
    TRISAbits.TRISA1 = 0;
    TRISAbits.TRISA2 = 0;
    TRISAbits.TRISA3 = 0;
    
#ifdef TICK    
    OUT_MS = 0; 
#endif    
    
    USART_OVERFLOW = 1;
    CHAR_IN = 1;
    
    // Enable USART
    RCSTAbits.SPEN = 1;
    RCSTAbits.SREN = 1;     // Enable serial port
    RCSTAbits.CREN = 1;
    TXSTAbits.TXEN = 1;
    
    // Setup IO
    OPTION_REGbits.nRBPU = 0;
    
#ifdef TICK    
    // Set Timer 0
    OPTION_REGbits.PS  = 2; // Prescaler 1:8
    OPTION_REGbits.PSA = 0; // Prescaler to Timer 0
    OPTION_REGbits.T0CS = 0; // Timer works from internal
#endif    
    
#ifdef PWM    
    //Set PWM
    TRISBbits.TRISB3 = 0;   // For PWM
    CCPR1L = 38;            // Arbitrary
    CCP1CONbits.CCP1M = 0xC; //PWM mode ON
    T2CONbits.T2CKPS = 1;   // 1:4 prescaler
    T2CONbits.TMR2ON = 1;   // Enable Timer 2
#endif
    
    // Enable interrupts
    PIE1bits.RCIE = 1;      // Enable USART receive interrupt
#ifdef TICK    
    INTCONbits.TMR0IE = 1;  // Enable Timer 0 interrupt
#endif    
#ifdef BUTTON    
    INTCONbits.INTE = 1;    // Enable RB0
#endif    
    INTCONbits.PEIE = 1;    // Enable peripherial interrupt
    INTCONbits.GIE = 1;     // Enable interrupt
}


void print(const char* m) {
    // Wait to finish previous message
    while (events & FLAG_USART_TX);
    
    // Wait for previous transmission to finish
    while(!TXSTAbits.TRMT);
    msg = m;
    
    events |= FLAG_USART_TX;
    // For clean transmission, set register data first
    // otherwise on interrupt it will pass first whatever
    // is in TXREG
    TXREG = *msg++;
    PIE1bits.TXIE = 1;    
}

#ifdef TICK
__bit event_tick() {
    if (events & FLAG_TICK_1MS) {
        events &= ~FLAG_TICK_1MS;
        return 1;
    }
    return 0;        
}
#endif

const char *event_cmd() {
    if (events & FLAG_CMD) {
        events &= ~FLAG_CMD;
        
        return input;
    }    
    return 0;
}

#ifdef BUTTON
__bit event_button() {
    if (events & FLAG_BUTTON) {
        events &= ~FLAG_BUTTON;   
        return 1;
    }
    return 0;
}
#endif

void enable_reception() {
    input_seek = 0;
    PIE1bits.RCIE = 1;
}