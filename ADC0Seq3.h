// ADC0Seq3.h
// Runs on TM4C123.
// Enables anaoog to digital conversion.
// Stores the converted data into a FIFO.
// Kirabo Nsereko
// Dec 15, 2020

#ifndef ADC0SEQ3_H
#define ADC0SEQ3_H

#define ADC_FIFO_SIZE 20 // max size of fifo
#define ADC_MAX_VALUE 4095 // ADC0_SSFIFO3_R register is 12 bytes long. 2^24 - 1 = 4095

/**************Function Prototypes*********************/

// Configures the ADC0 to collect a sample upon a TimerA0 trigger.
void ADC0_InitTimer0ATriggerSeq3PE2(uint32_t, void (*task)(void)); 

// Initialize FIFO.
void ADC_FIFO_Init(void);

// Puts data into FIFO.
int8_t ADC_FIFO_Put(uint8_t data);

// Retrieves data from FIFO.
uint8_t ADC_FIFO_Get(void);

// Converts ADC output to pixel position (x-axis) on Nokia5110 LCD.
// stores result in global variable "g_playerPos"
void ADC_to_UChar(void);

// Converts ADC output to pixel position (x-axis) on Nokia5110 LCD.
static uint8_t ADCout_to_UChar(uint32_t);

#endif
