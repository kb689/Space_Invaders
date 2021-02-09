// ADCT0ATrigger.c
// Runs on TM4C123
// Provide a function that initializes Timer0A to trigger ADC0
// Sequence 3 and requests an interrupt when conversion is complete.
// Converted result stored in a FIFO queue **if necessary**
// ****if using FIFO queue remove function pointer input to function "ADC0_InitTimer0ATriggerSeq3PE2",
// and update header file******.
// Daniel Valvano
// Jan 12, 2020
/* Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu */
//
// Kirabo Nsereko
// Dec 15, 2020



#include <stdint.h>
#include "ADC0Seq3.h"
#include "..//tm4c123gh6pm.h"
#include "GameConstants.h"
#include "Images.h"
#include "LEDs.h"

//*************************include only if using FIFO queue**********************//
volatile uint8_t ADC_FIFO[ADC_FIFO_SIZE]; // initialize FIFO array
static uint32_t PutI; //next index location to put data into ADC_FIFO 
static uint32_t GetI; // index of oldest data in ADC_FIFO
static uint32_t CurrSize; // current size of ADC_FIFO
static uint32_t LostData; // number of times data is lost due to full FIFO
//*******************************************************************************//

extern uint32_t g_playerPos;// player position (in pixels) on LCD. Declared in "SpaceInvaders_v2.c"
static void (*Update_Player_Pos)(void); // user function that updates the player x-axis position.


// Configures the ADC0 to collect a sample upon a TimerA0 trigger.
// Parameters:
// Timer0A: enabled
// Mode: 32-bit, down counting
// One-shot or periodic: periodic
// Interval value: programmable using 32-bit period
// Sample time is busPeriod*period
// Max sample rate: <=125,000 samples/second
// SS3 triggering event: Timer0A
// SS3 sample source: channel 1
// SS3 interrupts: enabled and promoted to controller
// ADC on PE2
// Input: interrupt rate (Hz), function pointer that updates player position
// Output: none
void ADC0_InitTimer0ATriggerSeq3PE2(uint32_t rate, void (*task)(void)){
	uint32_t period = SYSTEM_CLOCK/rate;
  volatile uint32_t delay;
	Update_Player_Pos = task;
  SYSCTL_RCGCADC_R |= 0x01;     // 1) activate ADC0 
	//delay = SYSCTL_RCGCADC_R;        // wait for clock to stabilize
  SYSCTL_RCGCGPIO_R |= 0x10;    // Port E clock
  delay = SYSCTL_RCGCGPIO_R;    // allow time for clock to stabilize
  GPIO_PORTE_DIR_R &= ~0x04; // make PE2 input
	GPIO_PORTE_AFSEL_R |= 0x04; // enable alternate function
	GPIO_PORTE_DEN_R &= ~0x04; // disable digital I/O
	GPIO_PORTE_AMSEL_R |= 0x04; // enable analog mode
  ADC0_PC_R = 0x01;             // 2) configure for 125K samples/sec
  ADC0_SSPRI_R = 0x3210;        // 3) sequencer 0 is highest, sequencer 3 is lowest. Pri irrelev since only using Seq3
  SYSCTL_RCGCTIMER_R |= 0x01;   // 4) activate timer0 
  delay = SYSCTL_RCGCGPIO_R;
	delay = SYSCTL_RCGCTIMER_R;   // wait for clock to stabilize
  TIMER0_CTL_R = 0x00000000;    // disable timer0A during setup
  TIMER0_CTL_R |= 0x00000020;   // enable timer0A trigger to ADC
  TIMER0_CFG_R = 0;             // configure for 32-bit timer mode
  TIMER0_TAMR_R = 0x00000002;   // configure for periodic mode, default down-count settings
  TIMER0_TAPR_R = 0;            // prescale value for trigger
  TIMER0_TAILR_R = period-1;    // start value for trigger
  TIMER0_IMR_R = 0x00000000;    // disable all interrupts
  TIMER0_CTL_R |= 0x00000001;   // enable timer0A 32-b, periodic, no interrupts
  ADC0_ACTSS_R &= ~0x08;        // 5) disable sample sequencer 3
  ADC0_EMUX_R = (ADC0_EMUX_R&0xFFFF0FFF)+0x5000; // 6) timer trigger event
  ADC0_SSMUX3_R = 1;            // 7) set channel Ain1 (PE2)
  ADC0_SSCTL3_R = 0x06;         // 8) no TS0 D0, yes IE0 END0                       
  ADC0_IM_R |= 0x08;            // 9) enable SS3 interrupts
  ADC0_ACTSS_R |= 0x08;         // 10) enable sample sequencer 3
  NVIC_PRI4_R = (NVIC_PRI4_R&0xFFFF0FFF)|0x00001000; // 11)priority 1
  NVIC_EN0_R = (NVIC_EN0_R)|(1<<17);           // 12) enable interrupt 17 in NVIC
}

