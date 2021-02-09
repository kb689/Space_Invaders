/* SpaceInvaders.c
// Runs on TM4C123 and Nokia5110 display.
// Project for edx course: "Embedded Systems - Shape The World: Multi-Threaded Interfacing", lab 15
// Kirabo Nsereko
// 12/20/2020
*/

/* This example accompanies the books
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2013
   "Embedded Systems: Introduction to Arm Cortex M Microcontrollers",
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2013
 Copyright 2014 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */
 
// ******* Required Hardware I/O connections*******************

// PA1, PA0 UART0 connected to PC through USB cable
// Slide pot pin 1 connected to ground
// Slide pot pin 2 connected to PE2/AIN1
// Slide pot pin 3 connected to +3.3V 
// fire button connected to PE0
// 8*R resistor DAC bit 0 on PB0 (least significant bit)
// 4*R resistor DAC bit 1 on PB1
// 2*R resistor DAC bit 2 on PB2
// 1*R resistor DAC bit 3 on PB3 (most significant bit)
// LED on PB4
// LED on PB5

#include <stdio.h>
#include <stdint.h>
#include "SpaceInvaders_v2.h"
#include "..//tm4c123gh6pm.h"
#include "Nokia5110.h"
#include "Random.h"
#include "Sounds.h"
#include "Images.h"
#include "DAC.h"
#include "ADC.h"
#include "LEDs.h"
#include "ADC0Seq3.h"
#include "PLL.h"


//*************************Global Variables*****************************//

volatile STyp PlayerMissiles[MAX_PLAYER_MISSILES]; // creates an array of all the possible player missiles in the game
STyp EnemyMissiles[MAX_ENEMY_MISSILES]; // creates an array of all the possible enemy missiles in the game. 3 chosen to prevent 
STyp Enemy[MAX_ENEMIES]; // creates an array of all the possible enemies in the game

static volatile uint32_t g_playerMissiles = 0; // number of player missiles on screen
static volatile int32_t g_freePlayerMissileIndex = 0; // array index of free player missile
volatile uint8_t g_playerPos; // player position (in pixels) on LCD
static volatile uint32_t g_bnkrDamageLvl=0; // total bunker damage
static volatile size_t g_soundIndex; // Index into Sound array.
static volatile uint32_t g_soundCount=0; // Number of points left in sound array.
static volatile const uint8_t *g_soundWave; // Pointer to sound array.

static void (*Fire_Missile)(void); // Pointer to a user function "Fire_One_Missile".
static void (*Sound)(void); // Pointer to user function "Play_Sound".


//***************************Semaphores******************************// 

struct Semaphores{
	volatile uint8_t GamePlay; // semaphore to allow gameplay(user tasks and displaying graphics)
}Semaphores;

