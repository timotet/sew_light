

/*
 * 10/4/15
 * Light for Mom's sewing machine
 * MSP430F2003 and 10 APA10C leds via a TI SN74LV4T125 level shifter chip
 * 10/10/15
 * Changed to MSP430G2202 way cheaper!
 */


//#include <msp430f2003.h>
#include <msp430g2202.h>
#include "stdbool.h"
#include "stdint.h"


#define button  BIT3
#define clkPin  USIPE5
#define dataPin USIPE6

#define numLeds 10
static const uint8_t start[4] = { 0x00, 0x00, 0x00, 0x00 };
static const uint8_t full[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
static const uint8_t threeQuarter[4] = { 0xF8, 0xFF, 0xFF, 0xFF };
static const uint8_t half[4] = { 0xF0, 0xFF, 0xFF, 0xFF };
static const uint8_t oneQuarter[4] = { 0xE8, 0xFF, 0xFF, 0xFF };
static const uint8_t off[4] = { 0xE0, 0x00, 0x00, 0x00 };

volatile uint8_t buttonPressed = true;       //start with true so stored pressCount is executed
volatile uint8_t pressCount = 0;             //keep track of button presses

//for writing flash
uint8_t * Flash_ptr = (unsigned char *) 0x1040; //location to write in flash info segment C

void millisecDelay(int delay) {
	int i;
	for (i = 0; i < delay; i++)
		_delay_cycles(365);
}

void init_gpio(void) {

	P1DIR &= ~button;   // button is an input
	P1REN |= button;    // pull down on button
	P1OUT &= ~button;   // set pull down
	P1IES &= ~button;   // interrupt Edge Select - 0: trigger on rising edge, 1: trigger on falling edge
	//P1IES |= button;    // interrupt on falling edge
	P1IFG &= ~button;     // interrupt flag for p1.3 is off
	P1IE |= button;       // enable interrupt

	P1DIR |= 0x97;      // set remaining pins in port 1 to outputs
	P1REN = 0x97;       // enable pull down resistors
	P1OUT &= ~0X4B;     // pull down remaining pins
	P2SEL = 0x00;       // enable port 2
	P2DIR |= 0xFF;      // make all port 2 outputs
	P2REN = 0xFF;       // enable pull down resistors
	P2OUT = 0x00;       // pull port 2 low


}

void init_spi(void) {

	USICTL0 |= 0x6B;  //
	//USICTL0 |= clkPin | dataPin | USIMST | USIOE | USISWRST;
	USICTL1 |= 0X91;
	//USICTL1 |= USICKPH | USIIE | USIIFG;
	USICKCTL |= 0x88;                                      // 0xA8 divide by 32 // works with 0x88 1Mhz clk USIDIV_4
	//USICKCTL |= USIDIV_5 | USISSEL_2;                    // divide by 32 500Khz clk
	USICTL0 &= ~USISWRST;                                  // Reset & release USI
	USICNT |= 8;                                           // send 8 bits
}

/*
void start_spi(void){

	USICTL0 &= ~USISWRST;   // put spi in reset state
}

void stop_spi(void){

	USICTL0 |= USISWRST;   // put spi in reset state
}
*/

void sendStart(void) {

	uint8_t i = 0;

    //start_spi();

	for (i = 0; i < 4; i++) {
		USISRL |= start[i];        // load shift register with data to send
		USICNT |= 8;               // shift out 8 bits
		millisecDelay(1);
	}
}

void writeLeds(const uint8_t data[4]) {

	uint8_t i, j;

	sendStart();

	for (i = 0; i < numLeds; i++) {
		for (j = 0; j < 4; j++) {
			USISRL |= data[j];        // load shift register with data to send
			USICNT |= 8;              // shift out 8 bits
			millisecDelay(1);
		}
	}

	//stop_spi();
}

void eOther(void) {

	uint8_t i, j;

	sendStart();

	for (i = 0; i < numLeds; i++) {
		if (i % 2 == 0) {
			for (j = 0; j < 4; j++) {
				USISRL |= threeQuarter[j];      // load shift register with data to send
				USICNT |= 8;                    // shift out 8 bits
				millisecDelay(1);
			}
		} else {
			for (j = 0; j < 4; j++) {
				USISRL |= off[j];         // load shift register with data to send
				USICNT |= 8;              // shift out 8 bits
				millisecDelay(1);
			}
		}
	}
}

// USI interrupt service routine
#pragma vector = USI_VECTOR
__interrupt void USI_TXRX(void) {

	USICTL1 &= ~USIIFG;              // Clear pending flag

}

#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void) {

	//millisecDelay(300); 			//for button debounce
	LPM4_EXIT;
	pressCount++;                   //increment button presses
	buttonPressed = true;		    //true
	P1IFG &= ~button;               //clear interrupt flag

}

void writeFlash(unsigned char data) {

	_disable_interrupts();
	FCTL1 = FWKEY + ERASE;         //Set Erase bit
	FCTL3 = FWKEY;                 //Clear Lock bit

	*Flash_ptr = 0;                //Dummy write to erase Flash segment

	FCTL1 = FWKEY + WRT;           //Set WRT bit for write operation

	*Flash_ptr = data;             //Write value to flash

	FCTL1 = FWKEY;                 //Clear WRT bit
	FCTL3 = FWKEY + LOCK;          //Set LOCK bit
	_enable_interrupts();
}

int main(void) {

	WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

	DCOCTL = 0;
	BCSCTL1 = CALBC1_16MHZ; // Run at 16 MHz
	DCOCTL = CALDCO_16MHZ;

	init_gpio();
	init_spi();

	// Set up flash timing generator
	// this is for 16Mhz clk
	FCTL2 = FWKEY + FSSEL_1 + FN5 + FN3; // MCLK/32 + 8 = MCLK/40 = 400hz for Flash Timing Generator
	//FCTL2 = FWKEY + FSSEL_1 + 0x15;    // these both work has to be between 257-476 hz

	pressCount = *Flash_ptr; // load value written in flash
	_enable_interrupts();

	while (1) {

		if (buttonPressed) {
			buttonPressed = false;    //reset


			switch (pressCount) {
			case 0:
				writeLeds(full); //bright white
				LPM4;
				break;
			case 1:
				writeLeds(threeQuarter);
				LPM4;
				break;
			case 2:
				writeLeds(half);
				LPM4;
				break;
			case 3:
				writeLeds(oneQuarter);
				LPM4;
				break;
			case 4:
				eOther();
				LPM4;
				break;
			case 5:
				writeLeds(off);
				LPM4;
				break;
			default:
				pressCount = 0; //default to 0
				buttonPressed = true; //force execution of case 0
				break;
			}
			writeFlash(pressCount);     // write to flash for poweroff
		}
	}
}
