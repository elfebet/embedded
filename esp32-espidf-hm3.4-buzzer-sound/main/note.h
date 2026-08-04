/*
 * note.h
 *
 *  Created on: 4 серп. 2026 р.
 *      Author: anton
 */

#ifndef MAIN_NOTE_H_
#define MAIN_NOTE_H_

#include <stdio.h>


// Table of base notes (Hz)
typedef enum {
    REST = 0,
    NOTE_C4 = 262,
    NOTE_D4 = 294,
    NOTE_E4 = 330,
    NOTE_F4 = 349,
    NOTE_G4 = 392,
    NOTE_A4 = 440,
    NOTE_B4 = 494,
    NOTE_C5 = 523,
    NOTE_D5 = 587,
    NOTE_E5 = 659,
    NOTE_F5 = 698,
    NOTE_G5 = 784,
    NOTE_A5 = 880,
    NOTE_B5 = 988
} NotePitch;

#define TICK_PERIOD_MS 50 // Base tick 50 ms

// Note duration in ticks (1 tick = 50 ms)
typedef enum {
    DUR_8TH  = 3,  // 150 ms
    DUR_4TH  = 6,  // 300 ms
    DUR_HALF = 12, // 600 ms
    DUR_FULL = 24  // 1200 ms
} NoteDuration;

typedef struct {
    uint16_t freq;      //  Hz, note frequency 
    uint8_t duration;   // tick duration (NoteDuration or any value)
} Note;

// Jingle Bells
static const Note NOTES_JINGLE_BELLS[] = {
    {NOTE_E4, DUR_4TH},
    {NOTE_E4, DUR_4TH},
    {NOTE_E4, DUR_HALF},
    {NOTE_E4, DUR_4TH},
    {NOTE_E4, DUR_4TH},
    {NOTE_E4, DUR_HALF},
    {NOTE_E4, DUR_4TH},
    {NOTE_G4, DUR_4TH},
    {NOTE_C4, DUR_4TH},
    {NOTE_D4, DUR_4TH},
   {NOTE_E4, DUR_FULL},
   {0, 0}  // finish marker
};

// Baby Shark
static const Note NOTES_BABY_SHARK[] = {
    {NOTE_C4, DUR_4TH},
    {NOTE_D4, DUR_4TH},
    {NOTE_E4, DUR_HALF},
    {NOTE_C4, DUR_4TH},
    {NOTE_D4, DUR_4TH},
    {NOTE_E4, DUR_HALF},
    {NOTE_C4, DUR_4TH},
    {NOTE_D4, DUR_4TH},
    {NOTE_E4, DUR_HALF},
    {0, 0} // finish marker
};

// Twinkle Twinkle Little Star
static const Note NOTES_TWINKLE[] = {
    {NOTE_C4, DUR_4TH},
    {NOTE_C4, DUR_4TH},
    {NOTE_G4, DUR_4TH},
    {NOTE_G4, DUR_4TH},
    {NOTE_A4, DUR_4TH},
    {NOTE_A4, DUR_4TH},
    {NOTE_G4, DUR_HALF},
    {NOTE_F4, DUR_4TH},
    {NOTE_F4, DUR_4TH},
    {NOTE_E4, DUR_4TH},
   {NOTE_E4, DUR_4TH},
   {NOTE_D4, DUR_4TH},
   {NOTE_D4, DUR_4TH},
   {NOTE_C4, DUR_HALF},
   {0, 0} // finish marker
};

// We Will Rock You
static const Note NOTES_WE_WILL_ROCK_YOU[] = {
    {NOTE_C4, DUR_8TH},
    {NOTE_C4, DUR_8TH},
    {NOTE_D4, DUR_4TH},
    {NOTE_C4, DUR_8TH},
    {NOTE_C4, DUR_8TH},
    {NOTE_D4, DUR_4TH},
    {NOTE_C4, DUR_8TH},
    {NOTE_C4, DUR_8TH},
    {NOTE_D4, DUR_4TH},
    {NOTE_E4, DUR_HALF},
    {0, 0} // finish marker
};

typedef enum {
    MELODY_JINGLE_BELLS = 0,
    MELODY_BABY_SHARK,
    MELODY_TWINKLE,
    MELODY_WE_WILL_ROCK_YOU,
} Melody;

#endif /* MAIN_NOTE_H_ */
