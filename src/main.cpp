#include "helper.h"
#include "ST7735_LCD.h"
#include "SPI_AVR.h"
#include "timerISR.h"

#define NUM_TASKS 0

// Shared variables

// Task struct for concurrent synchSMs implmentations
typedef struct _task{
    signed char state; //Task's current state
    unsigned long period; //Task period
    unsigned long elapsedTime; //Time elapsed since last task tick
    int (*TickFct)(int); //Task tick function
} task;

// Task periods and GCD
const unsigned long GCD_PERIOD = 100;

task tasks[NUM_TASKS]; // task array

// Task functions and states declaration

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
    
    SPI_INIT();
    ST7735_init();

    displayBlock(10,30,0,130,0x881f);
    return 0;
}