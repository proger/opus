Opus speech vs. music discriminator
===================================

![Example fig](https://github.com/jzombi/opus_sm/sm/screenshot.png "Segmentation, framewise music probabilities, waveform")

Command line speech vs. music discriminator tool based on the built in speech vs. music discriminator of the Opus codec. This tool calculates the framewise music probabilities for a given audio file. Optionally also provides speech-music segmentation. See above example figure showing the segmentation, the framewise music probabilities and the waveform of an [example audio file](https://media.xiph.org/monty/demo/opus-3/speech_music_test.wa)v.

The algorithm which calculates the framewise music probabilities was created by the Opus devs. Details: https://people.xiph.org/~xiphmont/demo/opus/demo3.shtml

Build instructions
------------------

The tool is built when the `custom-modes` flag is enabled (it is enabled by default in this fork). Compilation steps (in the root directory):

    ./autogen.sh
    ./configure
    make

The resulting executable `opus_sm_demo` is statically linked against opus libraries. This way it can be distributed individually.

To disable `custom-modes`, and thus this tool:

    ./configure --enable-custom-modes=no

Usage
-----

The program will print the syntax if it is executed without arguments:

    ./opus_sm_demo
    SM-Test speech music discriminator program

    Usage: ./opus_sm_demo <infile> [outfile pmusic] [outfile labels] [sm min dur] [b min dur]

        infile           path to a 16 bit, 48KHz sample rate PCM WAVE file
        outfile framewise path of the framewise stats output file (default: stdout)
        outfile labels   path of the labels (m|s|b) output file
        sm min dur       speech & music labeled segments' min duration
        b min dur        both labeled segments' min duration

    framewise output format:
    timestamp valid tonality tonality_slope noisiness activity music_prob music_prob_min music_prob_max bandwidth activity_probability max_pitch_ratio

It is important to note, that only 16 bit, 48KHz PCM WAVE files are supported. The WAVE file should not contain any metadata. Using ffmpeg to convert an audio file to the expected format:

    ffmpeg -i input.flac -ar 48000 -y  -fflags +bitexact -flags:v +bitexact -flags:a +bitexact -acodec pcm_s16le -map_metadata -1 output.wav

That's all folks!