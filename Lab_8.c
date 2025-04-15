// The following code is from lab 8 part 3
// This code compliments everything learned in lab 8 and is based on the 
// performance of the MSP demonstrated in the following YouTube video: 
// ADC: https://youtu.be/lqh7on4SMoU

#include "intrinsics.h"
#include "msp430fr6989.h"
#include <msp430.h>
#include <stdlib.h>

#define FLAGS UCA1IFG // Contains the transmit & receive flags
#define RXFLAG UCRXIFG // Receive flag
#define TXFLAG UCTXIFG // Transmit flag
#define TXBUFFER UCA1TXBUF // Transmit buffer
#define RXBUFFER UCA1RXBUF // Receive buffer
#define redLED BIT0 // Red at P1.0
#define greenLED BIT7 // Green at P9.7
#define BUT1 BIT1 // Button S1 at Port 1.1
#define ERROR 200

// Function prototypes
int i2c_read_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int *data);
int i2c_write_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int data);
void uart_write_char(unsigned char ch);
unsigned char uart_read_char(void);
void Initialize_UART(void);
void Initialize_ADC(void);

unsigned int values[4], setting = 0;
void main(void) {
    WDTCTL = WDTPW | WDTHOLD; // Stop WDT
    PM5CTL0 &= ~LOCKLPM5; // Enable GPIO pins
    P1DIR |= redLED; // Pins as output
    P1OUT |= redLED; // Red on

    // Initialize UART
    Initialize_UART();
    Initialize_ADC();

    // Configure buttons for interrupts
    P1DIR &= ~(BUT1); // Input
    P1REN |= (BUT1); // Enable resistors
    P1OUT |= (BUT1); // Pull-up
    P1IES |= (BUT1); // Falling edge interrupt
    P1IFG &= ~(BUT1); // Clear flags
    P1IE |= (BUT1); // Enable interrupts

    _enable_interrupts(); // Enable global interrupts

    // declare needed variables
    unsigned int x_pos, y_pos, X = 1, Y = 0, i, j, max = 0, new_max, flag = 0;
    int diff;

    // create platform
    uart_write_string("\x1B[2J");
    uart_write_string("\x1B[1;1H*** Platform Balancing Control ***");
    uart_write_string("\x1B[10;15HTOP");
    uart_write_string("\x1B[25;10HMove Cursor\tMax Delta");
    uart_write_string("\x1B[26;10HAdjust Value");
    uart_write_string("\x1B[25;9H*");
    uart_write_string("\x1B[11;10H---------------");
    uart_write_string("\x1B[20;10H---------------");
    uart_write_string("\x1B[12;10H|             |");
    uart_write_string("\x1B[13;9HL|             |R");
    uart_write_string("\x1B[14;9HE|             |I");
    uart_write_string("\x1B[15;9HF|  Balanced   |G");
    uart_write_string("\x1B[16;9HF|             |H");
    uart_write_string("\x1B[17;10H|             |T");
    uart_write_string("\x1B[18;10H|             |");
    uart_write_string("\x1B[19;10H|             |");
    uart_write_string("\x1B[21;13HBottom");
    uart_write_string("\x1B[26;30H");
    uart_write_uint16(max);

    // set corner values to 127 and print the values
    for(i= 0; i<4; i++)
        values[i] = 127;

    uart_write_string("\x1B[19;11H");
    uart_write_uint16(values[0]);
    uart_write_string("\x1B[19;19H");
    uart_write_uint16(values[1]);
    uart_write_string("\x1B[12;11H");
    uart_write_uint16(values[2]);
    uart_write_string("\x1B[12;19H");
    uart_write_uint16(values[3]);

    for(;;) {
        // set the ADC12SC bit to start the conversion
        ADC12CTL0 |= ADC12SC;

        // wait for ADC12BUSY bit to clear, which indicates that the result is ready.
        while((ADC12CTL1 & ADC12BUSY) != 0){}

       // get x and y positions
        x_pos = ADC12MEM0;
        y_pos = ADC12MEM1;

        // when in move cursor mode (setting = 0) adjust ordinate corner value(s) for potential movement of cursor
        if(setting == 0){
            if(x_pos < (2048 - ERROR)){
                if(X != 0)
                    X--;
            }
            if(x_pos > (2048 + ERROR)){
                if(X < 1)
                    X++;
            }
            if(y_pos < (1800 - ERROR)){
                if(Y != 0)
                    Y--;
            }
            if(y_pos > (1800 + ERROR)){
                if(Y < 1)
                    Y++;
            }
        // when in adjust value mode (setting = 1) adjust the corner value
        }else{
            if(X == 1 && Y == 1){
                if(x_pos > (2048 + ERROR))
                    values[0]++;
                if(x_pos < (2048 - ERROR))
                    values[0]--;
            }
            if(X == 1 && Y == 0){
                if(x_pos > (2048 + ERROR))
                    values[1]++;
                if(x_pos < (2048 - ERROR))
                    values[1]--;
            }
            if(X == 0 && Y == 1){
                if(x_pos > (2048 + ERROR))
                    values[2]++;
                if(x_pos < (2048 - ERROR))
                    values[2]--;
            }
            if(X == 0 && Y == 0){
                if(x_pos > (2048 + ERROR))
                    values[3]++;
                if(x_pos < (2048 - ERROR))
                    values[3]--;
            }
        }

       // find max difference between all corners
        new_max = 0;
        for(i = 0; i<4; i++){
            for(j = i; j<4; j++){
                diff = values[i] - values[j];
                if(abs(diff) > max)
                    new_max = abs(diff);
            }
        }

        // print max difference if new max difference was found
        if(max != new_max){
            uart_write_string("\x1B[26;30H   ");
            uart_write_string("\x1B[26;30H");
            uart_write_uint16(max);
            max = new_max;
        }

        // if max difference is greater than 10, print danger, else print balanced
        // flag used to minimize cursor movement
        if(max > 10 && flag == 0){
            uart_write_string("\x1B[15;10H|   Danger    |");
            flag = 1;
        }else if(flag == 1 && max < 11){
            uart_write_string("\x1B[15;10H|  Balanced   |");
            flag = 0;
        }

        // print corner values
        if(X == 1 && Y == 1){
            uart_write_string("\x1B[12;19H");
            uart_write_uint16(values[0]);
        }
        if(X == 1 && Y == 0){
            uart_write_string("\x1B[19;19H");
            uart_write_uint16(values[1]);
        }
        if(X == 0 && Y == 1){
            uart_write_string("\x1B[12;11H");
            uart_write_uint16(values[2]);
        }
        if(X == 0 && Y == 0){
            uart_write_string("\x1B[19;11H");
            uart_write_uint16(values[3]);
        }

        P1OUT ^= redLED;
        _delay_cycles(500000);
    }
}

