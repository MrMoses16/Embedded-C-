// The following code is from lab 5 part 3
// This code compliments everything learned in lab 5 and is based on the 
// performance of the MSP demonstrated in the following YouTube video: 
// Utility chronometer: https://youtu.be/WflUzULpUeQ


#include <stdint.h>
#include <msp430fr6989.h>

#define redLED BIT0  // Red at P1.0
#define greenLED BIT7 // Green at P9.7
#define BUT1 BIT1 // Button S1 at Port 1.1
#define BUT2 BIT2 // Button S2 at Port 1.2

void Initialize_LCD();
void config_ACLK_to_32KHz_crystal();
void LCD_Display_Counter(uint32_t counter);

const unsigned char LCD_Shapes[10] = {0xFC, 0x60, 0xDB, 0xF3, 0x67, 0xB7, 0xBF, 0xE0, 0xFF, 0xE7};

// Counter variable
volatile uint32_t counter = 356350;
volatile int lock = 1;

int main(void) {
    WDTCTL = WDTPW | WDTHOLD; // Stop WDT
    PM5CTL0 &= ~LOCKLPM5; // Enable GPIO pins
    P1DIR |= redLED; // Set LED as output
    P9DIR |= greenLED;
    P1OUT |= redLED; // Red LED ON
    P9OUT &= ~greenLED; // Green LED OFF

    // Configure ACLK to 32 KHz crystal
    config_ACLK_to_32KHz_crystal();

    // Configure the buttons for interrupts
    P1DIR &=  ~(BUT1|BUT2); // 0: input
    P1REN |= (BUT1|BUT2); // 1: enable built-in resistors
    P1OUT |= (BUT1 | BUT2); // 1: built-in resistor is pulled up to Vcc
    P1IES |=  (BUT1|BUT2); // 1: interrupt on falling edge (0 for rising edge)
    P1IFG &=  ~(BUT1|BUT2); // 0: clear the interrupt flags
    P1IE |= (BUT1|BUT2); // 1: enable the interrupts


    // Configure Timer_A for 1s interval using ACLK (32.768 kHz)
    TA0CCR0 = 32768 - 1; // Set compare value for 1-second interval
    TA0CCTL0 = CCIE; // Enable Timer_A CCR0 interrupt
    TA0CTL = TASSEL_1 | MC_1 | TACLR; // ACLK, Up Mode, Clear Timer

    // Enable interrupts globally
    _enable_interrupts();

    // Initialize the LCD
    Initialize_LCD();
    LCDCMEMCTL = LCDCLRM; // Clear all LCD segments
    LCDM7 = 0x04; // colon hour:min
    LCDM20 = 0x01; // period min:sec

    // Main loop
    for(;;){}
}

// Timer A0 ISR (Triggers every 1 second)
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A_ISR(void) {
    if(lock == 1){
        counter++; // Increment counter every second
        LCD_Display_Counter(counter); // Update LCD display
    }
}

uint16_t duration = 0;
uint16_t press_start = 0;
uint16_t button_held = 0;
// Port 1 ISR for button presses
#pragma vector = PORT1_VECTOR
__interrupt void Port1_ISR(void) {
    // Records the start time of button 1 being pressed
    if ((P1IFG & BUT1) && (P1IES & BUT1)) {
        press_start = TA0R; // Record start time
        button_held = 1;
        P1IES &= ~BUT1; // Switch to detect rising edge
        P1IFG &= ~BUT1; // Clear flag
    }
    // Button 1 Release Detected
    else if ((P1IFG & BUT1) && !(P1IES & BUT1)) {
        duration = TA0R - press_start; // Calculate duration
        button_held = 0;
        P1IES |= BUT1; // Switch back to detect falling edge
        P1IFG &= ~BUT1; // Clear flag

        // If held for more than 1 sec (32768 cycles), reset counter
        if (duration > 32768) {
            counter = 0;
            LCD_Display_Counter(counter);
        } else {
            lock = !lock; // Toggle pause
        }
    }

    // When button 2 is pressed the counter increments by 1
    // When button 2 and 1 are pressed at the same time, counter decrements by 1
    if ((P1IN & BUT2) == 0) {
        while((P1IN & BUT2) == 0){
            if((P1IN & BUT1) != 0){
                counter++;
                LCD_Display_Counter(counter);
            }else{
                counter--;
                LCD_Display_Counter(counter);
            }
        }
        counter += 1000; // Increment by 1000
        LCD_Display_Counter(counter);
        P1IFG &= ~BUT2; // Clear interrupt flag
    }
}

// Function to display counter on LCD
void LCD_Display_Counter(uint32_t counter) {
    // calculates the number of seconds, minutes, and hours recorded by the counter
    unsigned int sec = counter % 60;
    unsigned int min = (counter / 60) % 60;
    unsigned int hour = (counter / 3600);
    unsigned int digit1 = sec % 10;
    unsigned int digit2 = (sec / 10) % 10;
    unsigned int digit3 = min % 10;
    unsigned int digit4 = (min / 10) % 10;
    unsigned int digit5 = hour % 10;
    unsigned int digit6 = (hour / 10) % 10;
    
    // prints the corresponding value of sec, min, hour in the correct position
    LCDM10 = LCD_Shapes[digit6];
    LCDM6 = LCD_Shapes[digit5];
    LCDM4 = LCD_Shapes[digit4];
    LCDM19 = LCD_Shapes[digit3];
    LCDM15 = LCD_Shapes[digit2];
    LCDM8 = LCD_Shapes[digit1];
}

// LCD Initialization
void Initialize_LCD() {
    PJSEL0 = BIT4 | BIT5; // Select crystal for LFXT
    LCDCPCTL0 = 0xFFD0;
    LCDCPCTL1 = 0xF83F;
    LCDCPCTL2 = 0x00F8;

    CSCTL0_H = CSKEY >> 8; // Unlock CS registers
    CSCTL4 &= ~LFXTOFF; // Enable LFXT

    do {
        CSCTL5 &= ~LFXTOFFG;
        SFRIFG1 &= ~OFIFG;
    } while (SFRIFG1 & OFIFG); // Check oscillator faults

    CSCTL0_H = 0; // Lock CS registers

    // Initialize LCD
    LCDCCTL0 = LCDDIV__1 | LCDPRE__16 | LCD4MUX | LCDLP;
    LCDCVCTL = VLCD_1 | VLCDREF_0 | LCDCPEN;
    LCDCCPCTL = LCDCPCLKSYNC;
    LCDCMEMCTL = LCDCLRM; // Clear LCD memory
    LCDCCTL0 |= LCDON; // Turn on LCD
}

// Configures ACLK to 32 KHz crystal
void config_ACLK_to_32KHz_crystal() {
    PJSEL1 &= ~BIT4;
    PJSEL0 |= BIT4;

    CSCTL0 = CSKEY; // Unlock CS registers
    do {
        CSCTL5 &= ~LFXTOFFG;
        SFRIFG1 &= ~OFIFG;
    } while ((CSCTL5 & LFXTOFFG) != 0);
    CSCTL0_H = 0; // Lock CS registers
}
