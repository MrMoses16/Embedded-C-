// The following code is from lab 4 part 5
// This code compliments everything learned in lab 4 and is based on the 
// performance of the MSP demonstrated in the following YouTube video: 
// https://youtu.be/SLjYTANZmO0


// The LED flashing pattern
// is the following. Both LEDs flashing at a slow pace indicates the vehicle is spot on and no correction is
// needed. The red LED may flash at one of three speeds indicating how much correction is needed to the
// left. Faster flashing indicates more correction is needed. The green LED works similarly by supporting
// three flashing speeds.
#include "intrinsics.h"
#include <msp430fr6989.h>

#define redLED BIT0 // Red LED at P1.0
#define greenLED BIT7 // Green LED at P9.7
#define BUT1 BIT1 // Button S1 at Port 1.1
#define BUT2 BIT2 // Button S2 at Port 1.2

volatile int blink = 0;

void config_ACLK_to_32KHz_crystal();

void main(void) {
    WDTCTL = WDTPW | WDTHOLD; // Stop the Watchdog timer
    PM5CTL0 &= ~LOCKLPM5;     // Enable the GPIO pins
    P1DIR |= redLED;          // Configure pin as output
    P9DIR |= greenLED;        // Configure pin as output
    P1OUT &= ~redLED;         // Turn red LED off
    P9OUT &= ~greenLED;       // Turn green LED off

    // Configure the buttons for interrupts
    P1DIR &=  ~(BUT1|BUT2); // 0: input
    P1REN |= (BUT1|BUT2); // 1: enable built-in resistors
    P1OUT |= (BUT1 | BUT2); // 1: built-in resistor is pulled up to Vcc
    P1IES |=  (BUT1|BUT2); // 1: interrupt on falling edge (0 for rising edge)
    P1IFG &=  ~(BUT1|BUT2); // 0: clear the interrupt flags
    P1IE |= (BUT1|BUT2); // 1: enable the interrupts

    // Configure Channel 0 for up mode with interrupts
    TA0CCR0 = 32768-1;        // 1 second @ 32 KHz
    TA0CCTL0 |= CCIE;         // Enable Channel 0 interrupt (CCIE bit)
    TA0CCTL0 &= ~CCIFG;       // Clear Channel 0 interrupt flag

    // Timer_A: ACLK, div by 1, up mode, clear TAR
    TA0CTL = (TASSEL_1 | ID_0 | MC_1 | TACLR);

    // Configure ACLK to the 32 KHz crystal
    config_ACLK_to_32KHz_crystal();

    // Enable global interrupts
    _enable_interrupts();

    // Main infinite loop
    for(;;) {
        if(blink == 0)
            TA0CCR0 = 32768-1;
        if(blink == 1 || blink == -1)
            TA0CCR0 = 32768/2-1;
        if(blink == 2 || blink == -2)
            TA0CCR0 = 32768/4-1;
        if(blink == 3 || blink == -3)
            TA0CCR0 = 32768/8-1;
    }
}

// ISR for Timer_A Channel 0 interrupt (corrected vector)
#pragma vector = TIMER0_A0_VECTOR // Correct vector for Channel 0
__interrupt void T0A0_ISR() {
    if(blink == 0){
        // Toggle red and green LEDs
        P1OUT ^= redLED;
        P9OUT ^= greenLED;

        // Clear interrupt flag for Channel 0
       
    }
    if(blink > 0){
        P9OUT &= ~greenLED;
        P1OUT ^= redLED;
       
    }
    if(blink < 0){
        P1OUT &= ~redLED;
        P9OUT ^= greenLED;
    }
    TA0CCTL0 &= ~CCIFG;
}


// ISR for Timer_A Channel 0 interrupt (corrected vector)
#pragma vector = PORT1_VECTOR
__interrupt void Port1_ISR() {
    if ((P1IFG & BUT1) != 0) {
        __delay_cycles(20000);  // Debounce delay
        if ((P1IN & BUT1) == 0) {
            blink++;
            if (blink > 3)
                blink = 3;
        }
        P1IFG &= ~BUT1;  // Clear interrupt flag
    }

    if ((P1IFG & BUT2) != 0) {
        __delay_cycles(20000);  // Debounce delay
        if ((P1IN & BUT2) == 0) {
            blink--;
            if (blink < -3)
                blink = -3;
        }
        P1IFG &= ~BUT2;  // Clear interrupt flag
    }

    if(blink == 0){
        P1OUT &= ~redLED;
        P9OUT &= ~greenLED;
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
        CSCTL5 &= ~LFXTOFFG;  // Clear local fault flag
        SFRIFG1 &= ~OFIFG;    // Clear global fault flag
    } while ((CSCTL5 & LFXTOFFG) != 0);  // Wait for oscillator to stabilize

    CSCTL0_H = 0;  // Lock CS registers
}
