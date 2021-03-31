cmake_minimum_required(VERSION 3.5)

project(midi2mod LANGUAGES C)

set(MIDI_TEST OFF)

if (MIDI_TEST)
    add_executable(midi2mod miditest.c midi.h midi.c stringcatalog.h stringcatalog.c mod.h mod.c)
else()
    add_executable(midi2mod main.c midi.h midi.c stringcatalog.h stringcatalog.c mod.h mod.c)
endif()
