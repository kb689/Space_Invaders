// LEDs.c
// Includes functions that initialize and control two LEDs
// Green LED on means special missiles activated
// RED LED on means special missiles deactivated
// Kirabo Nsereko
// 9/3/2020


#include "..//tm4c123gh6pm.h"
#include "LEDs.h"


// Initialize PortB (PB5-4) for LEDs
// PB4: green LED
// PB5: red LED
// Input: none
// Output: none
void LEDs_Init(void){
	volatile unsigned long delay;
	SYSCTL_RCGC2_R |= 0x02;
	delay = SYSCTL_RCGC2_R;
	GPIO_PORTB_AMSEL_R &= ~0x30;
	GPIO_PORTB_PCTL_R &= ~0x30;
	GPIO_PORTB_DIR_R |= 0x30;
	GPIO_PORTB_AFSEL_R &= ~0x30;
	GPIO_PORTB_DEN_R |= 0x30;
}

void Green_LED_On(void){ // turn on green LED
	GPIO_PORTB_DATA_R |= 0x10;
}

void Green_LED_Off(void){ // turn off green LED
	GPIO_PORTB_DATA_R &= ~0x10;
}

void Red_LED_On(void){ // turn on green LED
	GPIO_PORTB_DATA_R |= 0x20;
}

void Red_LED_Off(void){ // turn off green LED
	GPIO_PORTB_DATA_R &= ~0x20;
}