// ADC0Seq3 ISR.
// Puts converted data into ADC_FIFO.
// Inputs: none
// Outputs: none
void ADC0Seq3_Handler(void){
  ADC0_ISC_R = 0x08;         // acknowledge ADC sequence 3 completion
	(*Update_Player_Pos)();
}

// Converts ADC output to pixel position (x-axis) on Nokia5110 LCD.
// Inputs: none
// Outputs: none
void ADC_to_UChar(void){
  uint32_t temp; // holds temp value of PlayerPos before its converted to unsigned char
	// PlayerShip x position can move from 0 to (LCD_LENGTH-PLAYERW) pixels on LCD. (0-66)
	temp = ((LCD_WIDTH - PLAYERW)*ADC0_SSFIFO3_R)/ADC_MAX_VALUE;
	g_playerPos = (uint8_t)temp;
}

//// ADC0Seq3 ISR.
//// Puts converted data into ADC_FIFO.
//// Inputs: none
//// Outputs: none
//void ADC0Seq3_Handler(void){
//	uint8_t playerPos; // player x position
//  ADC0_ISC_R = 0x08;         // acknowledge ADC sequence 3 completion
//	playerPos = ADCout_to_UChar(ADC0_SSFIFO3_R); // convert ADC data to pixel position
//	ADC_FIFO_Put(playerPos); // pass data to foreground
//}

// Converts ADC output to pixel position (x-axis) on Nokia5110 LCD.
// Inputs: ADC output data from register: ADC0_SSFIFO3_R
// Outputs: pixel position on Nokia5110 x-axis
static uint8_t ADCout_to_UChar(uint32_t ADCdata){
	uint8_t playerPos; // player ship x position
  uint32_t temp; // holds temp value of PlayerPos before its converted to unsigned char
	// PlayerShip x position can move from 0 to (LCD_LENGTH-PLAYERW) pixels on LCD. (0-66)
	temp = ((LCD_WIDTH - PLAYERW)*ADCdata)/ADC_MAX_VALUE;
	playerPos = (uint8_t)temp;
	return playerPos;
}	

// Initialize FIFO.
// Input: none
// Output: none
void ADC_FIFO_Init(void){
  PutI = GetI = 0; // first index of fifo
	CurrSize = 0; 
	LostData = 0;
}	


// Puts data into FIFO.
// Inputs: converted data from ADC
// Outputs: returns (-1) if FIFO full,
//          or (0) if successful 
int8_t ADC_FIFO_Put(uint8_t data){
  if (CurrSize == ADC_FIFO_SIZE){ // FIFO full
	  LostData++;
		return -1;
	}
	
	ADC_FIFO[PutI] = data;
	PutI = (PutI+1)%ADC_FIFO_SIZE;
	CurrSize++;
	return 0;
}

// Retrieves data fro FIFO.
// Inputs: none
// Outputs: returns (255) if FIFO empty,
//          or ADC data from ADC_FIFO
uint8_t ADC_FIFO_Get(void){
  uint8_t data;
	if (CurrSize){ // FIFO not empty
	  data = ADC_FIFO[GetI];
		GetI = (GetI+1)%ADC_FIFO_SIZE;
		CurrSize--;
		return data;
	}
	
	return 255; // fifo empty
}