//****************************Main Program***********************************//
int main(void){
//******************************Declare and Initialize Variables**********************************//	 
	 // local variables
	 int32_t freeEnemyMissileIndex = 0; // array index of free enemy missile
   uint32_t enemyMissiles = 0; // munber of enemy missiles on screen
	 uint32_t frameCount=0; // used to access 2 enemy image frames. frames played in sequence give appearance of enemy moving
	 uint32_t score=0; // player total score
	 // used to decrease rate of enemy missile fire wrt to rate objects are drawn on LCD.
	 uint8_t enemyFire=0; 
   uint32_t playerLives = MAX_PLAYER_LIVES; // number of player lives left	 
	 uint32_t level=1; // urrent level the game is on
	 
	 Semaphores.GamePlay = 0; // initialize Semaphore
	 
//**********************************Initializations*****************************************//
	 DisableInterrupts();
	 //TExaS_Init(NoLCD_NoScope);  // set system clock to 80 MHz
	 PLL_Init(); // set system clock to 50MHz
   // you cannot use both the Scope and the virtual Nokia (both need UART0)
	 //TExaS_Init(SSI0_Real_Nokia5110_Scope);  // set system clock to 80 MHz
	 Nokia5110_Init();
	 Switch_Init(Fire_One_Missile);
	 Timer3_Init(10);
	 Timer2_Init(11500, Sound_Values_to_DAC);
	 SysTick_Init(30);
	 //ADC_FIFO_Init();
	 LEDs_Init(); // initialize the LEDs
	 ADC0_Init();
	 //ADC0_InitTimer0ATriggerSeq3PE2(20, ADC_to_UChar); // initialize ADC
	 Init_Enemy(ENEMY_LIVES_LVL1);
	 DAC_Init(); // initialize the DAC
	 LEDs_Init(); // initialize the LEDs
	 
//***************************Game start sequence**************************//

	// display start sequence to LCD
	Nokia5110_Clear();
	Nokia5110_SetCursor(1, 1);
  Nokia5110_OutString("PRESS");
	Nokia5110_SetCursor(1, 2);
  Nokia5110_OutString("BUTTON TO");
	Nokia5110_SetCursor(1, 3);
  Nokia5110_OutString("START");
	
	// wait for switch to be pressed
	while (!(GPIO_PORTE_DATA_R&0x01)) {};
		
	Nokia5110_Clear();
  Nokia5110_SetCursor(1, 1);
  Nokia5110_OutString("DEFEAT");
  Nokia5110_SetCursor(1, 2);
  Nokia5110_OutString("THE");
  Nokia5110_SetCursor(1, 3);
  Nokia5110_OutString("INVADERS!!");
	Delay1ms(1000);
	Nokia5110_Clear();
	Nokia5110_OutString("GOOD LUCK!!");
	Delay1ms(1000);
	Nokia5110_Clear();
	Nokia5110_OutString("GO!!!");
	Delay1ms(1000);
	Nokia5110_SetCursor(0, 0); // renders screen
	Nokia5110_Clear();	

  // Set initial game play
	Green_LED_On();
  Draw_Enemy(&frameCount);
	Draw_Bunker();
	Draw_Player();
	Nokia5110_DisplayBuffer();
	// acknowledge flag0 to prevent missile (if switch pressed) fire before game starts
	GPIO_PORTE_ICR_R |= 0x01; 
	EnableInterrupts();
	Init_Unique_Random_Num();
	
   while(1){
		 
		 // Execcute game play functions at a rate of 30Hz
		 if (Semaphores.GamePlay){
		   Nokia5110_ClearBuffer();
			 Move_Player();
			 Enemy_Hit(&score);
			 Bunker_Hit(&enemyMissiles, &freeEnemyMissileIndex);
			 Enemy_Missile_Hit_PlayerShip(&enemyMissiles, &playerLives, &freeEnemyMissileIndex);
			 Player_Missile_Hit_Enemy_Missile(&enemyMissiles, &freeEnemyMissileIndex);
			 Enemy_Fire(&enemyMissiles, &freeEnemyMissileIndex, &enemyFire);
			 Revive_Enemy_If_Lvl_Not_Reached(&level, &score);
			 Move_Player_Missiles();
			 Move_Enemy_Missiles(&enemyMissiles, &freeEnemyMissileIndex);
			 Move_Enemy();
			 
			 Move_Player();
			 Draw_Player_Missiles();
			 Draw_Enemy_Missiles();
			 Draw_Enemy(&frameCount);
			 Draw_Bunker();
			 Draw_Player();
			 Draw_Player_Life(&playerLives);
			 Inrease_Level(&score, &level, &enemyMissiles);
			 Nokia5110_DisplayBuffer();
			 Game_Won(&score);
			 Game_Lost(&playerLives, &score);
			 Semaphores.GamePlay = 0; // clear semaphore
		 }
	 }
}

// Initialize the random num generator with a unique seed.
// Input: none
// Output: none
void Init_Unique_Random_Num(void){
  Random_Init(NVIC_ST_CURRENT_R);
}
		 
// Determines if game has been lost.
// Prints message to screen if true.
// Inputs: pointer to total score
// Outputs: none
void Game_Lost(uint32_t *playerLivesPtr, uint32_t *scorePtr){
  if (!(*playerLivesPtr)){
		DisableInterrupts();
		Green_LED_Off();
		Red_LED_On();
		Nokia5110_PrintBMP(g_playerPos, LCD_HEIGHT-1, BigExplosion0, 0);
		Nokia5110_DisplayBuffer();
		Delay1ms(500);
		Nokia5110_PrintBMP(g_playerPos, LCD_HEIGHT-1, BigExplosion1, 0);
		Nokia5110_DisplayBuffer();
		Delay1ms(1500);
	  Nokia5110_ClearBuffer();
	  Nokia5110_Clear();
		Nokia5110_SetCursor(1, 1);
		Nokia5110_OutString("YOU LOST!");
		Nokia5110_SetCursor(1, 2);
		Nokia5110_OutString("Score:");	
		Nokia5110_SetCursor(1, 3);
		Nokia5110_OutUDec((*scorePtr));
		Delay1ms(2000);
		Nokia5110_Clear();
		Nokia5110_SetCursor(1, 1);
		Nokia5110_OutString("PRESS RESET");
		Nokia5110_SetCursor(0, 0); // renders screen
		while(1){};
	}
}

// Determines if game has been won.
// Prints message to screen if true.
// Inputs: none
// Outputs: none
void Game_Won(uint32_t *scorePtr){
  if ((*scorePtr) >= WINNING_SCORE){
		DisableInterrupts();
		Delay1ms(1500);
	  Nokia5110_ClearBuffer();
		Nokia5110_Clear();
		Nokia5110_SetCursor(1, 1);
		Nokia5110_OutString("YOU WON!!");
		Nokia5110_SetCursor(1, 2);
		Nokia5110_OutString("Score:");	
		Nokia5110_SetCursor(1, 3);
		Nokia5110_OutUDec((*scorePtr));
		Delay1ms(1500);
		Nokia5110_Clear();
		Nokia5110_SetCursor(1, 1);
		Nokia5110_OutString("PRESS RESET");
		Nokia5110_SetCursor(0, 0); // renders screen
		while(1){};
	}
}

