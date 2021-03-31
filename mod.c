/*
 * mod.c
 *
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <math.h>

#include "midi.h"
#include "mod.h"

int
compare_absolute_midi_event(const void *a, const void *b)
{
	return ((const AbsoluteMidiEvent *)a)->time - ((const AbsoluteMidiEvent *)b)->time;
}

int
midi_to_mod(Mod *mod, const Midi *midi)
{
	int i, j, k;
	long int current_time;
	int current_pattern;
	int current_channel;
	long int total_events;
	ModCommand command;
	AbsoluteMidiEvent *events;
	MidiEvent *event;
	short unsigned int ticks_per_beat = 64;
	uint8_t tempo;
	short int division;
	char *channel_occupied; // For tracking whether or not a channel is free to play a note.
	uint8_t midi_channel_sample[16]; // Current sample that each midi channel is using.

	// TODO: This should probably be done better.
	// To hold notes that are currently on [midi channel][note number].
	struct {
		char on;
		short unsigned int channel; // Mod channel that it is on
	} current_note[16][128];
	memset(current_note, 0, sizeof(current_note));

	mod->num_channels = 8;

	total_events = 0;
	for(i=0; i < midi->num_tracks; i++) total_events += midi->tracks[i]->num_events;
	events = calloc(total_events, sizeof(AbsoluteMidiEvent));

	memset(mod->patterns, 0, sizeof(mod->patterns));

	k = 0;
	for(i=0; i < midi->num_tracks; i++) {
		current_time = 0;

		for(j=0; j < midi->tracks[i]->num_events; j++) {
			events[k].event = midi->tracks[i]->events[j];
			current_time += midi->tracks[i]->events[j]->delta_time;
			events[k++].time = current_time;
		}
	}

	qsort(events, total_events, sizeof(AbsoluteMidiEvent), compare_absolute_midi_event);

	memset(midi_channel_sample, 0, sizeof(midi_channel_sample));

	channel_occupied = calloc(mod->num_channels, sizeof(char));
	memset(channel_occupied, 0, sizeof(channel_occupied));

	current_pattern = 0;
	for(i=0; i < total_events; i++) {
		event = events[i].event;

		//printf("%-10d ", events[i].time);
		//print_midi_event(stdout, event);

		current_pattern = 1.0*events[i].time / ticks_per_beat / 64;
		division = ((1.0*events[i].time / ticks_per_beat) - (current_pattern*64));// * 4;

		if(event->type == MIDI_EVENT) {
			// event->delta_time, event->command, event->channel

			if(event->command == MIDI_NOTEON) {
				// skip percussion.  TAKE THIS OUT
				if(event->channel == 10) continue;

				//event->note, event->velocity
				if(current_note[event->channel][event->note].on) {
					current_channel = current_note[event->channel][event->note].channel;
				} else {
					current_channel = -1;
					for(j=0; j < mod->num_channels; j++) {
						command = mod->patterns[current_pattern].data[j][division];
						if(!channel_occupied[j] && !command.sample && !command.effect) {
							current_channel = j;
							break;
						}
					}
					if(current_channel < 0) continue;
				}

				current_note[event->channel][event->note].on = 1;
				current_note[event->channel][event->note].channel = current_channel;
				channel_occupied[current_channel] = 1;

				if(event->note > 71) {
					command.sample = 30;
					command.period = PERIOD[event->note - 12];
				} else {
					command.sample = midi_channel_sample[event->channel];
					command.period = PERIOD[event->note];
				}
				command.effect = EF_VOLUME;
				command.effect_x = ((event->velocity * 100 / 256) & 0xF0) >> 4;
				command.effect_y = (event->velocity * 100 / 256) & 0x0F;

				//command.effect = 0;
				//command.effect_x = 0;
				//command.effect_y = 0;

				mod->patterns[current_pattern].data[current_channel][division] = command;
			} else if(event->command == MIDI_NOTEOFF) {
				// skip percussion.  TAKE THIS OUT
				if(event->channel == 10) continue;

				current_channel = current_note[event->channel][event->note].channel;
				current_note[event->channel][event->note].on = 0;
				channel_occupied[current_channel] = 0;

				command.sample = 0;
				command.period = 0;
				command.effect = EF_VOLUME;
				command.effect_x = 0;
				command.effect_y = 0;

				mod->patterns[current_pattern].data[current_channel][division] = command;
			} else if(event->command == MIDI_PATCHCHANGE) {
				// TODO: the sample number needs to be mapped from
				// event->patch.
				//midi_channel_sample[event->channel] = event->patch;
				midi_channel_sample[event->channel] = 1;
			}
		} else if (event->type == MIDI_EVENT_META) {
			// event->delta_time, event->meta_type

			// Text event.
			if(
				event->meta_type >= MIDI_META_TEXT &&
				event->meta_type <= MIDI_META_CUEPOINT) {
				// event->data
			} else if(event->meta_type == MIDI_META_SETTEMPO) {
				// event->tempo
				tempo = 1.0/event->tempo * 60000000;

				current_channel = -1;
				for(j=0; j < mod->num_channels; j++) {
					command = mod->patterns[current_pattern].data[j][division];
					if(!channel_occupied[j] && !command.sample && !command.effect) {
						current_channel = j;
						break;
					}
				}
				if(current_channel < 0) continue;

				command.sample = 0;
				command.period = 0;
				command.effect = EF_TEMPO;
				command.effect_x = (tempo >> 4) & 0x0F;
				command.effect_y = tempo & 0x0F;
				
				mod->patterns[current_pattern].data[current_channel][division] = command;
				//printf("tempo: %d\n", tempo);
			} else if(event->meta_type == MIDI_META_TIMESIGNATURE) {
				ticks_per_beat = event->time_signature.ticks_per_click;
			} else {
				fprintf(stderr, "%d/%d ", current_pattern, division);
				print_midi_event(stderr, event);
			}
		} else if (event->type == MIDI_EVENT_SYSEX) {
			// event->delta_time, event->command
		} else {
			//fprintf(stderr, "Unknown MIDI event:\t%d,\t%d,\t%x\n", event->delta_time, event->type, event->command);
			fprintf(stderr, "%d/%d ", current_pattern, division);
			print_midi_event(stderr, event);
		}
	}

	mod->num_patterns = current_pattern + 1;
	memset(mod->pattern_table, 0, sizeof(mod->pattern_table));
	for(i=0; i < mod->num_patterns; i++) mod->pattern_table[i] = i;

	free(events);
	free(channel_occupied);

	return 0;
}

int
write_mod_file(Mod *mod, FILE *outfile)
{
	int i, d, c;
	char a[22];
	char b;
	ModCommand *command;
	uint32_t data;
	uint16_t z = 0;

	// Title
	fwrite("Test\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", sizeof(char), 20, outfile);

	memset(a, 0, sizeof(a));
	strcpy(a, "Sample ");
	for(i=0; i<31; i++) {
		a[7] = i+'a';
		fwrite(a, sizeof(a), 1, outfile);
		uint16_t length = 8287;
		length = htons(length);
		fwrite(&length, sizeof(uint16_t), 1, outfile);
		fwrite(&z, sizeof(uint8_t), 1, outfile);
		uint8_t v = 64;
		fwrite(&v, sizeof(uint8_t), 1, outfile);
		fwrite(&z, sizeof(uint16_t), 1, outfile);
		fwrite(&length, sizeof(uint16_t), 1, outfile);
	}

	fwrite(&mod->num_patterns, sizeof(uint8_t), 1, outfile);
	b = 127;
	fwrite(&b, sizeof(uint8_t), 1, outfile);

	fwrite(mod->pattern_table, sizeof(uint8_t), 128, outfile);
	//fwrite("M.K.", sizeof(char), 4, outfile);
	fwrite("8CHN", sizeof(char), 4, outfile);

	for(i=0; i < mod->num_patterns; i++) {
	//for(i=0; i < 64; i++) {
		for(d=0; d<64; d++) {
			for(c=0; c < mod->num_channels; c++) {
				command = &(mod->patterns[i].data[c][d]);
				data =
					((command->sample & 0xf0) << 24) |
					((command->period & 0x0FFF) << 16) |
					((command->sample & 0x0f) << 12) |
					((command->effect & 0x0f) << 8) |
					((command->effect_x & 0x0f) << 4) |
					(command->effect_y & 0x0f);

				data = htonl(data);
				fwrite(&data, sizeof(uint32_t), 1, outfile);
			}
		}
	}

	// Samples
	for(i=0; i<8; i++) {
		int8_t v = 0;
		fwrite(&v, sizeof(uint8_t), 1, outfile);
		fwrite(&v, sizeof(uint8_t), 1, outfile);
		for(d=2; d<16574; d++) {
			v = 128 * sin((1.0*d/16574)*2*3.14159 * 1024);
			//printf("%d\n", v);
			fwrite(&v, sizeof(uint8_t), 1, outfile);
		}
	}
	for(i=8; i<31; i++) {
		int8_t v = 0;
		fwrite(&v, sizeof(uint8_t), 1, outfile);
		fwrite(&v, sizeof(uint8_t), 1, outfile);
		for(d=2; d<16574; d++) {
			v = 128 * sin((1.0*d/16574)*2*3.14159 * 2048);
			//printf("%d\n", v);
			fwrite(&v, sizeof(uint8_t), 1, outfile);
		}
	}
}

