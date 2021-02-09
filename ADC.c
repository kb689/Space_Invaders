/* ADC.c
// This file converts analog input (PE2) from the slide pot into digital data.
// Kirabo Nsereko
// August 14, 2020
*/


#include "..//tm4c123gh6pm.h"
#include "ADC.h"


// This initialization function sets up the ADC 
// Max sample rate: <=125,000 samples/second
// SS3 triggering event: software trigger
// SS3 1st sample source:  channel 1
// SS3 interrupts: enabled but not promoted to controller
// ADC on PE2
void ADC0_Init(void){
	volatile unsigned long delay;
  SYSCTL_RCGC2_R |= 0x10; // activate clock for port E
	delay = SYSCTL_RCGC2_R; // allow time for clock to stabilize
	GPIO_PORTE_DIR_R &= ~0x04; // make PE2 input
	GPIO_PORTE_AFSEL_R |= 0x04; // enable alternate function
	GPIO_PORTE_DEN_R &= ~0x04; // disable digital I/O
	GPIO_PORTE_AMSEL_R |= 0x04; // enable analog mode
	SYSCTL_RCGC0_R |= 0x00010000; // activate ADC0
	delay = SYSCTL_RCGC0_R;
	SYSCTL_RCGC0_R &= ~0x00000300; // max sampling rate of 125k
	ADC0_SSPRI_R = 0x0123; // sequencer 3 highest priority
	ADC0_ACTSS_R &= ~0x0008; // disable sequencer 3
	ADC0_EMUX_R &= ~0xF000; // seq3 is software trigger
	ADC0_SSMUX3_R &= ~0x000F; // clear SS3 field
	ADC0_SSMUX3_R += 1; // set channel Ain1 (PE2)
	ADC0_SSCTL3_R = 0x0006; // no TS0 D0, yes IE0 END0
	ADC0_ACTSS_R |= 0x0008; // enable sequencer 3
}

//------------ADC0_In------------
// Busy-wait Analog to digital conversion
// Input: none
// Output: 12-bit result of ADC conversion
unsigned long ADC0_In(void){  
  unsigned long result;
	ADC0_PSSI_R = 0x08; //initiate conversion on sequencer 3
	while((ADC0_RIS_R&0x08)==0); // wait for conversion completion
	//result = ADC0_SSFIFO3_R&0xFFF; // retrieve converted data
	result = ADC0_SSFIFO3_R; // retrieve converted data
	ADC0_ISC_R = 0x0008; // clear ADC0_RIS_R flag
	return result;
}