// Initialize timer2A.
// Timer used to play sounds after an event occurs. 
// Input: interrupt rate (11.5kHz), pointer to a void-void function to be executed by the ISR
// Output: none
void Timer2_Init(uint32_t rate, void (*task)(void)){ 
  uint32_t volatile delay;
	uint32_t period = SYSTEM_CLOCK/rate;
	Sound = task;
  SYSCTL_RCGCTIMER_R |= 0x04;   // 0) activate timer2
  delay = SYSCTL_RCGCTIMER_R;
  TIMER2_CTL_R = 0x00000000;    // 1) disable timer2A during setup
  TIMER2_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER2_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER2_TAILR_R = period-1;    // 4) reload value
  TIMER2_TAPR_R = 0;            // 5) bus clock resolution
  TIMER2_ICR_R = 0x00000001;    // 6) clear timer2A timeout flag
  TIMER2_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI5_R = (NVIC_PRI5_R&0x00FFFFFF)|0xE0000000; // 8) priority 7
// interrupts enabled in the main program after all devices initialized
// vector number 39, interrupt number 23
  NVIC_EN0_R = 1<<23;           // 9) enable IRQ 23 in NVIC
  //TIMER2_CTL_R = 0x00000001;    // 10) enable timer2A
}

// Sends values in "SoundWave" array to the DAC ("DAC_Out").
// Inputs: none
// Outputs: none
static void Sound_Values_to_DAC(void){
  if (g_soundCount){
		DAC_Out(g_soundWave[g_soundIndex]);
		g_soundIndex++;
		g_soundCount--;
	} else {
			Timer2A_Stop();
	}
}

// ISR for Timer 2A.
// Inputs: none
// Outputs: none
void Timer2A_Handler(void){
  TIMER2_ICR_R = 0x00000001;   // acknowledge timer2A timeout
	(*Sound)(); // execute user function 
}

// Determines what sound will be played based on an event (eg player/enemy hit)
// Input: pointer to a sound array, length of the sound array
// Output: none
static void Play_Sound(const uint8_t *pt, uint32_t count){
  g_soundWave = pt;
  g_soundIndex = 0;
  g_soundCount = count;
  Timer2A_Start();
}

// Enables TImer2A.
// Input: void
// Output: void
static void Timer2A_Start(void){
	TIMER2_CTL_R |= 0x01; // enable
}

// Disables Timer2A.
// Input: void
// Output: void
static void Timer2A_Stop(void){
	TIMER2_CTL_R &= ~0x01; // disable
}

// Delay by a multiple of 1ms.
// Input: delay time(ms)
// Output: void
void Delay1ms(uint32_t count){
	uint32_t volatile time;
  while(count>0){
    
		// 1ms at 80MHz. calculation: delay/clk period/#cycles to execute function. where cycles=6
		// time = 1ms/(1/80M)/6
		time = 13333; 
		
    while(time){
	  	time--;
    }
    count--;
  }
}

// Initializes enemies
// Input: number of lives each enemy starts with
// Output: void
void Init_Enemy(uint32_t lives){ 
	int32_t i; int32_t j = 0;
	for (i=0; i<MAX_ENEMIES; i++){ // max enemies has to be 5 in order for correct initialization
		Enemy[i].x = j;
		Enemy[i].y = ENEMY10H + PLAYER_LIFE_4H;
		Enemy[i].life = lives;
		if (i == 0){
			Enemy[i].image[0] = SmallEnemy10PointA;
			Enemy[i].image[1] = SmallEnemy10PointB;
		} else if(i == 1 || i == 2){
				Enemy[i].image[0] = SmallEnemy20PointA;
				Enemy[i].image[1] = SmallEnemy20PointB;			
		} else if (i == 3 || i == 4){
				Enemy[i].image[0] = SmallEnemy30PointA;
				Enemy[i].image[1] = SmallEnemy30PointB;
		}
		j += ENEMY10W; // all enemies are same width (16)
	}
}

// Put enemy sprite location data in the buffer before printing.
// Input: frameCount pointer
// Output: void
void Draw_Enemy(uint32_t *frameCountPtr){ int32_t i;
  for(i=0;i<MAX_ENEMIES;i++){
    if(Enemy[i].life > 0){
     Nokia5110_PrintBMP(Enemy[i].x, Enemy[i].y, Enemy[i].image[(*frameCountPtr)], 0);
    }
  }
  (*frameCountPtr) = ((*frameCountPtr)+1)&0x01; // 0,1,0,1,...
}

