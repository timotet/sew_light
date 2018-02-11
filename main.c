

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
#include "stdlib.h"

#define RGB  // comment out for white leds
#define numColors 1972


#define button  BIT3
#define clkPin  USIPE5
#define dataPin USIPE6

#define numLeds 10
//static uint8_t z[3 * numLeds]; //RGB values for 10 LEDs

static const uint8_t start[4] = { 0x00, 0x00, 0x00, 0x00 };
static const uint8_t off[4] = { 0xE0, 0x00, 0x00, 0x00 };
static const uint8_t white[4] = { 0xFF, 0xFF, 0xFF, 0xFF };

static const uint8_t threeQuarter[4] = { 0xF8, 0xFF, 0xFF, 0xFF };
static const uint8_t half[4] = { 0xF0, 0xFF, 0xFF, 0xFF };
static const uint8_t oneQuarter[4] = { 0xE8, 0xFF, 0xFF, 0xFF };

static const uint8_t blue[4] = { 0xFF, 0xFF, 0x00, 0x00 };
static const uint8_t green[4] = { 0xFF, 0x00, 0xFF, 0x00 };
static const uint8_t red[4] = { 0xFF, 0x00, 0x00, 0xFF };
static const uint8_t purple[4] = { 0xFF, 0xA8, 0x18, 0x97 };
static const uint8_t orange[4] = { 0xFF, 0x0C, 0x10, 0xFF };

volatile uint8_t buttonPressed = true;       // start with true so stored pressCount is executed
volatile uint8_t pressCount = 0;             // keep track of button presses

//for writing flash
uint8_t * Flash_ptr = (unsigned char *) 0x1040; // location to write in flash info segment C

void millisecDelay(int delay) {
	int i;
	for (i = 0; i < delay; i++)
		_delay_cycles(365);
}

void init_gpio(void) {

	P1DIR &= ~button;     // button is an input
	P1REN |= button;      // pull down on button
	P1OUT &= ~button;     // set pull down
	P1IES &= ~button;     // interrupt Edge Select - 0: trigger on rising edge, 1: trigger on falling edge
	//P1IES |= button;    // interrupt on falling edge
	P1IFG &= ~button;     // interrupt flag for p1.3 is off
	P1IE |= button;       // enable interrupt

	//P1DIR |= 0xF7;      // set remaining pins in port 1 to outputs
	//P1REN |= 0xF7;      // enable pull down resistors
	//P1OUT &= ~0XF7;     // pull down remaining pins
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
	USICKCTL |= 0x88;                                      // 16Mhz 0xA8 divide by 32 500Khz clk // 1Mhz clk 0x88
	//USICKCTL |= USIDIV_5 | USISSEL_2;                    // 0x68 divide by 8 2Mhz
	USICTL0 &= ~USISWRST;                                  // Reset & release USI
	USICNT |= 8;                                           // send 8 bits
}

/*
void start_spi(void){

	USICTL0 &= ~USISWRST;   // put spi in release state
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

	for (i = 0; i <= numLeds; i++) {
		for (j = 0; j < 4; j++) {
			USISRL |= data[j];        // load shift register with data to send
			USICNT |= 8;              // shift out 8 bits
			millisecDelay(1);
		}
	}

	//stop_spi();
}

void eOther(uint8_t intensity) {

	uint8_t i, j;
	uint8_t aTemp[4] = {0};

	aTemp[0] = intensity;
	for ( i = 1; i < 4; i++) {
		aTemp[i] = 0xFF;
	}

	sendStart();

	for (i = 0; i <= numLeds; i++) {
		if (i % 2 == 0) {
			for (j = 0; j < 4; j++) {
				USISRL |= aTemp[j];      // load shift register with data to send
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
/*
void loadLED(int ledNum, uint8_t R, uint8_t G, uint8_t B) //load z[] for one LED
{
	uint8_t i = 0;
	for (i = 0; i < ledNum; i++) {
		z[0] = 0xFF;                     // full intensity
		z[1] = G;
		z[2] = R;
		z[3] = B;

	}
}
*/

void getRGB(int color, uint8_t *R, uint8_t *G, uint8_t *B) //this generates RGB values for 1972 rainbow colors from red to violet and back to red
{
	//float brightness = 0.3; //scale factor to dim LEDs. if they are too bright, color washes out

	if (color >= (numColors / 2)) //adjust color for return from violet to red
		color = numColors - color;
	if (color < 199) {
		*R = 255;
		*G = 56 + color;
		*B = 57;
	} else if (color < 396) {
		*R = 254 - (color - 199);
		*G = 255;
		*B = 57;
	} else if (color < 592) {
		*R = 57;
		*G = 255;
		*B = 58 + (color - 396);
	} else if (color < 789) {
		*R = 57;
		*G = 254 - (color - 592);
		*B = 255;
	} else {
		*R = 58 + (color - 789);
		*G = 57;
		*B = 255;
	}
	// *R *= brightness; //apply brightness modification
	// *G *= brightness;
	// *B *= brightness;
}

