#include "helper.h"
#include "periph.h"
#include "ST7735_LCD.h"
#include "SPI_AVR.h"
#include "timerISR.h"

#define NUM_TASKS 4

// Game constants
const char PADDLE_WIDTH = 26;
const char PADDLE_DEPTH = 5;
const char BALL_DIAMETER = 4;
const short BACKGROUND_COLOR = (0x0000);
const short OBJECT_COLOR = (0xFFFF);

// Shared variables
unsigned char player1Loc[4] = {10, 10 + PADDLE_DEPTH, 52, 52 + PADDLE_WIDTH}; // in order: xs, xe, ys, ye
unsigned char player2Loc[4] = {119 - PADDLE_DEPTH, 119, 52, 52 + PADDLE_WIDTH}; 
unsigned char ballLoc[4] = {62, 62 + BALL_DIAMETER, 62, 62 + BALL_DIAMETER}; // in order: xs, xe, ys, ye
signed char ballVec[2] = {2,2}; // in order:x, y
unsigned char screenClear = 0;

// Task struct for concurrent synchSMs implmentations
typedef struct _task{
    signed char state; //Task's current state
    unsigned long period; //Task period
    unsigned long elapsedTime; //Time elapsed since last task tick
    int (*TickFct)(int); //Task tick function
} task;

// Task periods and GCD
const unsigned long GCD_PERIOD = 25;
const unsigned long START_GAME_PERIOD = 500;
const unsigned long PLAYER1_PERIOD = 25;
const unsigned long PLAYER2_PERIOD = 25;
const unsigned long BALL_PERIOD = 25;

task tasks[NUM_TASKS]; // task array

// Task functions and states declaration
enum StartGame { SG_INIT, SG_PLAY };
enum Player1 { P1_INIT, P1_MOVE };
enum Player2 { P2_INIT, P2_MOVE };
enum Ball { B_INIT, B_PLAY };
int Tick_Start_Game(int);
int Tick_Player1(int);
int Tick_Player2(int);
int Tick_Ball(int);

// Helper function declaration
int checkCollision(void);

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
    i++;
    tasks[i].state = B_INIT;
    tasks[i].period = BALL_PERIOD;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Tick_Ball;

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
            break;
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
            break;
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

int Tick_Ball(int state) {
    static unsigned char newX;
    static unsigned char newY;
    switch (state) { // Transitions
        case B_INIT:
            if (screenClear) { state = B_PLAY; }
            break;
        case B_PLAY:
            break;
        default:
            state = B_INIT;
            break;
    }
    switch (state) { // Actions
        case B_PLAY:

            checkCollision();
            displayBlock(ballLoc[0], ballLoc[1], ballLoc[2], ballLoc[3], BACKGROUND_COLOR); // clear previous paddle
            
            // TODO: move into helper function
            newX = ballLoc[0] + ballVec[0]; // get new ball xs
            newY = ballLoc[2] + ballVec[1]; // get new ball ys

            if(newX <= 0) {newX = 0;}
            if(newX >= 128) {newX = 128;}
            if(newY <= 0) {newY = 0;}
            if(newY >= 128) {newY = 128;}

            ballLoc[0] = newX;
            ballLoc[1] = newX + BALL_DIAMETER;
            ballLoc[2] = newY;
            ballLoc[3] = newY + BALL_DIAMETER;
            
            displayBlock(ballLoc[0], ballLoc[1], ballLoc[2], ballLoc[3], OBJECT_COLOR); // display paddle at new location
            break;
        default:
            break;
    }   
    return state;
}

// Helper functions
/* Checks whether the ball has collided with the walls or paddles, and changes it's vector accordingly.
   Should be called before clearing the old ball from the display. 
   Returns 1 when player 1 scores, 2 when player 2 scores, and 0 when nobody scores. */
int checkCollision(void) {
    // side walls
    if (ballLoc[2] <= 2 || ballLoc[3] >= 127) {
        ballVec[1] *= -1;
    }

    // paddle 1
    if ((ballLoc[0] >= player1Loc[0] && ballLoc[0] <= player1Loc[1])) {
        // adjust vector based on where on the paddle it hits
        if (ballLoc[3] >= player1Loc[2] && ballLoc[3] <= player1Loc[2] + 8) {
            ballVec[0] *= -1;
            ballVec[1] += 1;
        }
        else if (ballLoc[3] >= player1Loc[2] + 8 && ballLoc[3] <= player1Loc[3] - 8) {
            ballVec[0] *= -1;
            ballVec[1] -= 1;
        }
        else if (ballLoc[3] >= player1Loc[3] - 8 && ballLoc[2] <= player1Loc[3]) {
            ballVec[0] *= -1;
            ballVec[1] += 1; 
        }
        else {}
        if (ballVec[1] > 4) { ballVec[1] = 4; }
        if (ballVec[1] < 1) { ballVec[1] = 1; }
    }

    // paddle 2
    if (ballLoc[1] >= player2Loc[0] && ballLoc[1] <= player2Loc[1]) {
        // adjust vector based on where on the paddle it hits
        if (ballLoc[3] >= player2Loc[2] && ballLoc[3] <= player2Loc[2] + 8) {
            ballVec[0] *= -1;
            ballVec[1] += 1;
        }
        else if (ballLoc[3] >= player2Loc[2] + 8 && ballLoc[3] <= player2Loc[3] - 8) {
            ballVec[0] *= -1;
            ballVec[1] -= 1;
        }
        else if (ballLoc[3] >= player2Loc[3] - 8 && ballLoc[2] <= player2Loc[3]) {
            ballVec[0] *= -1;
            ballVec[1] += 1; 
        }
        else {}
        if (ballVec[1] > 4) { ballVec[1] = 4; }
        if (ballVec[1] < 1) { ballVec[1] = 1; }
    }

    // behind paddle 1
    if (ballLoc[0] <= 4) {
        ballLoc[0] = 50;
        ballLoc[1] = 54;
        ballLoc[2] = 50;
        ballLoc[3] = 54;
        return 2; // player 2 gets a point
    }

    // behind paddle 2
    if (ballLoc[1] >= 125) {
        ballLoc[0] = 50;
        ballLoc[1] = 54;
        ballLoc[2] = 50;
        ballLoc[3] = 54;
        return 1; // player 1 gets a point
    }
    
    return 0; // no points awarded
}