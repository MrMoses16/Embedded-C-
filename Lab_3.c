// The following code is from lab 3 part 3
// This code compliments everything learned in lab 3 and is based on the 
// performance of the MSP demonstrated in the following YouTube videos: 
// Pulse Repeater: https://youtu.be/ZJux_cjxh30


// Turn red LED on for duration button 1 was pressed if not exceeding 64K cycles
// If exceeding, turn green LED on and lock system until button 2 is pressed
#include <msp430fr6989.h>

#define redLED BIT0 // Red LED at P1.0
#define greenLED BIT7 // Green LED at P9.7
#define BUT1 BIT1 // Button S1 at P1.1
#define BUT2 BIT2 // Button S2 at P1.2
#define MAX 0xFFFF

void config_ACLK_to_32KHz_crystal();


void main(void){
    // Stop the Watchdog timer
    WDTCTL = WDTPW | WDTHOLD;

    // Unlock the GPIO pins
    PM5CTL0 &= ~LOCKLPM5;

    // Configure the LEDs as output
    P1DIR |= redLED;
    P9DIR |= greenLED;
    P1OUT &= ~redLED;
    P9OUT &= ~greenLED;

    // Configure button for S1
    P1DIR &= ~BUT1; // Direct pin as input
    P1REN |= BUT1; // Enable built-in resistor
    P1OUT |= BUT1; // Set resistor as pull-up

    // Configure button for S2
    P1DIR &= ~BUT2;
    P1REN |= BUT2;
    P1OUT |= BUT2;


    // Configure ACLK to the 32 KHz crystal (function call)
    config_ACLK_to_32KHz_crystal();

    // Configure Timer_A
    // Set timer period
    TA0CCR0 = 0;

    // Timer_A: ACLK, div by 1, up mode, clear TAR
    TA0CTL = (TASSEL_1 | ID_0 | MC_1 | TACLR);

    // Ensure flag is cleared at the start
    TA0CTL &= ~TAIFG;

    unsigned int count = 0;
    int pressed = 0;
    int lock = 0;
    // Infinite loop
    for(;;) {
        // unlock system if button 2 is pressed
        if((P1IN & BUT2) == 0){
            P9OUT &= ~greenLED;
            lock = 0;
        }
        // record the duration of button 1 pressed
        while(lock == 0 && (P1IN & BUT1) == 0){

            if(count < MAX)
                count++;

            pressed = 1;
        }

        // skip if button 1 was not pressed
        if(pressed != 0){
            // Turn green LED on and lock system if button 1 was pressed for more than 64K cycles
            if(count > 63998){
                P9OUT ^= greenLED;
                lock = 1;
            }
            // skip if system is locked
            if(lock == 0){
                // set Timer_A duration and turn on red LED
                TA0CCR0 = count;
                P1OUT ^= redLED;
                // delay reached
                while((TA0CTL & TAIFG) == 0);
                // turn red LED off and reset interrupt
                P1OUT ^= redLED;
                TA0CTL &= ~TAIFG;
            }
            // reset variables
            pressed = 0;
            TA0CCR0 = 0;
            count = 0;
        }
    }
}

// Configures ACLK to 32 KHz crystal
void config_ACLK_to_32KHz_crystal() {
    // By default, ACLK runs on LFMODCLK at 5MHz/128 = 39 KHz
    // Reroute pins to LFXIN/LFXOUT functionality
    PJSEL1 &= ~BIT4;
    PJSEL0 |= BIT4;

    // Wait until the oscillator fault flags remain cleared
    CSCTL0 = CSKEY; // Unlock CS registers

    do {
        CSCTL5 &=  ~LFXTOFFG; // Local fault flag
        SFRIFG1 &=  ~OFIFG; // Global fault flag
    } while((CSCTL5 & LFXTOFFG) != 0);

    CSCTL0_H = 0; // Lock CS registers

    return;
}
