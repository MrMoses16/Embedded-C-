// The following code is from lab 7 part 4
// This code compliments I2C communication between the MSP430 and
// its booster pack. The MSP uses I2C to read the lux value recorded
// by the light sensor in the booster pack and sends the data to the
// connected computers UART port using UART communication.

#include "intrinsics.h"
#include "msp430fr6989.h"
#include <msp430.h>
#define FLAGS UCA1IFG // Contains the transmit & receive flags
#define RXFLAG UCRXIFG // Receive flag
#define TXFLAG UCTXIFG // Transmit flag
#define TXBUFFER UCA1TXBUF // Transmit buffer
#define RXBUFFER UCA1RXBUF // Receive buffer
#define redLED BIT0 // Red at P1.0
#define greenLED BIT7 // Green at P9.7

// Function prototypes
int i2c_read_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int *data);
int i2c_write_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int data);
void uart_write_char(unsigned char ch);
unsigned char uart_read_char(void);
void Initialize_UART(void);
void Initialize_I2C(void);

void main(void) {
    WDTCTL = WDTPW | WDTHOLD; // Stop WDT
    PM5CTL0 &= ~LOCKLPM5; // Enable GPIO pins
    P1DIR |= redLED; // Pins as output
    P9DIR |= greenLED;
    P1OUT |= redLED; // Red on
    P9OUT &= ~greenLED; // Green off


    // Configure pins to I2C functionality
    P4SEL1 |= (BIT1 | BIT0);
    P4SEL0 &= ~(BIT1 | BIT0); 

    // Initialize I2C and UART
    Initialize_I2C();
    Initialize_UART();

    // Constructing the correct format for display on UART
    unsigned int count = 0; // Variable to store counter for UART
    unsigned int data_test; // Variable to store data read from sensor
    char countString[] = "Counter: "; // String for Seconds Interval

    // Configure the sensor
    i2c_write_word(0x44, 0x01, 0x7604); // Configuration value of the sensor

    for(;;) {
        // Read data_test from sensor
        i2c_read_word(0x44, 0x00, &data_test);

        // Delay for 1.00 seconds
        _delay_cycles(1000000);

        // Calculate lux value
        data_test = data_test * 1.28;

        // Transmit data over UART
        uart_write_uint16(data_test);
        uart_write_string("\n\r");

        // Transmit counter over UART
        uart_write_string(countString);
        uart_write_uint16(count);
        
        // Increment counter
        count++;
        uart_write_string("\n\r");
    }
}

void Initialize_I2C(void) {
    // Configure the MCU in Master mode
    // Configure pins to I2C functionality
    // (UCB1SDA same as P4.0) (UCB1SCL same as P4.1)
    // (P4SEL1=11, P4SEL0=00) (P4DIR=xx)

    P4SEL1 |= (BIT1 | BIT0);
    P4SEL0 &= ~(BIT1 | BIT0);

   // Enter reset state and set all fields in this register to zero
    UCB1CTLW0 = UCSWRST;

    // Fields that should be nonzero are changed below
    // (Master Mode: UCMST) (I2C mode: UCMODE_3) (Synchronous mode: UCSYNC)
    // (UCSSEL 1:ACLK, 2,3:SMCLK)
    UCB1CTLW0 |= UCMST | UCMODE_3 | UCSYNC | UCSSEL_3;
    
    // Clock frequency: SMCLK/8 = 1 MHz/8 = 125 KHz
    UCB1BRW = 8;

    // Chip Data Sheet p. 53 (Should be 400 KHz max)
    // Exit the reset mode at the end of the configuration
    UCB1CTLW0 &= ~UCSWRST;
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
    // Wait for any ongoing transmission to complete
    while ((FLAGS & TXFLAG)==0 ) {}

    while(n > 0){
        TXBUFFER = (n % 10) + '0';

        n /= 10;

        while ((FLAGS & TXFLAG) == 0) {}
    }

    return;
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