// Move enemy sprites.
// kills enemy if it moves ouside of LCD screen, revives them if enemy re-enters screen while
// not transitioning to a next level.
// adds up the total num of dead enemies
// Input: void
// Output: void
void Move_Enemy(void){ 
	int32_t i;
	for (i=0; i<MAX_ENEMIES; i++){
		Enemy[i].x = (Enemy[i].x + 1)%LCD_WIDTH;
	}
}

// Revives a dead enemy at position x=0.
// Inputs: none
// Outputs: none
static void Revive_Enemy_At_XPos0(uint32_t *lvlPtr){
  uint32_t i;
	for (i=0; i<MAX_ENEMY_MISSILES; i++){
	  if (Enemy[i].x == 0 && Enemy[i].life == 0){
			if ((*lvlPtr) == 1){
			  Enemy[i].life = ENEMY_LIVES_LVL1;
				return;
			}
		  if ((*lvlPtr) == 2){
			  Enemy[i].life = ENEMY_LIVES_LVL2;
				return;
			}
			if ((*lvlPtr) == 3){
			  Enemy[i].life = ENEMY_LIVES_LVL3;
				return;
			}
			if ((*lvlPtr) == 4){
			  Enemy[i].life = ENEMY_LIVES_LVL4;
				return;
			}
		}
	}
}

// Revive dead enemy if the next level score not reached.
// Inputs: pointers to current level, score
// Outputs: none
void Revive_Enemy_If_Lvl_Not_Reached(uint32_t *lvlPtr, uint32_t *scorePtr){
  if ((*lvlPtr)== 1 && (*scorePtr) < LEVEL2_SCORE){
	  Revive_Enemy_At_XPos0(lvlPtr);
		return;
	}
	
	if ((*lvlPtr)== 2 && (*scorePtr) < LEVEL3_SCORE){
	  Revive_Enemy_At_XPos0(lvlPtr);
		return;
	}
	
	if ((*lvlPtr)== 3 && (*scorePtr) < LEVEL4_SCORE){
	  Revive_Enemy_At_XPos0(lvlPtr);
		return;
	}
	
	if ((*lvlPtr)== 4 && (*scorePtr) < WINNING_SCORE){
	  Revive_Enemy_At_XPos0(lvlPtr);
		return;
	}
}

// Initializes pin PE0 as digital input.
// Pin PE0 is a switch input.
// PE0 fires one missile.
// Initialize hardware interrupt on rising edge of PE0.
// Input: pointer to a void-void function to be executed by the ISR
// Output void
void Switch_Init(void (*task)(void)){
	volatile uint32_t delay;
	SYSCTL_RCGC2_R |= 0x10; // Activate PortE clock
	Fire_Missile = task;
	delay = SYSCTL_RCGC2_R; // wait for clk to initialize
	GPIO_PORTE_IM_R &= ~0x01;       // disarm interrupt on PE0
	GPIO_PORTE_AMSEL_R &= ~0x01;    // disable analog
	GPIO_PORTE_PCTL_R &= ~0x01;     // disable analog
	GPIO_PORTE_DIR_R &= ~0x01;      // PE0 input
	GPIO_PORTE_AFSEL_R &= ~0x01;    // disable alternate functions
	GPIO_PORTE_DEN_R |= 0x01;       // PE0 digital enable 
	GPIO_PORTE_IS_R &= ~0x01;       // PE0 is edge-sensitive 
  GPIO_PORTE_IBE_R &= ~0x01;      // PE0  is not both edges 
  GPIO_PORTE_IEV_R |= 0x01;       // PE0 rising edge event 
  GPIO_PORTE_ICR_R |= 0x01;       // clear flags
  GPIO_PORTE_IM_R |= 0x01;        // arm interrupt on PE0
  NVIC_PRI1_R = (NVIC_PRI1_R&0xFFFFFF0F)|0x00000020; // priority 1
  NVIC_EN0_R |= 0x00000010;        // enable interrupt 4 in NVIC
	
}

// ISR for switch on PE0.
// Fire/create missile.
// debounce switch if pressed.
// Input: void
// Output: void
void GPIOPortE_Handler(void){
  if (GPIO_PORTE_RIS_R&0x01){ // poll PE0 
	  GPIO_PORTE_ICR_R |= 0x01; // acknowledge flag0
		//****** intialize debounce switch on PE0 *****//
    GPIO_PORTE_IM_R &= ~0x01;  // diasble interrupt on PE0
		TIMER3_IMR_R = 0x00000001;    // arm timeout interrupt
    TIMER3_CTL_R = 0x00000001; //enable TIMER3A		
		//*********************************************//
		
		(*Fire_Missile)(); // execute user function
	}
}