// Button Interrupt Service Routine
#pragma vector = PORT1_VECTOR
__interrupt void Port1_ISR(void) {
    if ((P1IFG & BUT1) != 0) { // Button 1 pressed
        // changed setting type
        if(setting == 1){
            setting = 0;
            uart_write_string("\x1B[25;9H*");
            uart_write_string("\x1B[26;9H ");
        }else{
            setting = 1;
            uart_write_string("\x1B[25;9H ");
            uart_write_string("\x1B[26;9H*");
        }
        P1IFG &= ~BUT1; // Clear interrupt flag
    }
}

void Initialize_ADC() {
    // Configure the pins to analog functionality
    // X-axis: A10/P9.2, for A10 (P9DIR=x, P9SEL1=1, P9SEL0=1)
    P9SEL1 |= BIT2;
    P9SEL0 |= BIT2;

    // Y-axis: J3.26 / P8.7
    P8SEL1 |= BIT7;
    P8SEL0 |= BIT7;

    // Turn on the ADC module
    ADC12CTL0 |= ADC12ON;
    // Turn off ENC (Enable Conversion) bit while modifying the configuration
    ADC12CTL0 &= ~ADC12ENC;

    //*************** ADC12CTL0 ***************
    // Set ADC12SHT0 (select the number of cycles that you computed)
    // (10000 + 10000) * (0.000000001 + 0.000000015) * ln(2^(12+1)) = 2.89 us
    ADC12CTL0 |= ADC12SHT0_2;
    ADC12CTL0 |= ADC12MSC;

    //*************** ADC12CTL1 ***************
    // Set ADC12SHS (select ADC12SC bit as the trigger)
    // Set ADC12SHP bit
    // Set ADC12DIV (select the divider you determined)
    // Set ADC12SSEL (select MODOSC)
    ADC12CTL1 |= ADC12SHS_0 | ADC12SHP | ADC12DIV_7 | ADC12SSEL_0;
    ADC12CTL1 |= ADC12CONSEQ_1;

    //*************** ADC12CTL2 ***************
    // Set ADC12RES (select 12-bit resolution)
    // Set ADC12DF (select unsigned binary format)
    ADC12CTL2 = ADC12RES_2; // 12-bit resolution
    ADC12CTL2 &= ~ADC12DF; // Unsigned binary format

    //*************** ADC12CTL3 ***************
    // Set ADC12CSTARTADD to 0 (first conversion in ADC12MEM0)
    ADC12CTL3 |= ~ADC12CSTARTADD_0;


    //*************** ADC12MCTL0 ***************
    // Set ADC12VRSEL (select VR+=AVCC, VR-=AVSS)
    // Set ADC12INCH (select channel A10)
    ADC12MCTL0 |= ADC12VRSEL_0 | ADC12INCH_10;

    ADC12MCTL1 |= ADC12VRSEL_0 | ADC12INCH_4 | ADC12EOS;

    // Turn on ENC (Enable Conversion) bit at the end of the configuration
    ADC12CTL0 |= ADC12ENC;
    return;
}

