/*
 * mod.h
 *
 */

#ifndef MOD_H
#define MOD_H

#include <inttypes.h>

#include "midi.h"

#define EF_VOLUME 0x0C
#define EF_TEMPO 0x0F

static const int PERIOD[128] =
	{
	0,      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	1712,1616,1525,1440,1357,1281,1209,1141,1077,1017, 961, 907,
	856,  808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
	428,  404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
	214,  202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
	107,  101,  95,  90,  85,  80,  76,  71,  67,  64,  60,  57,
	0,      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,      0,   0,   0,   0,   0,   0,   0};

typedef struct {
	char name[23];
	uint16_t length;         // In words.
	int8_t fine_tune;        // -8 to 7, 1/8ths of a semitone up or down.
	uint8_t volume;          // 0 to 64, decibels = 20*log10(volume/64).
	uint16_t repeat_offset;  // In words.
	uint16_t repeat_length;  // In words.  Only loops if this is greater than 1.
} ModSample;

typedef struct {
	uint8_t sample;
	uint16_t period;

	uint8_t effect;
	uint8_t effect_x;
	uint8_t effect_y;
} ModCommand;

typedef struct {
	ModCommand data[8][64]; // [channel][division]
} ModPattern;

typedef struct {
	char title[20];
	ModSample *samples[32];
	uint8_t song_length;         // Number of patterns that the song consists of.
	uint8_t pattern_table[128];
	uint8_t num_channels;

	uint8_t num_patterns;
	ModPattern patterns[128];
} Mod;

typedef struct {
	long int time;  // Absolute time of event.
	MidiEvent *event;
} AbsoluteMidiEvent;

int compare_absolute_midi_event(const void *a, const void *b);
int midi_to_mod(Mod *, const Midi *);
int write_mod_file(Mod *, FILE *);

#endif /* MOD_H */