// ***************** Timer3_Init ****************
// Implements a delay to debounce switch (PE0).
// Initialize software interrupt.
// Inputs:  delay in ms
// Outputs: none
void Timer3_Init(uint32_t delay){
	uint32_t period = (SYSTEM_CLOCK*delay)/1000;
  SYSCTL_RCGCTIMER_R |= 0x08;   // 0) activate TIMER3
  TIMER3_CTL_R = 0x00000000;    // 1) disable TIMER3A during setup
  TIMER3_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER3_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER3_TAILR_R = period-1;    // 4) reload value
  TIMER3_TAPR_R = 0;            // 5) bus clock resolution
  TIMER3_ICR_R = 0x00000001;    // 6) clear TIMER3A timeout flag
  // 7) arm timeout interrupt
  NVIC_PRI8_R = (NVIC_PRI8_R&0x00FFFFFF)|0xA0000000; // 8) priority 5
// vector number 51, interrupt number 35
  NVIC_EN1_R = 1<<(35-32);      // 9) enable IRQ 35 in NVIC
	// 10) enable TIMER3A
}


// Debounces switch on PE0
// disables Timer3A
// Inputs: none
// Outputs: none
void Timer3A_Handler(void){
  TIMER3_ICR_R = TIMER_ICR_TATOCINT;  // acknowledge TIMER3A timeout
	TIMER3_IMR_R &= ~0x00000001;    // disarm timeout interrupt
	TIMER3_CTL_R = 0x00000000;          // disable TIMER3A
	GPIO_PORTE_ICR_R |= 0x01;           // clear flag0 (PE0)
	GPIO_PORTE_IM_R |= 0x01;            // arm interrupt on PE0
}

// Fires one player missile.
// Inputs: none
// Outputs: none
void Fire_One_Missile(void){
	if (g_playerMissiles == MAX_PLAYER_MISSILES){ // missile limit reached
	  return;
	}
	
	// player is underneath bunker and it isnt destroyed. firing missile disabled
	// magic numbers 9 and 11 chosen to prevent fired missile intersecting with bunker image
	if (g_playerPos >= (BUNKER_XPOS - 9) && g_playerPos <= (BUNKER_XPOS + 11) && 
	  g_bnkrDamageLvl < BUNKER_DESTROYED){ 
		 return;
	}
	
	//initialize missile
	PlayerMissiles[g_freePlayerMissileIndex].x = g_playerPos + (PLAYERW/2 - 2);
	PlayerMissiles[g_freePlayerMissileIndex].y = (LCD_HEIGHT-1) - PLAYERH-1; // one pixel above the player ship
  PlayerMissiles[g_freePlayerMissileIndex].image[0] = Missile1;
	PlayerMissiles[g_freePlayerMissileIndex].life = 1;
	g_playerMissiles++; // increment player missiles
	Update_Free_Index(&g_freePlayerMissileIndex, &g_playerMissiles, 'p');
  Play_Sound(Shoot, SHOOT_ARRAY_SIZE);
}

// Finds free index in an array.
// Inputs:  pointers to number of enemy/player missiles and free index in EnemyMissiles/PlayerMissiles array,
//          type(enemy:'e' or player: 'p')
// Outputs: none
static void Update_Free_Index(volatile int32_t *freeIndexPtr, volatile uint32_t *numMissilesPtr, char type){
	uint32_t i;
	// find index of free missile in PlayerMissiles array
  if (type == 'p'){ 
	  if ((*numMissilesPtr) == MAX_PLAYER_MISSILES){ // missile array full
		  (*freeIndexPtr) = -1;
			return;
		}
		
		for (i=0; i<MAX_PLAYER_MISSILES; i++){
		  if (PlayerMissiles[i].life == 0){
			  (*freeIndexPtr) = i;
				return;
			}
		}
	}
	
	// find index of free missile in EnemyMissiles array
	if (type == 'e'){ 
	  if ((*numMissilesPtr) == MAX_ENEMY_MISSILES){ // missile array full
		  (*freeIndexPtr) = -1;
			return;
		}
		
		for (i=0; i<MAX_ENEMY_MISSILES; i++){
		  if (EnemyMissiles[i].life == 0){
			  (*freeIndexPtr) = i;
				return;
			}
		}
	}
	
	return;
}

// Move player missiles.
// Input: void
// Output: void
void Move_Player_Missiles(void){ 
	uint32_t i;
	for (i = 0; i < MAX_PLAYER_MISSILES; i++){
		if (PlayerMissiles[i].life == 1){
			// destroy missile that reached top of screen
			if ((PlayerMissiles[i].y - MISSILEH) <= 0){
				PlayerMissiles[i].life = 0;
				g_playerMissiles--;
				// missile array was previously full
				if (g_freePlayerMissileIndex == -1){
				  Update_Free_Index(&g_freePlayerMissileIndex, &g_playerMissiles, 'p');
				}
				continue;
			}
			PlayerMissiles[i].y -= 1; // move missile one pixel up
		}
	}			
}

