/*
 * Source of SM-Test tool
 *
 * This tool reads a wave file
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "src/analysis.h"
#include "opus_sm_copy.h"
#include "celt.h"
#include "opus_sm.h"
#include "wavfile.h"
#include "opus_sm_label.h"

#define OPUS_SUPPORTED_FS     SM_SUPPORTED_SAMPLERATE
#define ANALYSIS_FRAME_SIZE   SM_FRAME_SIZE

void print_syntax(const char* argv0) {
		printf("SM-Test speech music discriminator program\n");
		printf("\n");
		printf("Usage: %s <infile> [outfile pmusic] [outfile labels] [sm min dur] [b min dur]\n", argv0);
		printf("\n");
		printf("    infile           path to a 16 bit, 48KHz sample rate PCM WAVE file\n");
		printf("    outfile framewise path of the framewise stats output file (default: stdout)\n");
		printf("    outfile labels   path of the labels (m|s|b) output file\n");
		printf("    sm min dur       speech & music labeled segments' min duration\n");
		printf("    b min dur        both labeled segments' min duration\n");
		printf("\n");
		printf("framewise output format:\n");
		printf("timestamp valid tonality tonality_slope noisiness activity music_prob music_prob_min music_prob_max bandwidth activity_probability max_pitch_ratio\n");
}


void int2float(int16_t* src, float* dest, int numsamples, int numchannels) {
	for (int ii = 0; ii < numsamples*numchannels; ++ii) {
		dest[ii] = (float)src[ii] / (float) INT16_MAX;
	}
}


/* Open wav file, check sample rate. Return WAVE* on success, NULL on error, and print the error messages to stderr. */
WAVE* open_wav(const char* infile, int verbose) {
	WAVE* wave = wopen(infile, "r");
	if (wave == NULL) {
		fprintf(stderr, "Error while opening wave file \"%s\".\n", infile);
		return NULL;
	}

	wgetheader(wave);
	if (werror(wave) != WAVE_OK) {
		fprintf(stderr, "Error while reading wave header from file \"%s\", error code: %d.\n", infile, werror(wave));
		return NULL;
	}

	if (verbose == 1) {
		printf("File info ============\n");
		printf("Wave file:   \"%s\"\n", infile);
		printf("Sample rate: %d Hz\n", wave->header.SampleRate);
		printf("Length:      %f s\n",
		((float)wave->header.Subchunk2Size / (wave->header.NumChannels * wave->header.BitsPerSample/8)) / (float)wave->header.SampleRate);
		printf("Sample bits: %d bits\n", wave->header.BitsPerSample);
		printf("Channels:    %d\n", wave->header.NumChannels);
	}

	if (wave->header.SampleRate != OPUS_SUPPORTED_FS) {
		fprintf(stderr, "Sample rate %d Hz is not supported. Only files with sample rate %d Hz are accepted.\n",
		        wave->header.SampleRate, OPUS_SUPPORTED_FS);
		wave = wclose(wave);
		return NULL;
	}

	return wave;
}


/* Init Opus encoder, return it on success. Print error to stdout, release wave resource and return NULL on error. */
OpusSM* init_opus(WAVE* wave) {
	OpusSM* sm = sm_init(wave->header.SampleRate, wave->header.NumChannels);

	if (sm_error(sm) != SM_OK) {
		fprintf(stderr, "Opus encoder returned error on opus_encoder_create(). Error code: %d\n", sm_error(sm));
		sm = sm_destroy(sm);
		return NULL;
	}

	return sm;
}


/* Open file for writing. Return FILE* on success. Print error to stdout, return NULL on error. */
FILE* open_output_file(const char* fname) {
	if (fname == NULL) {
	    return stdout;
	}
	FILE* o_file = fopen(fname, "w");
	if (o_file == NULL) {
		fprintf(stderr, "Error while opening output file(s).\n");
		return NULL;
	}
	return o_file;
}


