/* DAC.c
// Implementation of the 4-bit digital to analog converter
// Kirabo Nsereko
// Port B bits 3-0 have the 4-bit DAC
*/


#include "DAC.h"
#include "..//tm4c123gh6pm.h"

#define DACOUT (*((volatile unsigned long *)0x4000503C)) // PB3-0

unsigned char Sounds[60]; // only for testing
unsigned long idx = 0; // only for testing


// **************DAC_Init*********************
// Initialize 4-bit DAC 
// Input: none
// Output: none
void DAC_Init(void){ // initialize port B for digital output (PB3-0)
	volatile unsigned long delay;
	SYSCTL_RCGC2_R |= 0x02;
	delay = SYSCTL_RCGC2_R;
	GPIO_PORTB_AMSEL_R &= ~0x0F;
	GPIO_PORTB_PCTL_R &= ~0x0F;
	GPIO_PORTB_DIR_R |= 0x0F;
	GPIO_PORTB_AFSEL_R &= ~0x0F;
	GPIO_PORTB_DEN_R |= 0x0F;
	GPIO_PORTB_DR8R_R |= 0x0F;
}


// **************DAC_Out*********************
// output to DAC
// Input: 4-bit data, 0 to 15 
// Output: none
void DAC_Out(unsigned long data){ // output to port B
  if(data>15) {
		return;
	}
	
	//GPIO_PORTB_DATA_R |= data;
	DACOUT = data;
	
	// the following piece of code is for testing purposes
	if(idx<60){
	Sounds[idx] = GPIO_PORTB_DATA_R;
	idx++;
	}
}