// Put player missile location data in the buffer before printing.
// Input: void
// Output: void
void Draw_Player_Missiles(void){ 
	int32_t i;
	for (i=0; i<MAX_PLAYER_MISSILES; i++){
		if (PlayerMissiles[i].life == 1){
			Nokia5110_PrintBMP(PlayerMissiles[i].x, PlayerMissiles[i].y, PlayerMissiles[i].image[0], 0);
		}
	}
}

// Initialize SysTick timer.
// Timer used to draw objects on the LCD at a rate of 30Hz
// Input: Interrupt rate in Hz
// Output: void
void SysTick_Init(uint32_t rate){
	uint32_t period = SYSTEM_CLOCK/rate;
	NVIC_ST_CTRL_R = 0;
	NVIC_ST_RELOAD_R = period-1;
	NVIC_ST_CURRENT_R = 0;
	NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x40000000; // priority 2
	NVIC_ST_CTRL_R = 0x07;
}

// ISR for SysTick timer
// determines player position from ADC input
// determines if a player missile is fired
// Input: void
// Output: void
void SysTick_Handler(void){
  Semaphores.GamePlay = 1; // sets semaphore to allow gameplay
}

// Updates player position by calling "ADC0_In".
// Inputs: none
// Outputs: none
void Move_Player(void){
  unsigned long ADCdata;
	ADCdata = ADC0_In();
	// PlayerShip x position can move from 0 to (LCD_LENGTH-PLAYERW) pixels on LCD. 0-66
	g_playerPos = (uint8_t)(((LCD_WIDTH - PLAYERW)*ADCdata)/ADC_MAX_VALUE);
}

//// Moves player by fetching position from ADC_FIFO.
//// Inputs: none
//// Outputs: none
//void Move_Player(void){
//  uint8_t temp;
//	temp = ADC_FIFO_Get();
//	if (temp == 255){ // FIFO empty
//	  return;
//	}
//	
//	g_playerPos = temp;
//}

// Put player ship location data in the buffer before printing.
// Inputs: none
// Outputs: none
void Draw_Player(void){
  Nokia5110_PrintBMP(g_playerPos, LCD_HEIGHT-1, PlayerShip0, 0);
}

// Put bunker image in buffer to be printed.
// Input: void
// Output: void
void Draw_Bunker(void) {
	if (g_bnkrDamageLvl <  LIGHT_BKR_DAMAGE){
	  Nokia5110_PrintBMP(BUNKER_XPOS, LCD_HEIGHT - 1 - PLAYERH, Bunker0, 0);
		return;
	}
	
	if (g_bnkrDamageLvl >= LIGHT_BKR_DAMAGE && g_bnkrDamageLvl < HEAVY_BKR_DAMAGE){
	  Nokia5110_PrintBMP(BUNKER_XPOS, LCD_HEIGHT - 1 - PLAYERH, Bunker1, 0);
		return;
	}
	
	if (g_bnkrDamageLvl >= HEAVY_BKR_DAMAGE && g_bnkrDamageLvl < BUNKER_DESTROYED){
	  Nokia5110_PrintBMP(BUNKER_XPOS, LCD_HEIGHT - 1 - PLAYERH, Bunker2, 0);
		return;
	}
	
	if (g_bnkrDamageLvl >=  BUNKER_DESTROYED){
		Nokia5110_PrintBMP(BUNKER_XPOS, LCD_HEIGHT - 1 - PLAYERH, 0, 0);
		return;
	}
	
}

// Determines if an enemy is hit by a player missile. Prints an explosion if true.
// Input: Total score
// Output: void
void Enemy_Hit(uint32_t *scorePtr){ 
	int32_t i, j;
	for (i = 0; i<MAX_PLAYER_MISSILES; i++){
		// live player missile in hit range
		if (PlayerMissiles[i].life == 1 && (PlayerMissiles[i].y - MISSILEH) <= ENEMY10H+1){
			for (j=0; j<MAX_ENEMIES; j++){
				// Determines if player missile hits live enemy and enemy is on the screen 
				if (Enemy[j].life > 0 && PlayerMissiles[i].x >= Enemy[j].x && 
					   PlayerMissiles[i].x < (Enemy[j].x + ENEMY10W) && 
				      Enemy[j].x <= (LCD_WIDTH-ENEMY10W)){
					PlayerMissiles[i].life = 0;
					Enemy[j].life --;
					g_playerMissiles -= 1;
					if (Enemy[j].life == 0){
					  (*scorePtr) += 5;
					}
					Nokia5110_PrintBMP(Enemy[j].x, Enemy[j].y, SmallExplosion0, 0);
					Play_Sound(InvaderKilled, INVADER_KILLED_ARRAY_SIZE);
					return;
				}
			}
		}
	}
	return;
}

