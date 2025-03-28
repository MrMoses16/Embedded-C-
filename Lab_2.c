// The following code is from lab 2 part 4
// This code compliments everything learned in lab 2 and is based on the 
// performance of the MSP demonstrated in the following description: 
// The idea here is that the operators should never make simultaneous requests. If a machine is running and the
// other operator makes a request, the running machine stops immediately and no machine can turn on until
// both operators release their requests (i.e. both buttons are released)

// Electric Generator Load Control (v2)
#include <msp430fr6989.h>
#define redLED BIT0 // Red LED at P1.0
#define greenLED BIT7 // Green LED at P9.7
#define BUT1 BIT1 // Button S1 at P1.1
#define BUT2 BIT2 // Button S2 at P1.2
void main(void) {
    WDTCTL = WDTPW | WDTHOLD; // Stop the Watchdog timer
    PM5CTL0 &= ~LOCKLPM5; // Enable the GPIO pins
    // Configure and initialize LEDs
    P1DIR |= redLED; // Direct pin as output
    P9DIR |= greenLED; // Direct pin as output
    P1OUT &=  ~redLED; // Turn LED Off;
    P9OUT &=  ~greenLED; // Turn LED Off

    // Configure button for S1
    P1DIR &= ~BUT1; // Direct pin as input
    P1REN |= BUT1; // Enable built-in resistor
    P1OUT |= BUT1; // Set resistor as pull-up

    // Configure button for S2
    P1DIR &= ~BUT2;
    P1REN |= BUT2;
    P1OUT |= BUT2;

    // Polling the button in an infinite loop
    unsigned int grant = 0;
    unsigned int block = 0;
    for(;;) {
        if((P1IN & BUT1) == 0 && (P1IN & BUT2) == 0){
            block = 1;
        }

        if((P1IN & BUT1) != 0 && (P1IN & BUT2) != 0){
            block = 0;
        }

        if ((P1IN & BUT1) == 0 && grant == 0 && block == 0){
            P1OUT |= redLED;
        }else{
            grant = 1;
            P1OUT &= ~redLED;
        }

        if ((P1IN & BUT2) == 0 && grant == 1 && block == 0){
            P9OUT |= greenLED;
        }else{
            grant = 0;
            P9OUT &= ~greenLED;
        }
    }
}
