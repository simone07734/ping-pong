#include "helper.h"
#include "periph.h"
#include "ST7735_LCD.h"
#include "SPI_AVR.h"
#include "timerISR.h"

#define NUM_TASKS 6

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
unsigned char gameStatus = 0; // 1 for game in progress, 0 otherwise
unsigned char numPlayers = 2; // can be 1 or 2 players
unsigned char pointScored = 0; // 1 for point scored by player 1, 2 for point scored by player 2
unsigned char player1Score = 0;
unsigned char plyaer2Score = 0;

// Task struct for concurrent synchSMs implmentations
typedef struct _task{
    signed char state; //Task's current state
    unsigned long period; //Task period
    unsigned long elapsedTime; //Time elapsed since last task tick
    int (*TickFct)(int); //Task tick function
} task;

// Task periods and GCD
const unsigned long GCD_PERIOD = 25;
const unsigned long GAME_MANAGER_PERIOD = 100;
const unsigned long START_RESET_BUTTON_PERIOD = 200;
const unsigned long PLAYER_TOGGLE_BUTTON_PERIOD = 200;
const unsigned long PLAYER1_PERIOD = 25;
const unsigned long PLAYER2_PERIOD = 25;
const unsigned long BALL_PERIOD = 25;

task tasks[NUM_TASKS]; // task array

// Task functions and states declaration
enum GameManager { GM_INIT, GM_PLAY, GM_POINT };
enum StartResetButton { SR_RESET, SR_PRESS_START, SR_START, SR_PRESS_RESET };
enum PlayerToggleButton { PT_TWO, PT_PRESS_ONE, PT_ONE, PT_PRESS_TWO };
enum Player1 { P1_INIT, P1_MOVE };
enum Player2 { P2_INIT, P2_MOVE, P2_AUTO };
enum Ball { B_INIT, B_FLASH, B_MOVE };
int Tick_Game_Manager(int);
int Tick_Start_Reset(int);
int Tick_Player_Toggle(int);
int Tick_Player1(int);
int Tick_Player2(int);
int Tick_Ball(int);

// Helper function declaration
int checkCollision(void);
void moveBall(void);
void autonomousPlayer2(void);

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
    tasks[i].state = GM_INIT;
    tasks[i].period = GAME_MANAGER_PERIOD;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Tick_Game_Manager;
    i++;
    tasks[i].state = SR_RESET;
    tasks[i].period = START_RESET_BUTTON_PERIOD;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Tick_Start_Reset;
    i++;
    tasks[i].state = PT_TWO;
    tasks[i].period = PLAYER_TOGGLE_BUTTON_PERIOD;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Tick_Player_Toggle;
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

int Tick_Game_Manager(int state) {
    switch (state) { // Transitions
        case GM_INIT:
            if (gameStatus) {
                state = GM_PLAY;    
            }
            break;
        case GM_PLAY:
            break;
        case GM_POINT:
            break;
        default:
            state = GM_INIT;
            break;
    }
    switch (state) { // Actions
        case GM_INIT:
            displayBlock(0, 129, 0, 129, BACKGROUND_COLOR); // initialize empty board
            break;
        default:
            break;
    }
    return state;
}

int Tick_Start_Reset(int state) {
    switch(state) { // Transitions
        case SR_RESET:
            if (GetBit(PINC, 3)) {
                state = SR_PRESS_START;
            }
            break;
        case SR_PRESS_START:
            if (!GetBit(PINC, 3)) {
                state = SR_START;
                gameStatus = 1;
            }
            break;
        case SR_START:
            if (GetBit(PINC, 3)) {
                state = SR_PRESS_RESET;
            }
            break;
        case SR_PRESS_RESET:
            if (!GetBit(PINC, 3)) {
                state = SR_RESET;
                gameStatus = 0;
            }
            break;
        default:
            break;
    }
    return state;
}

int Tick_Player_Toggle(int state) {
    switch(state) { // Transitions
        case PT_TWO:
            if (!gameStatus && GetBit(PINC, 4)) {
                state = PT_PRESS_ONE;
            }
            break;
        case PT_PRESS_ONE:
            if (!gameStatus && !GetBit(PINC, 4)) {
                state = PT_ONE;
                numPlayers = 1;
            }
            break;
        case PT_ONE:
            if (!gameStatus && GetBit(PINC, 4)) {
                state = PT_PRESS_TWO;
            }
            break;
        case PT_PRESS_TWO:
            if (!gameStatus && !GetBit(PINC, 4)) {
                state = PT_TWO;
                numPlayers = 2;
            }
            break;
        default:
            state = PT_TWO;
            break;
    }
    return state;
}