// Enemy sprites fire missiles
// Input: pointers to enemyMissiles, freeIndex in EnemyMissiles array, enemyFire
// Output: void
void Enemy_Fire(uint32_t *enemyMissilesPtr, int32_t *freeIndexPtr, uint8_t *enemyFirePtr){
	uint32_t enemy;
	if ((*enemyMissilesPtr) == (MAX_ENEMY_MISSILES)){ // EnemyMissiles array full
			return;
		}
	
	if ((*enemyFirePtr) == ENEMY_FIRE_RATE_REDUCTION){ // slow down fire rate
	  enemy = (Random32()>>24)%MAX_ENEMIES; // a number from 0 to MAX_ENEMIES
	  // enemy alive and on screen	
	  if (Enemy[enemy].life > 0 && Enemy[enemy].x <= (LCD_WIDTH-ENEMY10W)){
	    //initialize missile
		  EnemyMissiles[(*freeIndexPtr)].x = Enemy[enemy].x + (ENEMY10W/2 - 1);
		  EnemyMissiles[(*freeIndexPtr)].y = ((ENEMY10H-1)+1)+LASERH;
		  EnemyMissiles[(*freeIndexPtr)].image[0] = Laser0;
		  EnemyMissiles[(*freeIndexPtr)].life = 1;
	    (*enemyMissilesPtr)++; // increment enemy missiles
	    Update_Free_Index(freeIndexPtr, enemyMissilesPtr, 'e');
	  }
	}
	
  (*enemyFirePtr) = ((*enemyFirePtr)+1)%(ENEMY_FIRE_RATE_REDUCTION+1);		
	
}

// Puts enemy missiles in buffer to be printed to screen
// Input: void
// Output: void
void Draw_Enemy_Missiles(void){ 
	int32_t i;
	for (i=0; i<MAX_ENEMY_MISSILES; i++){
		if (EnemyMissiles[i].life == 1){
			Nokia5110_PrintBMP(EnemyMissiles[i].x, EnemyMissiles[i].y, EnemyMissiles[i].image[0], 0);
		}
	}
}


// Moves enemy missile locations
// Input: pointers to num of enemyMissiles, free index in EnemyMissiles array
// Output: void
void Move_Enemy_Missiles(uint32_t *numMissilesPtr, int32_t *freeIndexPtr){ 
	int32_t i;
	for (i = 0; i < MAX_ENEMY_MISSILES; i++){
		if (EnemyMissiles[i].life == 1){
			// destroy missile that reached bottom of screen
			if ((EnemyMissiles[i].y - MISSILEH) > LCD_HEIGHT){
				EnemyMissiles[i].life = 0;
				(*numMissilesPtr)--;
				// missile array was previously full
				if ((*freeIndexPtr) == -1){
				  Update_Free_Index(freeIndexPtr, numMissilesPtr, 'e');
				}
				continue;
			}
			EnemyMissiles[i].y += 1; // move missile one pixel down
		}
	}			
}

// Determines if bunker is hit bt enemy missile. If true, bunker is degraded.
// Input: pointers to number of enemyMissiles and free index in EnemyMissiles array
// Output: void
void Bunker_Hit(uint32_t *numMissilesPtr, int32_t *freeIndexPtr){ 
	int32_t i;
	if (g_bnkrDamageLvl == BUNKER_DESTROYED){
	  return;
	}
	
	for (i=0; i<MAX_ENEMY_MISSILES; i++){
		// enemy missile in bunker space
	  if (EnemyMissiles[i].life == 1 && EnemyMissiles[i].x >= BUNKER_XPOS && 
			   EnemyMissiles[i].x < (BUNKER_XPOS + BUNKERW) && 
		      EnemyMissiles[i].y > (LCD_HEIGHT-1-PLAYERH-BUNKERH)){
			
						EnemyMissiles[i].life = 0; // kill missile
            g_bnkrDamageLvl++; // increase bunker damage
            (*numMissilesPtr)--;	// decrement num of enemy missiles
            // missile array was previously full
			      if ((*freeIndexPtr) == -1){
			        Update_Free_Index(freeIndexPtr, numMissilesPtr, 'e');
			      }
		}    				
	}
}

// Determines if a player missile hit an enemy missile.
// Input: pointers to number of enemyMissiles and free index in EnemyMissiles array
// Output: void
void Player_Missile_Hit_Enemy_Missile(uint32_t *enemyMissilesPtr, int32_t *freeEnemyMissIndexPtr){
	int32_t i, j;
	for (i=0; i<MAX_PLAYER_MISSILES; i++){
		if (PlayerMissiles[i].life){
			for (j=0; j<MAX_ENEMY_MISSILES; j++){
				if (EnemyMissiles[j].life){
					if ((PlayerMissiles[i].x + MISSILEW) > EnemyMissiles[j].x && 
						    PlayerMissiles[i].x < (EnemyMissiles[j].x + LASERW) &&
								 (PlayerMissiles[i].y - MISSILEH) < EnemyMissiles[j].y){
								    PlayerMissiles[i].life = 0;
									  EnemyMissiles[j].life = 0;
									  (*enemyMissilesPtr)--;
									  g_playerMissiles--;
									  if ((*freeEnemyMissIndexPtr) == -1){
			                Update_Free_Index(freeEnemyMissIndexPtr, enemyMissilesPtr, 'e');
			              }
										
										if (g_freePlayerMissileIndex == -1){
			                Update_Free_Index(&g_freePlayerMissileIndex, &g_playerMissiles, 'p');
			                }
								}
				}
			}
		}
	}
}