// Configure UART to the popular configuration
// 9600 baud, 8-bit data, LSB first, no parity bits, 1 stop bit
// no flow control, oversampling reception
// Clock: SMCLK @ 1 MHz (1,000,000 Hz)
void Initialize_UART(void){
    // Configure pins to UART functionality
    P3SEL1 &= ~(BIT4|BIT5);
    P3SEL0 |= (BIT4|BIT5);

    // Main configuration register

    UCA1CTLW0 = UCSWRST; // Engage reset; change all the fields to zero
    // Most fields in this register, when set to zero, correspond to the
    // popular configuration

    UCA1CTLW0 |= UCSSEL_2; // Set clock to SMCLK
    // Configure the clock dividers and modulators (and enable oversampling)

    UCA1BRW = 6; // divider
    // Modulators: UCBRF = 8 = 1000 --> UCBRF3 (bit #3)
    // UCBRS = 0x20 = 0010 0000 = UCBRS5 (bit #5)

    UCA1MCTLW = UCBRF3 | UCBRS5 | UCOS16;
    // Exit the reset state

    UCA1CTLW0 &= ~UCSWRST;
}

void uart_write_char(unsigned char ch){
    // Wait for any ongoing transmission to complete
    while ((FLAGS & TXFLAG)==0 ) {}

    // Copy the byte to the transmit buffer
    TXBUFFER = ch;
    // Tx flag goes to 0 and Tx begins!

    return;
}

unsigned char uart_read_char(void) {
    unsigned char temp;
    // Return null character (ASCII=0) if no byte was received
    if ((FLAGS & RXFLAG) == 0)
        return 0;
    // Otherwise, copy the received byte (this clears the flag) and return it
    temp = RXBUFFER;
    return temp;
}

void uart_write_string (char * str){
    // Wait for any ongoing transmission to complete
    while ((FLAGS & TXFLAG)==0 ) {}

    unsigned int index = 0;

    while(str[index] != '\0'){
        unsigned char ch = str[index];
        TXBUFFER = str[index];
        while ((FLAGS & TXFLAG)==0 ) {}
        index++;
    }

    return;
}
void uart_write_uint16 (unsigned int n){
    char buffer[6]; // Max 5 digits + null terminator
    int i = 0;

    if (n == 0) {
        uart_write_char('0');
        return;
    }

    // Extract digits into buffer
    while(n > 0) {
        buffer[i++] = (n % 10) + '0';
        n /= 10;
    }

    // Print digits in correct order (reverse the buffer)
    while(i > 0) {
        uart_write_char(buffer[--i]);
    }
}


// Read a word (2 bytes) from I2C (address, register)
int i2c_read_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int *data) {
    unsigned char byte1=0, byte2=0; // Intialize to ensure successful reading

    UCB1I2CSA = i2c_address; // Set address
    UCB1IFG &= ~UCTXIFG0;

    // Transmit a byte (the internal register address)
    UCB1CTLW0 |= UCTR;
    UCB1CTLW0 |= UCTXSTT;

    while((UCB1IFG & UCTXIFG0)==0) {} // Wait for flag to raise
    UCB1TXBUF = i2c_reg; // Write in the TX buffer

    while((UCB1IFG & UCTXIFG0)==0) {} // Buffer copied to shift register; Tx in progress; set Stop bit

    // Repeated Start
    UCB1CTLW0 &= ~UCTR;
    UCB1CTLW0 |= UCTXSTT;

    // Read the first byte
    while((UCB1IFG & UCRXIFG0)==0) {} // Wait for flag to raise
    byte1 = UCB1RXBUF;

    // Assert the Stop signal bit before receiving the last byte
    UCB1CTLW0 |= UCTXSTP;

    // Read the second byte
    while((UCB1IFG & UCRXIFG0)==0) {} // Wait for flag to raise
    byte2 = UCB1RXBUF;

    while((UCB1CTLW0 & UCTXSTP)!=0) {}
    while((UCB1STATW & UCBBUSY)!=0) {}

    *data = (byte1 << 8) | (byte2 & (unsigned int)0x00FF);
    return 0;
}

// Write a word (2 bytes) to I2C (address, register)
int i2c_write_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int data) {
    unsigned char byte1, byte2;

    UCB1I2CSA = i2c_address; // Set I2C address

    byte1 = (data >> 8) & 0xFF; // MSByte
    byte2 = data & 0xFF; // LSByte

    UCB1IFG &= ~UCTXIFG0;

    // Write 3 bytes
    UCB1CTLW0 |= (UCTR | UCTXSTT);

    while( (UCB1IFG & UCTXIFG0) == 0) {}
    UCB1TXBUF = i2c_reg;

    while( (UCB1IFG & UCTXIFG0) == 0) {}
    UCB1TXBUF = byte1;

    while( (UCB1IFG & UCTXIFG0) == 0) {}
    UCB1TXBUF = byte2;

    while( (UCB1IFG & UCTXIFG0) == 0) {}

    UCB1CTLW0 |= UCTXSTP;
    while( (UCB1CTLW0 & UCTXSTP) != 0 ) {}
    while((UCB1STATW & UCBBUSY)!=0) {}

    return 0;
}
