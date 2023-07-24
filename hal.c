
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
#define OUT_MS_TRICK    TRISAbits.TRISA0
#endif

#ifdef SPI
#define SPI_DATA_IN         PORTAbits.RA4
#define SPI_DATA_IN_TRIS    TRISAbits.TRISA4

#define SPI_DATA_OUT        PORTAbits.RA3
#define SPI_DATA_OUT_TRIS   TRISAbits.TRISA3

#define SPI_CLCK            PORTAbits.RA0
#define SPI_CLCK_TRIS       TRISAbits.TRISA0

#define SPI_CS              PORTBbits.RB5
#define SPI_CS_TRIS         TRISBbits.TRISB5             
#endif

#define CHAR_IN         PORTAbits.RA1
#define USART_OVERFLOW  PORTAbits.RA2

static uint8_t events;
static volatile uint8_t in;
static char input[INPUT_BUFFER_SIZE];
static uint8_t input_seek;
static const char *msg;

void __interrupt() _Interrupt(void) {
    if (PIR1bits.RCIF) {
        in = RCREG;
        CHAR_IN = !CHAR_IN;
        if (in == 127) { //backspace
            if (input_seek) {
                input[input_seek--] = 0;
            }
        } else if (in == '\r' || in == '\n') {
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

}

void setup() {
    // Setup output
    CMCONbits.CM = 7; // Turn comparator off, all digital pins

    // Setup IO
    OPTION_REGbits.nRBPU = 0;

#ifdef TICK    
    OUT_MS_TRICK = 0;
#endif    

    TRISAbits.TRISA1 = 0;
    TRISAbits.TRISA2 = 0;
#ifdef SPI
    SPI_CS_TRIS = 0;
    SPI_CS = 1;
    SPI_DATA_OUT_TRIS = 0;
    SPI_DATA_OUT = 0;
    SPI_CLCK_TRIS = 0;
    SPI_CLCK = 0;
    spi_set_mode(spi_mode_3);
    
#endif    

#ifdef TICK    
    OUT_MS = 0;
#endif    

    USART_OVERFLOW = 1;
    CHAR_IN = 1;

    // Enable USART
    RCSTAbits.SPEN = 1;
    RCSTAbits.SREN = 1; // Enable serial port
    RCSTAbits.CREN = 1;
    TXSTAbits.TXEN = 1;


#ifdef TICK    
    // Set Timer 0
    OPTION_REGbits.PS = 2; // Prescaler 1:8
    OPTION_REGbits.PSA = 0; // Prescaler to Timer 0
    OPTION_REGbits.T0CS = 0; // Timer works from internal
#endif    

#ifdef PWM    
    //Set PWM
    TRISBbits.TRISB3 = 0; // For PWM
    CCPR1L = 38; // Arbitrary
    CCP1CONbits.CCP1M = 0xC; //PWM mode ON
    T2CONbits.T2CKPS = 1; // 1:4 prescaler
    T2CONbits.TMR2ON = 1; // Enable Timer 2
#endif

    // Enable interrupts
    PIE1bits.RCIE = 1; // Enable USART receive interrupt
#ifdef TICK    
    INTCONbits.TMR0IE = 1; // Enable Timer 0 interrupt
#endif    
#ifdef BUTTON    
    INTCONbits.INTE = 1; // Enable RB0
#endif    
    INTCONbits.PEIE = 1; // Enable peripherial interrupt
    INTCONbits.GIE = 1; // Enable interrupt
}

void wait_for_printer() {
    // Wait to finish previous message
    while (events & FLAG_USART_TX);

    // Wait for previous transmission to finish
    while (!TXSTAbits.TRMT);

}

void print(const char* m) {
    wait_for_printer();

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

#ifdef SPI
static enum spi_mode s_mode = spi_mode_3;

void spi_cs_down() {
    SPI_CS = 0;
}

void spi_cs_up() {
    SPI_CS = 1;
}

void spi_set_mode(enum spi_mode new_mode) {
    s_mode = new_mode;
    switch (s_mode) {
        case spi_mode_0:
        case spi_mode_1:
            SPI_CLCK = 0;
            break;
        case spi_mode_2:
        case spi_mode_3:
            SPI_CLCK = 1;
            break;
    }
}

enum spi_mode spi_get_mode() {
    return s_mode;
}

uint8_t spi_exchange(uint8_t _data) {
    uint8_t ret = 0;
    
    uint8_t data = _data;

    switch (s_mode){
        case spi_mode_1: 
        case spi_mode_3:    
        SPI_CLCK = !SPI_CLCK;
        default:
            break;
    }
    for (int8_t count = 7; count >= 0; count--) {
        ret <<= 1;
        if (data & 0x80) {
             SPI_DATA_OUT = 1;
        } else {
             SPI_DATA_OUT = 0;
        }
        SPI_CLCK = !SPI_CLCK;
        __asm ("nop");
        __asm ("nop");
        ret |= SPI_DATA_IN;
        if (count > 0) {
            SPI_CLCK = !SPI_CLCK;
            data <<= 1;
        } else {
            switch (s_mode){ 
                case spi_mode_0: 
                case spi_mode_2:    
                SPI_CLCK = !SPI_CLCK;
                default:
                    break;
            }

        }
    }
    
    return ret;
}


#endif