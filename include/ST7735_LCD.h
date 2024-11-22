/* 
VCC - 5V
GND - GND
CS - 10 - B2
RESET - 8 - B0
CD(A0) - 9 - B1
MOS(SDA) - 11 - B3
CSK(SCK) - 13 - B5
LED - 3.3V
*/

#include "helper.h"
#include "SPI_AVR.h"

const char SWRESET = 0x01;
const char CASET = 0x2A;
const char RASET = 0x2B;
const char RAMWR = 0x2C;
const char COLMOD = 0x3A;
const char SLPOUT = 0x11;
const char DISPON = 0x29;
const char MADCTL = 0x36;

void HardwareReset() {
    PORTB = SetBit(PORTB, 0, 0); // set reset pin to low
    _delay_ms(200);
    PORTB = SetBit(PORTB, 0, 1); // set reset pin to high
    _delay_ms(200);
}

void Send_Command(int cmd) {
    PORTB = SetBit(PORTB, 2, 0); // set CS pin to 0
    PORTB = SetBit(PORTB, 1, 0); // set A0 pin to 0
    SPI_SEND(cmd);
    PORTB = SetBit(PORTB, 2, 1); // set CS pin to 1
}
 
void Send_Data(int data) {
    PORTB = SetBit(PORTB, 2, 0); // set CS pin to 0
    PORTB = SetBit(PORTB, 1, 1); // set A0 pin to 1
    SPI_SEND(data);
    PORTB = SetBit(PORTB, 2, 1); // set CS pin to 1
}

void ST7735_init() {
    HardwareReset();
    Send_Command(SWRESET);
    _delay_ms(150);
    Send_Command(SLPOUT);
    _delay_ms(200);
    Send_Command(COLMOD);
    Send_Data(0x05); // for 16 bit color mode
    _delay_ms(10);
    Send_Command(DISPON);
    _delay_ms(200);
    Send_Command(MADCTL);
    _delay_ms(200);
    Send_Data(0b11101000); // turns the screen "sideways" to match game view
}

/* 
Inputs: xs (x start), xe (x end), ys (y start), ye (y end), color (16 bits)
Fill in the entire rectangle with the color
*/
void displayBlock(char xs, char xe, char ys, char ye, short color) {
    int i, j;

    // set location of block
    Send_Command(CASET);
    Send_Data(0);
    Send_Data(xs);
    Send_Data(0);
    Send_Data(xe);
    Send_Command(RASET);
    Send_Data(0);
    Send_Data(ys);
    Send_Data(0);
    Send_Data(ye);

    // set block color
    Send_Command(RAMWR);
    for (i = 0; i <= uint16_t(xe - xs); i++) {
        for (j = 0; j <= uint16_t(ye - ys); j++) {
            Send_Data((color & 0xF0) >> 8);
            Send_Data(color & 0x0F);
        }
    }
}

/* TODO: displayBitmap
Inputs: xs, xe, bitmap (using array, with first 2 vals = dimensions)
Behavior: Takes the top left corner. Uses the first 2 array vals to pick the size
we are drawing in. Iterate through the rest of the bitmap to draw the think
*/