/* Process wave file, write music probability and labels into the specified files. Return 0 on success, print error and return non-zero on error. */
int process(const char* infile,
            WAVE* wave,
            OpusSM* sm,
            FILE* ofp_pmusic,
            FILE* ofp_labels,
            double sm_segment_min_dur,
            double b_segment_min_dur
           )
{
	double frame_dur = (double)ANALYSIS_FRAME_SIZE/wave->header.SampleRate;
	Labeler* lb = lb_init(sm_segment_min_dur/frame_dur, b_segment_min_dur/frame_dur);

	float*   analysis_pcm = malloc(ANALYSIS_FRAME_SIZE*wave->header.NumChannels*sizeof(float));
	int16_t* buffer       = malloc(ANALYSIS_FRAME_SIZE*wave->header.NumChannels*sizeof(int16_t));
	double total_music_ratio = 0;
	int error = 0;
	for (int ii = 0; ii <= wave->size - ANALYSIS_FRAME_SIZE; ii = ii + ANALYSIS_FRAME_SIZE) {
		int readcount = wread(buffer, ANALYSIS_FRAME_SIZE, wave);
		
		error = (readcount != ANALYSIS_FRAME_SIZE);
		
		if (error) {
			fprintf(stderr, "Could not read from wave file \"%s\", read count %d.\n", infile, readcount);
			break;
		}

		int2float(buffer, analysis_pcm, ANALYSIS_FRAME_SIZE, wave->header.NumChannels);
		float pmusic = sm_pmusic(sm, analysis_pcm);
		total_music_ratio += pmusic;

		if (ofp_labels != NULL) {
			lb_add_frame(lb, sm->analysis_info.music_prob, sm->analysis_info.activity_probability);
		}

		fprintf(ofp_pmusic, "%.2f %d %.2f %.2f %.2f %.2f %.2f %.2f %.2f %d %.2f %.2f\n",
			(double)ii / wave->header.SampleRate,
			sm->analysis_info.valid,
			sm->analysis_info.tonality,
			sm->analysis_info.tonality_slope,
			sm->analysis_info.noisiness,
			sm->analysis_info.activity,
			sm->analysis_info.music_prob,
			sm->analysis_info.music_prob_min,
			sm->analysis_info.music_prob_max,
			sm->analysis_info.bandwidth,
			sm->analysis_info.activity_probability,
			sm->analysis_info.max_pitch_ratio);

	}

	if (!error) {
		if (ofp_labels != NULL) {
			lb_finalize(lb);
			lb_print_to_file(lb, ofp_labels, frame_dur);
		}
		total_music_ratio = (total_music_ratio * (double)ANALYSIS_FRAME_SIZE) / (double) wave->size;
		fprintf(stderr, "Music ratio: %f\n", total_music_ratio);
	}


	lb = lb_destroy(lb);
	free(analysis_pcm);
	free(buffer);

	return error;
}


/* Main program. */
int main(int argc, char* argv[]) {

	const int verbose = 0;

	if ((argc < 2) || (argc > 6)) {
		print_syntax(argv[0]);
		return 1;
	}

	const char* infile  = argv[1];
	const char* out_pmusic = NULL;
	const char* out_labels = NULL;
	double sm_segment_min_dur = 4.0f;
	double b_segment_min_dur = 4.0f;

	if (argc >= 3) {
		out_pmusic = argv[2];
	}

	if (argc >= 4) {
		out_labels = argv[3];
	}

	if (argc >= 5) {
		sm_segment_min_dur = atof(argv[4]);
	}

	if (argc >= 6) {
		b_segment_min_dur = atof(argv[5]);
	}

	/* Load file */
	
	WAVE* wave = open_wav(infile, verbose);
	
	if (wave == NULL) {
		return 1;
	}

	/* Init Opus encoder */

	OpusSM* sm = init_opus(wave);

	if (sm == NULL) {
		wclose(wave);
		return 1;
	}

	/* Open output files */

	FILE* ofp_pmusic = open_output_file(out_pmusic);
	if (ofp_pmusic == NULL) {
		wclose(wave);
		sm_destroy(sm);
		return 1;
	}

	FILE* ofp_labels = open_output_file(out_labels);
	if (ofp_labels == NULL) {
		wclose(wave);
		sm_destroy(sm);
		fclose(ofp_pmusic);
		return 1;
	}

	/* Processing */
	
	int error = process(infile,
	                    wave,
	                    sm,
	                    ofp_pmusic,
	                    ofp_labels,
	                    sm_segment_min_dur,
	                    b_segment_min_dur
	                   );

	/* Clean up */

	sm = sm_destroy(sm);
	wave = wclose(wave);
	fclose(ofp_pmusic);
	fclose(ofp_labels);

	return error;
}
