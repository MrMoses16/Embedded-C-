// The following code is from lab 6 part 4
// This code compliments everything learned in lab 6 and is based on the 
// performance of the MSP demonstrated in the following YouTube video: 
// UART: https://youtu.be/tdRD_unS3xk


#include "intrinsics.h"
#include <stdint.h>
#include <msp430fr6989.h>

#define FLAGS UCA1IFG // Contains the transmit & receive flags
#define RXFLAG UCRXIFG // Receive flag
#define TXFLAG UCTXIFG // Transmit flag
#define TXBUFFER UCA1TXBUF // Transmit buffer
#define RXBUFFER UCA1RXBUF // Receive buffer
#define redLED BIT0 // Red at P1.0
#define greenLED BIT7 // Green at P9.7
#define BUT1 BIT1 // Button S1 at Port 1.1
#define BUT2 BIT2 // Button S2 at Port 1.2

void uart_write_char(unsigned char ch);
void uart_write_string(char *str);
unsigned char uart_read_char(void);
void Initialize_UART(void);
void config_ACLK_to_32KHz_crystal();

volatile unsigned int but1_press = 0; // Track button presses
volatile unsigned int but2_press = 0; // Track button presses


int main(){
    WDTCTL = WDTPW | WDTHOLD; // Stop WDT
    PM5CTL0 &= ~LOCKLPM5; // Enable GPIO pins
    P1DIR |= redLED; // Red LED as output
    P9DIR |= greenLED;
    P1OUT &= ~redLED; // Red LED off
    P9OUT &= ~greenLED; // Green LED off

    // Configure buttons for interrupts
    P1DIR &= ~(BUT1 | BUT2); // Input
    P1REN |= (BUT1 | BUT2); // Enable resistors
    P1OUT |= (BUT1 | BUT2); // Pull-up
    P1IES |= (BUT1 | BUT2); // Falling edge interrupt
    P1IFG &= ~(BUT1 | BUT2); // Clear flags
    P1IE |= (BUT1 | BUT2); // Enable interrupts

    config_ACLK_to_32KHz_crystal();

    // Configure Timer_A for 1s interval using ACLK (32.768 kHz)
    TA0CCR0 = 32768 - 1; // 1-second interval
    TA0CCTL0 = CCIE; // Enable Timer ISR
    TA0CTL = TASSEL_1 | MC_1 | TACLR; // ACLK, Up Mode

    _enable_interrupts(); // Enable global interrupts

    Initialize_UART();

    // Clear screen and setup UI
    uart_write_string("\x1B[2J\x1B[1;1HOrlando Airport");
    uart_write_string("\x1B[5;10HRunway 1\tRunway 2\n\r");
    uart_write_string("Request:\t1\t3\n\rForfeit:\t7\t9");
    uart_write_string("\x1B[10;1HRunway 1\t\tRunway 2");

    unsigned char input;
    for(;;){
        input = uart_read_char();
       
        if (input == '1') {
            uart_write_string("\x1B[11;1HREQUESTED");
            P1OUT |= redLED;
        } else if (input == '3') {
            uart_write_string("\x1B[11;30HREQUESTED");
            P9OUT |= greenLED;
        } else if (input == '7') {
            uart_write_string("\x1B[11;1H          "); // Clear Runway 1 Request
            uart_write_string("\x1B[12;1H          "); // Clear Runway 1 "IN USE"
            uart_write_string("\x1B[13;1H                "); // Clear "*** INQUIRY ***"
            but1_press = 0;
            P1OUT &= ~redLED;
        } else if (input == '9') {
            uart_write_string("\x1B[11;30H          "); // Clear Runway 2 Request
             uart_write_string("\x1B[11;30H          "); // Clear Runway 1 Request
            uart_write_string("\x1B[12;30H          "); // Clear Runway 1 "IN USE"
            uart_write_string("\x1B[13;30H                "); // Clear "*** INQUIRY ***"
            but2_press = 0;
            P9OUT &= ~greenLED;
        }

        _delay_cycles(1000);
    }
}

// Timer A0 ISR (Toggles LED every second if Runway 1 is in use)
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A_ISR(void) {
    if (but1_press != 0) {
        P1OUT ^= redLED; // Toggle red LED
    }

    if (but2_press != 0) {
        P9OUT ^= greenLED; // Toggle red LED
    }
}

// Button Interrupt Service Routine
#pragma vector = PORT1_VECTOR
__interrupt void Port1_ISR(void) {
    if ((P1IFG & BUT1) != 0) { // Button 1 pressed
        but1_press++;
        if (but1_press == 1) {
            uart_write_string("\x1B[12;1HIN USE");
        } else if (but1_press == 2) {
            uart_write_string("\x1B[13;1H*** INQUIRY ***");
        }
        P1IFG &= ~BUT1; // Clear interrupt flag
    }

    if ((P1IFG & BUT2) != 0) { // Button 1 pressed
        but2_press++;
        if (but2_press == 1) {
            uart_write_string("\x1B[12;30HIN USE");
        } else if (but2_press == 2) {
            uart_write_string("\x1B[13;30H*** INQUIRY ***");
        }
        P1IFG &= ~BUT2; // Clear interrupt flag
    }
}

void uart_write_string(char *str) {
    unsigned int index = 0;
    while (str[index] != '\0') {
        while (!(FLAGS & TXFLAG)) {}  // Wait for TX buffer
        TXBUFFER = str[index++];
        _delay_cycles(1000); // Small delay to avoid loss of data
    }
}

void uart_write_char(unsigned char ch){
    while (!(FLAGS & TXFLAG)) {}
    TXBUFFER = ch;
}

unsigned char uart_read_char(void){
    while (!(FLAGS & RXFLAG)) {} // Wait for data
    return RXBUFFER; // Return received char
}

void Initialize_UART(void){
    P3SEL1 &= ~(BIT4 | BIT5);
    P3SEL0 |= (BIT4 | BIT5);
   
    UCA1CTLW0 = UCSWRST; // Reset
    UCA1CTLW0 |= UCSSEL_2; // SMCLK
   
    UCA1BRW = 6;
    UCA1MCTLW = UCBRF3 | UCBRS5 | UCOS16;
   
    UCA1CTLW0 &= ~UCSWRST; // Release from reset
}

void config_ACLK_to_32KHz_crystal() {
    PJSEL1 &= ~BIT4;
    PJSEL0 |= BIT4;
   
    CSCTL0 = CSKEY;
    do {
        CSCTL5 &= ~LFXTOFFG;
        SFRIFG1 &= ~OFIFG;
    } while (CSCTL5 & LFXTOFFG);
    CSCTL0_H = 0;
} 
