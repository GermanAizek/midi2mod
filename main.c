#include <stdio.h>
#include "midi.h"
#include "mod.h"

int main(int argc, char **argv)
{
    FILE* infile = fopen(argv[1], "rb");
    FILE* outfile = fopen("test.mod", "wb");

    Midi midi;
    if (read_midi_from_file(&midi, infile)) {
        fclose(infile);
        return 1;
    }
    fclose(infile);


    Mod mod;
    int i;

    midi_to_mod(&mod, &midi);

    for (i=0; i<128; i++) {
        if (midi.patches[i].used) {
            printf("%d:\t%s\tmin: %d, max: %d\n", i, MIDI_PATCH_NAMES[i], midi.patches[i].min, midi.patches[i].max);
        }
    }

    write_mod_file(&mod, outfile);

    destroy_midi(&midi);

    fclose(outfile);

    return 0;
}
