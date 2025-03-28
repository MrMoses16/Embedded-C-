// The following code is from lab 1 part 5
// This code compliments everything learned in lab 1 and is based on the 
// performance of the MSP demonstrated in the following YouTube videos: 
// Pattern #1: https://youtube.com/shorts/SlZDGJTgVAo
// Pattern #2: https://youtube.com/shorts/SqBy7YAlmHA


// Code that flashes the red and green LED in alternating fashion: Pattern 1
#include <msp430fr6989.h>

#define redLED BIT0 // Red LED at P1.0
#define greenLED BIT7 // Green LED at P9.7

void main(void) {

    WDTCTL = WDTPW | WDTHOLD; // Stop the Watchdog timer
    PM5CTL0 &= ~LOCKLPM5;
    P1DIR |= redLED; // Direct pin as output
    P1OUT &= ~redLED; // Turn LED Off

    P9DIR |= greenLED; // Direct pin as output
    P9OUT &= ~greenLED; // Turn LED Off

    int cycle = 0;
    int i;
    for(;;) {
        if(cycle == 0){
            for(i = 0; i<5; i++){
                P1OUT ^= redLED; // Toggle the LED
                _delay_cycles(100000);
            }
            P1OUT &= ~redLED;
            cycle = 1;
        }else{
            for(i = 0; i<5; i++){
                P9OUT ^= greenLED; // Toggle the LED
                _delay_cycles(100000);
            }
            P9OUT &= ~greenLED;
            cycle = 0;
        }
    }
}


// Code that flashes the red and green LED in alternating fashion: Pattern 2
#include <msp430fr6989.h>
#include <stdint.h>

#define redLED BIT0 // Red LED at P1.0
#define greenLED BIT7 // Green LED at P9.7

void main(void) {

    WDTCTL = WDTPW | WDTHOLD; // Stop the Watchdog timer
    PM5CTL0 &= ~LOCKLPM5;
    P1DIR |= redLED; // Direct pin as output
    P1OUT &= ~redLED; // Turn LED Off

    P9DIR |= greenLED; // Direct pin as output
    P9OUT &= ~greenLED; // Turn LED Off

    volatile uint32_t wait = 18000; // used for the delay
    int reduce = 1000; // rate at which the delay decreases
    int x;
    volatile uint32_t i;

    for(;;){
        // when the wait delay is too small, reset the wait delay and flash both LEDs 3 times
        if(wait < 2000){
            // reset the wait delay
            wait = 18000;

            // turn both LEDS off
            P1OUT &= ~redLED;
            P9OUT &= ~greenLED;

            // wait with both lights off
            for(i = 0; i<20000; i++){}

            // flash both LEDs 3 times
            for(x = 0; x<5; x++){
                P9OUT ^= greenLED;
                P1OUT ^= redLED;

                for(i = 0; i<8000; i++){}
            }

            // turn both LED off
            P9OUT &= ~greenLED;
            P1OUT &= ~redLED;

            // wait
            for(i = 0; i<20000; i++){}
        }

        // turn the green LED off and red LED on
        P9OUT &= ~greenLED;
        P1OUT ^= redLED;

        // wait
        for(i = 0; i<wait; i++){}

        // turn the green LED on and red LED off
        P1OUT &= ~redLED;
        P9OUT ^= greenLED;

        // wait
        for(i = 0; i<wait; i++){}

        // reduce wait delay
        wait -= reduce;
       
    }
}