void all() { //display rainbow color progression, each LED out of phase with the others

	uint8_t R, G, B, j;
	int color;
	uint8_t ledNum = 0;
	uint8_t aTemp[4] = { 0 };

	while (!buttonPressed) {
		sendStart();
		for (color = 0; color < numColors; color++) { //progress from red to violet and back to red
			for (ledNum = 0; ledNum < 10; ledNum++) {
				getRGB((color + ledNum * 200) % numColors, &R, &G, &B); //offset LED colors by 200 from neighbors
				//loadLED(ledNum, R, G, B);

				aTemp[0] = 0xFF;
				aTemp[1] = R;
				aTemp[2] = G;
				aTemp[3] = B;

				for (j = 0; j < 4; j++) {
					USISRL |= aTemp[j]; // load shift register with data to send
					USICNT |= 8;              // shift out 8 bits
					millisecDelay(1);
				}

				//writeLeds(aTemp);
				millisecDelay(3000);
			}
			if (buttonPressed) //allow exit from function
				break;
			else if (color >= numColors) {
				color = 0;
			}
		}
	}
}
/*
void randomFade() {

	uint8_t i, j, x, z;
	uint8_t rand1[3] = { 0 };
	uint8_t rand2[3] = { 0 };
	uint8_t shift[3] = { 0 };
	uint8_t fadeColor[4] = { 0 };
	//uint8_t endColor[4];
	uint8_t steps = 255;

	// pick first set of random #s to be start color
	rand1[0] += rand() % 127;
	rand1[1] += rand() % 255;
	rand1[2] += rand() % 62;

	fadeColor[0] = 0xFF;
	for (z = 1; z < 4; z++) {         // iterate through each led(r,g,b) value
		fadeColor[z] = rand1[z - 1];  // load the fadeColor array this is the start color.
		}

	writeLeds(fadeColor);             // write the start color to the leds

	while (!buttonPressed) {

		// pick random #'s to fade to
		rand2[0] = rand() % 255;
		rand2[1] = rand() % 127;
		rand2[2] = rand() % 255;

		uint8_t endColor[4] = { 0xFF, rand2[0], rand2[1], rand2[2] };
		uint8_t div = 0;
		// figure the shift increment amount per step
		for (i = 0; i < 3; i++) {
			if (rand2[i] > rand1[i]) {
				div = rand2[i] / 2;
				shift[i] = ((rand2[i] - rand1[i]) / div); //increment amount
			} else {
				div = rand1[i] / 2;
				shift[i] = ((rand1[i] - rand2[i]) / div); //increment amount
			}
		}

		fadeColor[0] = 0xFF;
		for (j = 0; j < steps; j++) {         // increment through fade steps

			for (z = 1; z < 4; z++) {         // iterate through each array value
				fadeColor[z] = rand1[z - 1];  // rand1[] to fadeColor[] array
			}

			writeLeds(fadeColor);

			rand1[0] += shift[0];
			rand1[1] += shift[1];
			rand1[2] += shift[2];
			millisecDelay(3000);

			//uint8_t endColor[4] = {0xFF, rand2[0], rand2[1], rand2[2]};
			//endColor[0] = 0xFF;
			//endColor[1] = rand2[0];
			//endColor[2] = rand2[1];
			//endColor[3] = rand2[2];

			if (j > steps) {

				writeLeds(endColor);

				for (x = 0; x < 3; x++) {
					rand1[x] = rand2[x];  // shift rand2[] values into rand1[] values to set as new starting values
				}
			}
		}
	}
}
*/
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
	//buttonPressed = true;

	while (1) {

		if (buttonPressed) {
			buttonPressed = false;    //reset

#ifndef RGB
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
				eOther(0xFF);
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

#else   // RGB leds

			switch (pressCount) {
			case 0:
				writeLeds(white); //bright white
				LPM4;
				break;
			case 1:
				eOther(0xFF);
				LPM4;
				break;
			case 2:
				writeLeds(threeQuarter);
				LPM4;
				break;
			case 3:
				eOther(0xF8);
				LPM4;
				break;
			case 4:
				writeLeds(half);
				LPM4;
				break;
			case 5:
				eOther(0xF0);
				LPM4;
				break;
			case 6:
				writeLeds(oneQuarter);
				LPM4;
				break;
			case 7:
				eOther(0xE8);
				LPM4;
				break;
			case 8:
				writeLeds(purple);
				LPM4;
				break;
			case 9:
				writeLeds(orange);
				LPM4;
				break;
			case 10:
				writeLeds(red);
				LPM4;
				break;
			case 11:
				writeLeds(green);
				LPM4;
				break;
			case 12:
				writeLeds(blue);
				LPM4;
				break;
			case 13:
				all();
				LPM4;
				break;
			default:
				pressCount = 0; //default to 0
				buttonPressed = true; //force execution of case 0
				break;
			}
#endif

			writeFlash(pressCount);     // write to flash for poweroff
		}
	}
}
