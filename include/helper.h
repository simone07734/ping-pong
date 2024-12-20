#include <avr/io.h>
#include <avr/interrupt.h>
//#include <avr/signal.h>
#include <util/delay.h>
#ifndef HELPER_H
#define HELPER_H

unsigned char SetBit(unsigned char x, unsigned char k, unsigned char b) {
    return (b ? (x | (0x01 << k)) : (x & ~(0x01 << k)) );
    // Set bit to 1 Set bit to 0
}

unsigned char GetBit(unsigned char x, unsigned char k) {
    return ((x & (0x01 << k)) != 0);
}

int nums[16] = {0b1111110, 0b0110000, 0b1101101, 0b1111001, 0b0110011, 0b1011011,
0b1011111, 0b1110000, 0b1111111, 0b1111011, 0b1110111, 0b0011111, 0b1001110,
0b0111101, 0b1001111, 0b1000111 };
// a b c d e f g

void outNum(int num){
    PORTD = nums[num] << 1;
    PORTB = SetBit(PORTB, 1 ,nums[num]&0x01);
}

long map(long x, long in_min, long in_max, long out_min, long out_max) {
    long val = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    if (val > out_max) { val = out_max; }
    if (val < out_min) { val = out_min; }
    return val;
}

#endif /* HEPLER_H */