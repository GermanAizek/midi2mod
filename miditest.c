/*
 * miditest.c
 *
 * Heath Caldwell
 * hncaldwell@csupomona.edu
 *
 */

#include "midi.h"

int main(int argc, char **argv)
{
	Midi midi;
	FILE *infile = fopen(argv[1], "rb");
	int i, j;

	int read_status = read_midi_from_file(&midi, infile);
	fclose(infile);

	printf("Header:  Format %d, %d tracks, division = %d.\n", midi.format, midi.num_tracks, midi.division);
	for(i=0; i < midi.num_tracks; i++) {
		if(midi.tracks[i]) {
			printf("\nTrack:  %d events.\n", midi.tracks[i]->num_events);

			for(j=0; j < midi.tracks[i]->num_events; j++) {
				MidiEvent *event = midi.tracks[i]->events[j];

				print_midi_event(stdout, event);
			}
		}
	}

	destroy_midi(&midi);

	return 0;
}

