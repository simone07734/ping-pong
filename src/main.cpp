#include "helper.h"
#include "periph.h"
#include "ST7735_LCD.h"
#include "SPI_AVR.h"
#include "timerISR.h"

#define NUM_TASKS 3

// Game constants
const char PADDLE_WIDTH = 26;
const char PADDLE_DEPTH = 5;
const char BALL_DIAMETER = 4;
const short BACKGROUND_COLOR = (0x0000);
const short OBJECT_COLOR = (0xFFFF);

// Shared variables
unsigned char player1Loc[4] = {10, 10 + PADDLE_DEPTH, 52, 52 + PADDLE_WIDTH}; // in order: xs, xe, ys, ye
unsigned char player2Loc[4] = {119 - PADDLE_DEPTH, 119, 52, 52 + PADDLE_WIDTH}; 
unsigned char screenClear = 0;

// Task struct for concurrent synchSMs implmentations
typedef struct _task{
    signed char state; //Task's current state
    unsigned long period; //Task period
    unsigned long elapsedTime; //Time elapsed since last task tick
    int (*TickFct)(int); //Task tick function
} task;

// Task periods and GCD
const unsigned long GCD_PERIOD = 50;
const unsigned long START_GAME_PERIOD = 500;
const unsigned long PLAYER1_PERIOD = 100;
const unsigned long PLAYER2_PERIOD = 100;

task tasks[NUM_TASKS]; // task array

// Task functions and states declaration
enum StartGame { SG_INIT, SG_PLAY };
enum Player1 { P1_INIT, P1_MOVE };
enum Player2 { P2_INIT, P2_MOVE };
int Tick_Start_Game(int);
int Tick_Player1(int);
int Tick_Player2(int);

void TimerISR() {
    for ( unsigned int i = 0; i < NUM_TASKS; i++ ) { // Iterate through each task in the task array
        if ( tasks[i].elapsedTime == tasks[i].period ) { // Check if the task is ready to tick
            tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
            tasks[i].elapsedTime = 0; // Reset the elapsed time for the next tick
        }
        tasks[i].elapsedTime += GCD_PERIOD; // Increment the elapsed time by GCD_PERIOD
    }
}

int main() {
    DDRB = 0xff;
    PORTB = 0x00;
    DDRC = 0x00;
    PORTC = 0xff;
    DDRD = 0xff;
    PORTD = 0x00;
    
    SPI_INIT();
    ST7735_init();
    ADC_init();
    unsigned char i = 0;

    // initialize tasks
    tasks[i].state = SG_INIT;
    tasks[i].period = START_GAME_PERIOD;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Tick_Start_Game;
    i++;
    tasks[i].state = P1_INIT;
    tasks[i].period = PLAYER1_PERIOD;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Tick_Player1;
    i++;
    tasks[i].state = P2_INIT;
    tasks[i].period = PLAYER2_PERIOD;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Tick_Player2;

    TimerSet(GCD_PERIOD);
    TimerOn();
    while (1) {}
    return 0;
}

// Task function definitions

int Tick_Start_Game(int state) {
    switch (state) { // Transitions
        case SG_INIT:
            break;
        case SG_PLAY:
            break;
        default:
            state = SG_INIT;
            break;
    }
    switch (state) { // Actions
        case SG_INIT:
            // TODO: fix bug where it freezes when setting the whole screen
            displayBlock(0, 129, 0, 129, BACKGROUND_COLOR); // initialize empty board
            screenClear = 1;
            state = SG_PLAY;
            break;
        default:
            break;
    }
    return state;
}

int Tick_Player1(int state) {
    static unsigned char newLoc;
    switch (state) { // Transitions
        case P1_INIT:
            if (screenClear) { state = P1_MOVE; }
            break;
        case P1_MOVE:
            break;
        default:
            state = P1_INIT;
    }
    switch (state) { // Actions
        case P1_MOVE:
            displayBlock(player1Loc[0], player1Loc[1], player1Loc[2], player1Loc[3], BACKGROUND_COLOR); // clear previous paddle
            newLoc = map(ADC_read(1), 40, 1024, 0, 102); // get new paddle location
            player1Loc[0] = 10;
            player1Loc[1] = 10 + PADDLE_DEPTH;
            player1Loc[2] = newLoc; // update location info
            player1Loc[3] = newLoc + PADDLE_WIDTH;
            displayBlock(player1Loc[0], player1Loc[1], player1Loc[2], player1Loc[3], OBJECT_COLOR); // display paddle at new location
            break;
        default:
            break;
    }   
    return state;
}

int Tick_Player2(int state) {
    static unsigned char newLoc;
    switch (state) { // Transitions
        case P2_INIT:
            if (screenClear) { state = P2_MOVE; }
            break;
        case P2_MOVE:
            break;
        default:
            state = P2_INIT;
    }
    switch (state) { // Actions
        case P2_MOVE:
            displayBlock(player2Loc[0], player2Loc[1], player2Loc[2], player2Loc[3], BACKGROUND_COLOR); // clear previous paddle
            newLoc = map(ADC_read(2), 40, 1024, 0, 102); // get new paddle location
            player2Loc[0] = 119 - PADDLE_DEPTH;
            player2Loc[1] = 119;
            player2Loc[2] = newLoc; // update location info
            player2Loc[3] = newLoc + PADDLE_WIDTH;
            displayBlock(player2Loc[0], player2Loc[1], player2Loc[2], player2Loc[3], OBJECT_COLOR); // display paddle at new location
            break;
        default:
            break;
    }   
    return state;
}