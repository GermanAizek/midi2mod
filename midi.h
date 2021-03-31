/*
 * midi.h
 *
 * Heath Caldwell
 * hncaldwell@csupomona.edu
 *
 */

#ifndef MIDI_H
#define MIDI_H

#include <stdio.h>
#include <inttypes.h>

#include "stringcatalog.h"

#define MIDI_EVENT			0x01
#define MIDI_EVENT_SYSEX		0x02
#define MIDI_EVENT_META			0x03

/* Midi Event Commands */

#define MIDI_NOTEOFF			0x80
#define MIDI_NOTEON			0x90
#define MIDI_KEYAFTERTOUCH		0xA0
#define MIDI_CONTROLCHANGE		0xB0
#define MIDI_PATCHCHANGE		0xC0
#define MIDI_CHANNELAFTERTOUCH		0xD0
#define MIDI_PITCHWHEEL			0xE0

#define MIDI_META			0xFF
#define MIDI_META_SEQUENCENUMBER	0x00
#define MIDI_META_TEXT			0x01
#define MIDI_META_COPYRIGHT		0x02
#define MIDI_META_TRACKNAME		0x03
#define MIDI_META_INSTRUMENTNAME	0x04
#define MIDI_META_LYRIC			0x05
#define MIDI_META_MARKER		0x06
#define MIDI_META_CUEPOINT		0x07
#define MIDI_META_ENDTRACK		0x2F
#define MIDI_META_SETTEMPO		0x51
#define MIDI_META_TIMESIGNATURE		0x58
#define MIDI_META_KEYSIGNATURE		0x59
#define MIDI_META_SEQUENCERINFO		0x7F

#define MIDI_SYSEX			0xF0
#define MIDI_SYSEX_LITERAL		0xF7

/* */

extern const char *MIDI_NOTE_STRING[];
#define midi_note_string(x) MIDI_NOTE_STRING[(x)]

extern const int MIDI_EVENT_COMMAND_STRINGS_LENGTH;
extern const StringEntry MIDI_EVENT_COMMAND_STRINGS[];
extern const int MIDI_META_COMMAND_STRINGS_LENGTH;
extern const StringEntry MIDI_META_COMMAND_STRINGS[];
#define get_midi_event_command_string(x) get_string((x), MIDI_EVENT_COMMAND_STRINGS, MIDI_EVENT_COMMAND_STRINGS_LENGTH)
#define get_midi_meta_command_string(x) get_string((x), MIDI_META_COMMAND_STRINGS, MIDI_META_COMMAND_STRINGS_LENGTH)

typedef struct {
	uint8_t numerator;
	uint8_t denominator;
	uint8_t ticks_per_click;
	uint8_t n32_per_click;  // Number of 32nd notes per click.
} MidiTimeSignature;

typedef struct {
	uint32_t delta_time;
	uint8_t type;
	uint8_t command;
	uint8_t channel;
	uint8_t meta_type;
	uint8_t note;
	uint8_t velocity;
	uint8_t patch;
	uint32_t tempo;
	MidiTimeSignature time_signature;

	uint32_t data_length;
	uint8_t *data;
} MidiEvent;

typedef struct {
	uint32_t num_events;
	MidiEvent **events;
} MidiTrack;

typedef struct {
	uint8_t used; /* Whether or not this patch is used in this midi */
	uint8_t min;  /* Lowest note used in this patch */
	uint8_t max;  /* Highest note used in this patch */
} MidiPatch;

typedef struct {
	uint16_t format;  
	uint16_t division;
	uint32_t num_tracks;
	MidiTrack **tracks;
	MidiPatch patches[128];
} Midi;

int read_midi_from_file(Midi *, FILE *);
int read_midi_header(Midi *, FILE *);
int read_midi_track(MidiTrack *, MidiPatch *, int chan_patch[16], FILE *);

int get_midi_event(MidiEvent *, MidiPatch *, int chan_patch[16], const uint8_t *);
int get_l_quantity(uint32_t *, const uint8_t *);

void destroy_midi(Midi *);
void destroy_midi_track(MidiTrack *);
void destroy_midi_event(MidiEvent *);

void print_midi_event(FILE *, const MidiEvent *);

static const char *MIDI_PATCH_NAMES[] = {
			"Acoustic Grand Piano", "Bright Acoustic Piano",
			"Electric Grand Piano", "Honky-tonk Piano",
			"Electric Piano 1", "Electric Piano 2",
			"Harpsichord", "Clavi", "Celesta", "Glockenspiel",
			"Music Box", "Vibraphone", "Marimba", "Xylophone",
			"Tubular Bells", "Dulcimer", "Drawbar Organ",
			"Percussive Organ", "Rock Organ", "Church Organ",
			"Reed Organ", "Accordion", "Harmonica",
			"Tango Accordion", "Acoustic Guitar (nylon)",
			"Acoustic Guitar (steel)",
			"Electric Guitar (jazz)",
			"Electric Guitar (clean)",
			"Electric Guitar (muted)",
			"Overdriven Guitar", "Distortion Guitar",
			"Guitar harmonics", "Acoustic Bass",
			"Electric Bass (finger)", "Electric Bass (pick)",
			"Fretless Bass", "Slap Bass 1", "Slap Bass 2",
			"Synth Bass 1", "Synth Bass 2", "Violin", "Viola",
			"Cello", "Contrabass", "Tremolo Strings",
			"Pizzicato Strings", "Orchestral Harp", "Timpani",
			"String Ensemble 1", "String Ensemble 2",
			"SynthStrings 1", "SynthStrings 2", "Choir Aahs",
			"Voice Oohs", "Synth Voice", "Orchestra Hit",
			"Trumpet", "Trombone", "Tuba", "Muted Trumpet",
			"French Horn", "Brass Section", "SynthBrass 1",
			"SynthBrass 2", "Soprano Sax", "Alto Sax",
			"Tenor Sax", "Baritone Sax", "Oboe",
			"English Horn", "Bassoon", "Clarinet", "Piccolo",
			"Flute", "Recorder", "Pan Flute",
			"Blown Bottle", "Shakuhachi", "Whistle", "Ocarina",
			"Lead 1 (square)", "Lead 2 (sawtooth)",
			"Lead 3 (calliope)", "Lead 4 (chiff)",
			"Lead 5 (charang)", "Lead 6 (voice)",
			"Lead 7 (fifths)", "Lead 8 (bass + lead)",
			"Pad 1 (new age)", "Pad 2 (warm)",
			"Pad 3 (polysynth)", "Pad 4 (choir)",
			"Pad 5 (bowed)", "Pad 6 (metallic)",
			"Pad 7 (halo)", "Pad 8 (sweep)",
			"FX 1 (rain)", "FX 2 (soundtrack)",
			"FX 3 (crystal)", "FX 4 (atmosphere)",
			"FX 5 (brightness)", "FX 6 (goblins)",
			"FX 7 (echoes)", "FX 8 (sci-fi)", "Sitar",
			"Banjo", "Shamisen", "Koto", "Kalimba", "Bag pipe",
			"Fiddle", "Shanai", "Tinkle Bell", "Agogo",
			"Steel Drums", "Woodblock", "Taiko Drum",
			"Melodic Tom", "Synth Drum", "Reverse Cymbal",
			"Guitar Fret Noise", "Breath Noise", "Seashore",
			"Bird Tweet", "Telephone Ring", "Helicopter",
			"Applause", "Gunshot"};

#endif /* MIDI_H */

