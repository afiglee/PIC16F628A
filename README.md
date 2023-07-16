# PIC16F628A
Stab for PIC16F628A UART communication

This is a stab project: PIC16F628A communicates through UART at 115200 8N1 bauds.
Add your implementation.
It has 1 millisec tick on PORTA.0 if TICK is defined,
PWM output at PORTB.3 and catches interrupt on PORTB.0
if BUTTON is defined.
There is no switch bouncing protection, add 20-60 ms delay if you need.
PORTA.1 toggles on every character received.
PORTA.2 is overflow indicator.
INPUT_BUFFER_SIZE is set for 16 - this is how many chars it accepts until you hit enter. 
If more chars, PORTA.2 bit is cleared for overflow indicator and reception stops.
It accepts commans less then 16 chars and acts on pressing enter.
Simple circuit uses 7.3728 MHz crystal