int Tick_Player1(int state) {
    static unsigned char newLoc;
    switch (state) { // Transitions
        case P1_INIT:
            if (gameStatus) {
                state = P1_MOVE;
            } 
            break;
        case P1_MOVE:
            if (!gameStatus) {
                state = P1_INIT;
            }
            break;
        default:
            state = P1_INIT;
            break;
    }
    switch (state) { // Actions
        case P1_INIT:
            displayBlock(player1Loc[0], player1Loc[1], player1Loc[2], player1Loc[3], BACKGROUND_COLOR); // clear paddle
            break;
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
            if (gameStatus) {
                if (numPlayers == 2) {
                    state = P2_MOVE;
                }
                else {
                    state = P2_AUTO;
                }
            }
            break;
        case P2_MOVE:
            if (!gameStatus) {
                state = P2_INIT;
            }
            break;
        case P2_AUTO:
            if (!gameStatus) {
                state = P2_INIT;
            }
            break;
        default:
            state = P2_INIT;
            break;
    }
    switch (state) { // Actions
        case P2_INIT:
            displayBlock(player2Loc[0], player2Loc[1], player2Loc[2], player2Loc[3], BACKGROUND_COLOR); // clear paddle
            break;
        case P2_MOVE:
            displayBlock(player2Loc[0], player2Loc[1], player2Loc[2], player2Loc[3], BACKGROUND_COLOR); // clear previous paddle
            newLoc = map(ADC_read(2), 40, 1024, 0, 102); // get new paddle location
            player2Loc[0] = 119 - PADDLE_DEPTH;
            player2Loc[1] = 119;
            player2Loc[2] = newLoc; // update location info
            player2Loc[3] = newLoc + PADDLE_WIDTH;
            displayBlock(player2Loc[0], player2Loc[1], player2Loc[2], player2Loc[3], OBJECT_COLOR); // display paddle at new location
            break;
        case P2_AUTO:
            autonomousPlayer2();
        default:
            break;
    }   
    return state;
}

int Tick_Ball(int state) {
    static unsigned char i;
    switch (state) { // Transitions
        case B_INIT:
            // ball to default settings
            ballVec[0] = 2;
            ballVec[1] = 2;
            ballLoc[0] = 62;
            ballLoc[1] = 62 + BALL_DIAMETER;
            ballLoc[2] = 62;
            ballLoc[3] = 62 + BALL_DIAMETER;
            displayBlock(ballLoc[0], ballLoc[1], ballLoc[2], ballLoc[3], OBJECT_COLOR);

            if (gameStatus) {
                state = B_FLASH;
                i = 0;
            }
            break;
        case B_FLASH:
            if (i > 120) {
                state = B_MOVE;
            }
            break;
        case B_MOVE:
            if (!gameStatus) {
                state = B_INIT;
                displayBlock(ballLoc[0], ballLoc[1], ballLoc[2], ballLoc[3], BACKGROUND_COLOR);
                break;
            }
            if (pointScored) {
                state = B_FLASH;
                i = 0;
                displayBlock(ballLoc[0], ballLoc[1], ballLoc[2], ballLoc[3], BACKGROUND_COLOR);
            }
            break;
        default:
            state = B_INIT;
            break;
    }
    switch (state) { // Actions
        case B_INIT:
            break;
        case B_FLASH:
            if ((i / 20) % 2 == 0) {
               displayBlock(ballLoc[0], ballLoc[1], ballLoc[2], ballLoc[3], OBJECT_COLOR); 
            }
            else {
                displayBlock(ballLoc[0], ballLoc[1], ballLoc[2], ballLoc[3], BACKGROUND_COLOR);  
            }
            i++;
            break;
        case B_MOVE:
            pointScored = checkCollision();
            moveBall();            
            break;
        default:
            break;
    }   
    return state;
}

// Helper functions

/* Moves the ball according to ballVec*/
void moveBall(void) {
    unsigned char newX;
    unsigned char newY;

    displayBlock(ballLoc[0], ballLoc[1], ballLoc[2], ballLoc[3], BACKGROUND_COLOR); // clear previous ball
    
    newX = ballLoc[0] + ballVec[0]; // get new ball xs
    newY = ballLoc[2] + ballVec[1]; // get new ball ys

    // keep ball within bounds of screen
    if(newX <= 0) {newX = 0;}
    if(newX >= 128) {newX = 128;}
    if(newY <= 0) {newY = 0;}
    if(newY >= 128) {newY = 128;}

    // record new location of ball
    ballLoc[0] = newX;
    ballLoc[1] = newX + BALL_DIAMETER;
    ballLoc[2] = newY;
    ballLoc[3] = newY + BALL_DIAMETER;

    displayBlock(ballLoc[0], ballLoc[1], ballLoc[2], ballLoc[3], OBJECT_COLOR); // display ball at new location
}

/* Checks whether the ball has collided with the walls or paddles, and changes it's vector accordingly.
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
        displayBlock(ballLoc[0], ballLoc[1], ballLoc[2], ballLoc[3], BACKGROUND_COLOR); // clear previous paddle
        ballLoc[0] = 62;
        ballLoc[1] = 62 + BALL_DIAMETER;
        ballLoc[2] = 62;
        ballLoc[3] = 62 + BALL_DIAMETER;
        return 2; // player 2 gets a point
    }

    // behind paddle 2
    if (ballLoc[1] >= 125) {
        displayBlock(ballLoc[0], ballLoc[1], ballLoc[2], ballLoc[3], BACKGROUND_COLOR); // clear previous paddle
        ballLoc[0] = 62;
        ballLoc[1] = 62 + BALL_DIAMETER;
        ballLoc[2] = 62;
        ballLoc[3] = 62 + BALL_DIAMETER;
        return 1; // player 1 gets a point
    }
    
    return 0; // no points awarded
}

/* Moves paddle 2 autonomously towards the ball for 1 player games. */
void autonomousPlayer2(void) {
    displayBlock(player2Loc[0], player2Loc[1], player2Loc[2], player2Loc[3], BACKGROUND_COLOR);  // clear previous paddle
    if (ballLoc[2] <= player2Loc[2]) {
        player2Loc[2] -= 1;
        player2Loc[3] -= 1;
    }
    else if (ballLoc[3] >= player2Loc[3]) {
        player2Loc[2] += 2;
        player2Loc[3] += 2;
    }
    displayBlock(player2Loc[0], player2Loc[1], player2Loc[2], player2Loc[3], OBJECT_COLOR); // display new paddle
}