// Determines if enemy missile hit player ship.
// Input: pointers to playerLives, num of EnemyMissiles, and free index in EnemyMissiles array
// Output: void
void Enemy_Missile_Hit_PlayerShip(uint32_t *numMissilesPtr, 
                                   uint32_t *playerLivesPtr, int32_t *freeIndexPtr){
  int32_t i;
  for (i=0; i<MAX_ENEMY_MISSILES; i++){
	  if (EnemyMissiles[i].life && (EnemyMissiles[i].x + LASERW)>g_playerPos && 
			   EnemyMissiles[i].x<(g_playerPos + PLAYERW) && 
		      EnemyMissiles[i].y >= (LCD_HEIGHT - PLAYERH)){
		  (*playerLivesPtr)--;
			EnemyMissiles[i].life = 0;
			(*numMissilesPtr)--;
			// missile array was previously full
			if ((*freeIndexPtr) == -1){
			  Update_Free_Index(freeIndexPtr, numMissilesPtr, 'e');
			}
			Play_Sound(SmallExplosion, SMALL_EXPLOSION_ARRAY_SIZE);
		}
	}
}

// Puts player lives(hearts) in buffer to be printed to LCD
void Draw_Player_Life(uint32_t *playerLivesPtr) {
	uint32_t xpos; // xposition on the LCD
	xpos = LCD_WIDTH - ((*playerLivesPtr))*(PLAYER_LIFE_4W);
	
	for (; xpos<=(LCD_WIDTH-PLAYER_LIFE_4W); xpos += PLAYER_LIFE_4W) {
		Nokia5110_PrintBMP(xpos, PLAYER_LIFE_4H, PlayerLife4, 0);
	}
}

// Destroy all player missiles.
// Inputs: void 
//Outputs: void
static void Kill_Player_Missiles(void) { 
	int32_t i;
	for (i=0; i<MAX_PLAYER_MISSILES; i++) {
		PlayerMissiles[i].life = 0;
		// missile array was previously full
			if (g_freePlayerMissileIndex == -1){
			  Update_Free_Index(&g_freePlayerMissileIndex, &g_playerMissiles, 'p');
			}
	}
	
	g_playerMissiles = 0;
}

// Transition to the next level.
// Clears the LCD screen, kills player missiles, displays next level.
// Inputs: pointer to current level
// Outputs: none
static void Increase_Level_Transition(uint32_t *levelPtr){
  Nokia5110_Clear();
  Nokia5110_SetCursor(5, 15);
  if ((*levelPtr) == 2){
	  Nokia5110_OutString("LEVEL 2");
	} else if ((*levelPtr) == 3){
	  Nokia5110_OutString("LEVEL 3");
	} else if ((*levelPtr) == 4){
	  Nokia5110_OutString("LEVEL 4");
	}

  Delay1ms(1000);
	Kill_Player_Missiles();
}



// Advances player to next level.
// Reinitializes enemies.
// Input: pointers to current score, current level, and number of enemyMissiles
// Output: void
void Inrease_Level(uint32_t *scorePtr, uint32_t *levelPtr, uint32_t *enemyMissilesPtr){
	uint32_t numEnemies=0; // number of enemies alive
  uint32_t i;	

  // dont increase level if there are still enemy missiles on screen
	if ((*enemyMissilesPtr)){
	  return;
	}
											
  // count number of enemies that are alive
	for (i=0; i<MAX_ENEMIES; i++){
	  if (Enemy[i].life > 0){
		  numEnemies++;
		}
	}
	// dont increase level if there are still enemies on screen
	if (numEnemies){
	  return;
	}
	// transition to level 2
	if ((*scorePtr) >= LEVEL2_SCORE && (*levelPtr) == 1){
		(*levelPtr) = 2;
		Init_Enemy(ENEMY_LIVES_LVL2);
		Increase_Level_Transition(levelPtr);
		return;
	}
	// transition to level 3										
  if ((*scorePtr) >= LEVEL3_SCORE && (*levelPtr) == 2){
		(*levelPtr) = 3;
		Init_Enemy(ENEMY_LIVES_LVL3);
		Increase_Level_Transition(levelPtr);
		return;
	}
	// transition to level 4
	if ((*scorePtr) >= LEVEL4_SCORE && (*levelPtr) == 3){
		(*levelPtr) = 4;
		Init_Enemy(ENEMY_LIVES_LVL4);
		Increase_Level_Transition(levelPtr);
		return;
	}		
}
