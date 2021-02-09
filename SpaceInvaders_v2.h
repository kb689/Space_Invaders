/* SpaceInvaders_v2.h
// Runs on TM4C123.
// Used to play "Space Invaders" game on Nokia5110 display.
// Kirabo Nsereko
// 12/20/2020
*/

#ifndef SPACEINVADERS_V2_H
#define SPACEINVADERS_V2_H

//*****************************Game Constants********************************//

#define MAX_PLAYER_MISSILES  20 // maximum player missiles on screen
#define MAX_ENEMY_MISSILES	 10 // max enemy missiles on screen. should be 3x less than enemy missile arr len
#define LCD_WIDTH            84 // LCD is 84 pixels wide
#define LCD_HEIGHT           48 //LCD is 48 pixels heigh
#define MAX_ENEMIES          5  // max number of enemies (has to be 5 for "Init_Enemy(unsigned long)" to work
#define LEVEL2_SCORE				 50 // score needed to reach level 2
#define LEVEL3_SCORE         100 // score needed to reach level 3
#define LEVEL4_SCORE         150 // score needed to reach level 4
#define WINNING_SCORE        250 // score needed to win
#define LEVEL1_ENEMY_SPEED   1 // enemies move 1 pixel at a time on lvl 1
#define LEVEL2_3_ENEMY_SPEED 1 // enemies move 1 pixels at a time on lvl2,3  
#define MAX_PLAYER_LIVES     12 // max player lives
#define BUNKER_XPOS          33 // x coordinate of bunker on LCD
#define LIGHT_BKR_DAMAGE     10 // bunker hit by 6 enemy missiles
#define HEAVY_BKR_DAMAGE     20 // bunker hit by 12 enemy missiles
#define BUNKER_DESTROYED     30 // bunker hit by 18 enemy missiles
#define ENEMY_LIVES_LVL1     3 // enemy lives at level 1
#define ENEMY_LIVES_LVL2     4 // enemy lives at level 2
#define ENEMY_LIVES_LVL3     5 // enemy lives at level 3
#define ENEMY_LIVES_LVL4     6 // enemy lives at level 4
#define ENEMY_FIRE_RATE_REDUCTION 5 // factor at which enemy firing rate is reduced
#define SYSTEM_CLOCK         50000000 // 50MHz set by PLL

//************Data Structures of all objects in the game***************//

// data structure containing information on each enemy, player, and missile
struct State {
  uint32_t x;      // x coordinate
  uint32_t y;      // y coordinate
  const uint8_t *image[2]; // array of two pointers to images
  uint32_t life;            // 0=dead, 1=alive
};

typedef struct State STyp; // defines a state

//*********************************Function Prototypes*************************************//

void Switch_Init(void (*task)(void)); // initialize switches on PE1-0
void Timer3_Init(uint32_t); // initialize Timer3A
static void Update_Free_Index( volatile int32_t *, volatile uint32_t *, char); // locates free index in array
void SysTick_Init(uint32_t); // initialize SysTick Timer
void Draw_Player_Missiles(void); // puts missile data in print buffer
void Move_Player_Missiles(void); // moves player missiles
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void Fire_One_Missile(void); // fires on player missile
void Move_Player(void); // // Updates player position
void Draw_Player(void); // Put player ship location data in the buffer before printing.
void Init_Enemy(uint32_t); // Initializes enemies
void Draw_Enemy(uint32_t *);  // Put enemy sprite location data in the buffer before printing.
void Move_Enemy(void); // Move enemy sprites.
void Draw_Bunker(void); // Put bunker image in buffer to be printed.
void Enemy_Hit(uint32_t *); // Determines if an enemy is hit by a missile. Prints an explosion if true.
void Delay1ms(uint32_t); // Simple delay by a multiple of 1ms.
void Enemy_Fire(uint32_t *, int32_t *, uint8_t *); // Enemy sprites fire missiles
void Move_Enemy_Missiles(uint32_t *, int32_t *); // Moves enemy missile locations
void Draw_Enemy_Missiles(void); // Put enemy missile location data in the buffer before printing.
void Bunker_Hit(uint32_t *, int32_t *); // Determines if bunker is hit bt enemy missile. If true, bunker is degraded.
void Enemy_Missile_Hit_PlayerShip(uint32_t *, uint32_t *, int32_t *); // Determines if enemy missile hit player ship.
void Player_Missile_Hit_Enemy_Missile(uint32_t *, int32_t *); // Determines if a player missile hit an enemy missile.
void Draw_Player_Life(uint32_t *); // Puts player lives(hearts) in buffer to be printed to LCD.
static void Revive_Enemy_At_XPos0(uint32_t *); // Revives a dead enemy at position x=0.
void Revive_Enemy_If_Lvl_Not_Reached(uint32_t *, uint32_t *); // Revive dead enemy if the next level score not reached.
static void Kill_Player_Missiles(void); // Destroy all player missiles.
static void Increase_Level_Transition(uint32_t *); // Transition to the next level.
void Inrease_Level(uint32_t *, uint32_t *, uint32_t *); // Advances player to next level.
static void Timer2A_Stop(void); // Disables Timer2A.
static void Timer2A_Start(void); // Enables TImer2A.
void Timer2_Init(uint32_t rate, void (*task)(void)); // Initialize timer2A.
static void Sound_Values_to_DAC(void); // Sends values in "SoundWave" array to the DAC ("DAC_Out").
// Determines what sound will be played based on an event (eg player/enemy hit).
static void Play_Sound(const uint8_t *pt, uint32_t count);
void Game_Won(uint32_t *); // Determines if game has been won.
void Game_Lost(uint32_t *, uint32_t *); // Determines if game has been lost.
void Init_Unique_Random_Num(void); // Initialize the random num generator with a unique seed.

#endif
