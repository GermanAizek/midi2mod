/*
 * midi.c
 *
 * Heath Caldwell
 * hncaldwell@csupomona.edu
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "midi.h"
#include "stringcatalog.h"

#ifdef _WIN32
#include "winsock2.h"
#else
#include <arpa/inet.h>
#endif

const char *MIDI_NOTE_STRING[128] =
	{"C,,,,", "C#,,,,", "D,,,,", "D#,,,,", "E,,,,", "F,,,,", "F#,,,,", "G,,,,", "G#,,,,", "A,,,,", "A#,,,,", "B,,,,",
	 "C,,,", "C#,,,", "D,,,", "D#,,,", "E,,,", "F,,,", "F#,,,", "G,,,", "G#,,,", "A,,,", "A#,,,", "B,,,",
	 "C,,", "C#,,", "D,,", "D#,,", "E,,", "F,,", "F#,,", "G,,", "G#,,", "A,,", "A#,,", "B,,",
	 "C,", "C#,", "D,", "D#,", "E,", "F,", "F,#", "G,", "G#,", "A,", "A#,", "B,",
	 "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B",
	 "c", "c#", "d", "d#", "e", "f", "f#", "g", "g#", "a", "a#", "b",
	 "c'", "c#'", "d'", "d#'", "e'", "f'", "f#'", "g'", "g#'", "a'", "a#'", "b'",
	 "c''", "c#''", "d''", "d#''", "e''", "f''", "f#''", "g''", "g#''", "a''", "a#''", "b''",
	 "c'''", "c#'''", "d'''", "d#'''", "e'''", "f'''", "f#'''", "g'''", "g#'''", "a'''", "a#'''", "b'''",
	 "c''''", "c#''''", "d''''", "d#''''", "e''''", "f''''", "f#''''", "g''''", "g#''''", "a''''", "a#''''", "b''''",
	 "c'''''", "c#'''''", "d'''''", "d#'''''", "e'''''", "f'''''", "f#'''''", "g'''''"};

const int MIDI_EVENT_COMMAND_STRINGS_LENGTH = 7;
const StringEntry MIDI_EVENT_COMMAND_STRINGS[] =
	{{MIDI_NOTEOFF, "note off"},
	 {MIDI_NOTEON, "note on"},
	 {MIDI_KEYAFTERTOUCH, "key after touch"},
	 {MIDI_CONTROLCHANGE, "control change"},
	 {MIDI_PATCHCHANGE, "patch change"},
	 {MIDI_CHANNELAFTERTOUCH, "channel after touch"},
	 {MIDI_PITCHWHEEL, "pitch wheel"}};

const int MIDI_META_COMMAND_STRINGS_LENGTH = 13;
const StringEntry MIDI_META_COMMAND_STRINGS[] =
	{{MIDI_META_SEQUENCENUMBER, "sequence number"},
	 {MIDI_META_TEXT,           "text"},
	 {MIDI_META_COPYRIGHT,      "copyright"},
	 {MIDI_META_TRACKNAME,      "track name"},
	 {MIDI_META_INSTRUMENTNAME, "instrument name"},
	 {MIDI_META_LYRIC,          "lyric"},
	 {MIDI_META_MARKER,         "marker"},
	 {MIDI_META_CUEPOINT,       "cue point"},
	 {MIDI_META_ENDTRACK,       "end of track"},
	 {MIDI_META_SETTEMPO,       "set tempo"},
	 {MIDI_META_TIMESIGNATURE,  "time signature"},
	 {MIDI_META_KEYSIGNATURE,   "key signature"},
	 {MIDI_META_SEQUENCERINFO,  "sequencer specific information"}};

int
read_midi_from_file(Midi *midi, FILE *infile)
{
	MidiTrack *track;
	int i;
	int chan_patch[16];

    if (read_midi_header(midi, infile)) {
        return 1;
    }

	memset(midi->patches, 0, sizeof(midi->patches));
	memset(chan_patch, 0, sizeof(chan_patch));
	for(i=0; i < midi->num_tracks; i++) {
		track = calloc(1, sizeof(MidiTrack));
		if(!read_midi_track(track, midi->patches, chan_patch, infile))
			midi->tracks[i] = track;
	}

	return 0;
}

int
read_midi_header(Midi *midi, FILE *infile)
{
	uint8_t MThd[4];
	uint32_t size;
	uint16_t format;
	uint16_t num_tracks;
	uint16_t division;
	uint16_t i;

	if(!fread(MThd, sizeof(MThd), 1, infile)) {
		fprintf(stderr, "Unable to read midi midi.\n");
		return 1;
	}

	if(MThd[0] != 'M' || MThd[1] != 'T' || MThd[2] != 'h' || MThd[3] != 'd') {
		fprintf(stderr, "Not a midi file.\n");
		return 1;
	}

	if(!fread(&size, sizeof(size), 1, infile)) {
		fprintf(stderr, "Unable to read size.\n");
		return 1;
	}
	size = ntohl(size);

	if(size != 6) {
		fprintf(stderr, "Header is of incorrect size.\n");
		return 1;
	}

	if(!fread(&format, sizeof(format), 1, infile)) {
		fprintf(stderr, "Unable to read format.\n");
		return 1;
	}
	format = ntohs(format);

	if(!fread(&num_tracks, sizeof(num_tracks), 1, infile)) {
		fprintf(stderr, "Unable to read tracks.\n");
		return 1;
	}
	num_tracks = ntohs(num_tracks);

	if(!fread(&division, sizeof(division), 1, infile)) {
		fprintf(stderr, "Unable to read division.\n");
		return 1;
	}
	division = ntohs(division);

	midi->format = format;
	midi->division = division;
	midi->num_tracks = num_tracks;
	midi->tracks = calloc(num_tracks, sizeof(MidiTrack));

	for(i=0; i < num_tracks; i++) midi->tracks[i] = NULL;

	return 0;
}

// patches: 128 element array of MidiPatches
// chan_patch: Mapping of channel number to patch number
int
read_midi_track(MidiTrack *track, MidiPatch *patches, int chan_patch[16], FILE *infile)
{
	uint8_t MTrk[4];
	uint32_t length;

	int32_t event_length;
	MidiEvent *event;

	uint8_t *data;
	uint8_t *head;

	track->num_events = 0;
	track->events = NULL;

	if(!fread(MTrk, sizeof(MTrk), 1, infile)) {
		fprintf(stderr, "Unable to read track header.\n");
		return 1;
	}

	if(MTrk[0] != 'M' || MTrk[1] != 'T' || MTrk[2] != 'r' || MTrk[3] != 'k') {
		fprintf(stderr, "Not a track.\n");
		return 1;
	}

	if(!fread(&length, sizeof(length), 1, infile)) {
		fprintf(stderr, "Unable to read track length.\n");
		return 1;
	}
	length = ntohl(length);

	data = calloc(length, sizeof(uint8_t));
	head = data;
	if(!fread(data, sizeof(uint8_t), length, infile)) {
		fprintf(stderr, "Unable to read track data.\n");
		return 1;
	}
	
	while(head < data + length) {
		event = calloc(1, sizeof(MidiEvent));

		event_length = get_midi_event(event, patches, chan_patch, head);
		if(event_length < 0) {
			fprintf(stderr, "Error reading event.\n");
			return 1;
		}

		track->num_events++;

		track->events = realloc(track->events, track->num_events * sizeof(MidiEvent *));
		if(!track->events) {
			fprintf(stderr, "Out of memory.\n");
			return 1;
		}

		track->events[track->num_events - 1] = event;

		head += event_length;
	}

	free(data);

	return 0;
}

// patches: 128 element array of MidiPatches
// chan_patch: Mapping of channel number to patch number
int
get_midi_event(MidiEvent *event, MidiPatch *patches, int chan_patch[16], const uint8_t *data)
{
	uint8_t *head = (uint8_t *)data;
	uint8_t command;
	uint8_t type;
	uint32_t command_length;
	uint32_t delta_time;
	int quantity_length;
	MidiPatch *patch;

	event->data_length = 0;
	event->data = NULL;

	quantity_length = get_vl_quantity(&delta_time, head);
	if(quantity_length < 0) {
		fprintf(stderr, "Bad delta time.\n");
		return 1;
	}
	head += quantity_length;
	event->delta_time = delta_time;

	command = *(head++);

	// If it is a midi event (not meta or sysex), then the first nibble of
	// command must only be checked to see if it is in the range.
	if     (command >= MIDI_NOTEOFF && (command & 0xF0) <= MIDI_PITCHWHEEL) type = MIDI_EVENT;
	else if(command == MIDI_META)                                           type = MIDI_EVENT_META;
	else if(command == MIDI_SYSEX || command == MIDI_SYSEX_LITERAL)         type = MIDI_EVENT_SYSEX;
	else type = 0; // Unknown type.

	event->type = type;

	if(type == MIDI_EVENT) {
		event->channel = command & 0x0F;  // The channel is the second nibble.
		command = command & 0xF0;         // The command is the first nibble.

		if(command == MIDI_NOTEOFF ||
		   command == MIDI_NOTEON ||
		   command == MIDI_KEYAFTERTOUCH) {
			event->note = *head;
			event->velocity = *(head+1);

			patch = &patches[chan_patch[event->channel]];
			if(event->note < patch->min || !patch->min)
				patch->min = event->note;

			if(event->note > patch->max)
				patch->max = event->note;

			command_length = 2;
		} else if(command == MIDI_PATCHCHANGE) {
			event->patch = *head;
			patches[event->patch].used = 1;
			chan_patch[event->channel] = event->patch;
			command_length = 1;
		} else if(command == MIDI_CHANNELAFTERTOUCH) {
			command_length = 1;
		} else {
			// Comand has two bytes of data.
			command_length = 2;
		}
	} else if(type == MIDI_EVENT_META) {
		event->meta_type = *(head++);

		quantity_length = get_vl_quantity(&command_length, head);
		if(quantity_length < 0) {
			fprintf(stderr, "Bad command length.\n");
			return -1;
		}
		head += quantity_length;

		if(event->meta_type >= MIDI_META_TEXT &&     // Text event
		   event->meta_type <= MIDI_META_CUEPOINT) { //
			event->data_length = command_length;
			event->data = calloc(command_length + 1, sizeof(uint8_t));

			memcpy(event->data, head, command_length);
			event->data[command_length] = 0;
		} else if(event->meta_type == MIDI_META_SETTEMPO) {
			event->tempo = 0 | *head << 16 | *(head+1) << 8 | *(head+2);
		} else if(event->meta_type == MIDI_META_TIMESIGNATURE) {
			event->time_signature.numerator = *head;
			event->time_signature.denominator = 0x01 << *(head+1);  // 2^(*(head+1))
			event->time_signature.ticks_per_click = *(head+2);
			event->time_signature.n32_per_click = *(head+3);
		}
	} else if(type == MIDI_EVENT_SYSEX) {
		quantity_length = get_vl_quantity(&command_length, head);
		if(quantity_length < 0) {
			fprintf(stderr, "Bad command length.\n");
			return -1;
		}
		head += quantity_length;

		// Read command data.
	}

	event->command = command;
	head += command_length;

	// The difference between where we are and where we started is the
	// length of the event.
	return head - data;
}

// get_vl_quantity
// Get variable length quantity.
//
// Reads the bytes pointed at by head as long as the first bit is set
// until it reaches a byte with the first bit cleared (with a maximum of
// 4 bytes), as defined in the MIDI standard.  The last seven bits from
// each byte are packed together into q.
//
// Takes:  q    - Place to store quantity.
//         head - Pointer to first byte to read.
//
// Returns:  Number of bytes read.
//           Returns negative value on error.
int
get_vl_quantity(uint32_t *q, const uint8_t *head)
{
	*q = 0;

	int i;
	for(i=0; i<4; i++) {
		*q = (*q << 7) | (*(head + i) & 0x7F);

		if(*(head + i) < 0x80) return i+1; // Return number of bytes used to build q.
	}

	return -1;
}

void
destroy_midi(Midi *midi)
{
	int i;
	for(i=0; i < midi->num_tracks; i++) {
		if(midi->tracks[i])
			destroy_midi_track(midi->tracks[i]);
	}

	free(midi->tracks);
}

void
destroy_midi_track(MidiTrack *track)
{
	int i;
	for(i=0; i < track->num_events; i++) {
		destroy_midi_event(track->events[i]);
	}

	free(track->events);
}

void
destroy_midi_event(MidiEvent *event)
{
	free(event->data);
	free(event);
}

void
print_midi_event(FILE *outfile, const MidiEvent *event)
{
	if(event->type == MIDI_EVENT) {
		fprintf(outfile, "Midi event:\t%d,\t%s,\t%d", event->delta_time, get_midi_event_command_string(event->command), event->channel);

		if(event->command == MIDI_NOTEOFF ||
		   event->command == MIDI_NOTEON ||
		   event->command == MIDI_KEYAFTERTOUCH) {
			fprintf(outfile, ",\tNote:  %s (%d)", midi_note_string(event->note), event->velocity);
		} else if(event->command == MIDI_PATCHCHANGE) {
			fprintf(outfile, ",\tPatch:  %d", event->patch);
		}

		fprintf(outfile, "\n");
	} else if (event->type == MIDI_EVENT_META) {
		fprintf(outfile, "Meta event:\t%d,\t%s", event->delta_time, get_midi_meta_command_string(event->meta_type));

		// Text event.
		if(event->meta_type >= MIDI_META_TEXT &&
		   event->meta_type <= MIDI_META_CUEPOINT) {
			fprintf(outfile, ",\t%s", event->data);
		} else if(event->meta_type == MIDI_META_SETTEMPO) {
			fprintf(outfile, ",\t%d microseconds/quarter note", event->tempo);
		} else if(event->meta_type == MIDI_META_TIMESIGNATURE) {
			fprintf(outfile, ",\t%d/%d, %d ticks per beat, %d 32nd notes per beat.",
				event->time_signature.numerator,
				event->time_signature.denominator,
				event->time_signature.ticks_per_click,
				event->time_signature.n32_per_click);
		}

		fprintf(outfile, "\n");
	} else if (event->type == MIDI_EVENT_SYSEX) {
		fprintf(outfile, "Sysex event:\t%d,\t%x\n", event->delta_time, event->command);
	} else {
		fprintf(outfile, "Unknown event:\t%d,\t%d,\t%x\n", event->delta_time, event->type, event->command);
	}
}

