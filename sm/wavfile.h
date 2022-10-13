/*
 * Simple WAV file read and write library.
 * It supports only 16bit PCM WAVE files.
 *   (TODO: specify details: endianness, etc.)
 *
 * Wave format information is from:
 *    https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
 */

#ifndef _WAV_H_
#define _WAV_H_

#include <stdint.h>
#include <stdio.h>


#define WAVE_OK                             0
#define WAVE_ERR_NULL_HANDLER               1
#define WAVE_ERR_READ_MODE_EXPECTED         2
#define WAVE_ERR_WRITE_MODE_EXPECTED        3
#define WAVE_ERR_HEADER_ALREADY_INITIALIZED 4
#define WAVE_ERR_HEADER_NOT_INITIALIZED     5
#define WAVE_ERR_READ                       6
#define WAVE_ERR_WRITE                      7

typedef struct WAVEHeader {
	uint8_t     ChunkID[4];  // "RIFF"
	uint32_t    ChunkSize;   // size, complicated to calculate
	uint8_t     Format[4];   // "WAVE"

	// Subchunk "fmt "
	uint8_t    Subchunk1ID[4]; // "fmt "
	uint32_t   Subchunk1Size;  // PCM -> 16
	uint16_t   AudioFormat;    // PCM -> 1
	uint16_t   NumChannels;    // Mono -> 1, Stereo -> 2, etc.
	uint32_t   SampleRate;     // 8000, 44100, 48000, etc.
	uint32_t   ByteRate;       // SampleRate * NumChannels * BitsPerSample/8
	uint16_t   BlockAlign;     // NumChannels * BitsPerSamle/8
	uint16_t   BitsPerSample;  // 8 bits -> 8, 16 bits -> 16

	// Subchunk "data"
	uint8_t    Subchunk2ID[4]; // "data"
	uint32_t   Subchunk2Size;  // NumSamples * NumChannels * BitsPerSample/8
} WAVEHeader;


/* Data alignement: [sample 1 ch 1] [sample 1 ch 2] ... [sample 2 ch 1] [sample 2 ch 2] ...
   Sample access: sample[samplenum * header.NumChannels + channel] */

typedef struct WAVE {
	WAVEHeader   header;
	int          header_init_done;
	char         mode;
	int          size; // NumSamples
    int          error;
    FILE*        fp;
} WAVE;

WAVE* wopen(const char* fname, const char* mode);
int   wgetheader(WAVE* wave);
int   wread(int16_t* buf, int framenum, WAVE* wave);
int   wsetheader(WAVE* wave);
int   wwrite(int16_t* buf, int framenum, WAVE* wave);
int   frame_size_bytes(WAVE* wave, int frame_num);
WAVE* wclose(WAVE* wave);
int   werror(WAVE* wave);
int   wcopy_header(WAVE* src, WAVE* dest);

#endif /* _WAV_H_ */
