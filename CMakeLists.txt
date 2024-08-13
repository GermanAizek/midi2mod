cmake_minimum_required(VERSION 3.5)

project(midi2mod LANGUAGES C)

set(MIDI_TEST OFF)

if(MIDI_TEST)
    add_executable(midi2mod miditest.c midi.h midi.c stringcatalog.h stringcatalog.c mod.h mod.c)
else()
    add_executable(midi2mod main.c midi.h midi.c stringcatalog.h stringcatalog.c mod.h mod.c)
endif()

if(UNIX)
    set(CMAKE_C_FLAGS "-lm")
endif()

if(WIN32)
    target_link_libraries(midi2mod wsock32 ws2_32)
endif()
