#include <stdio.h>
#include "midi.h"
#include "mod.h"

int main(int argc, char **argv)
{
    char* outfile_name = "test.mod";
    if (argc > 2) {
        outfile_name = argv[2];
    }

    FILE* infile;
    FILE* outfile;

    #ifdef _WIN32
    fopen_s(&infile, argv[1], "rb");
    fopen_s(&outfile, outfile_name, "wb");
    #else
    infile = fopen(argv[1], "rb");
    outfile = fopen(outfile_name, "wb");
    #endif


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
