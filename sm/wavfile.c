#include "wavfile.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int is_equal_char_arr(const char* arr, const char* reference, const uint32_t length) {
	uint32_t ii;
	for (ii = 0; ii < length; ++ii) {
		if (arr[ii] != reference[ii]) return 0;
	}
	return 1;
}

int is_valid_header(const WAVEHeader header) {
	int result = 1;
	result = result & is_equal_char_arr((char*)&header.ChunkID,     "RIFF", 4);
	result = result & is_equal_char_arr((char*)&header.Format,      "WAVE", 4);
	result = result & is_equal_char_arr((char*)&header.Subchunk1ID, "fmt ", 4);
	result = result & is_equal_char_arr((char*)&header.Subchunk2ID, "data", 4);
	result = result & ((header.BitsPerSample % 8) == 0);

	/* Only 16 bits samples are supported at the moment */
	result = result & ((header.BitsPerSample == 16));

	return result;
}

int wcopy_header(WAVE* src, WAVE* dest) {
	if (src->header_init_done == 0) {
		src->error = WAVE_ERR_HEADER_NOT_INITIALIZED;
		return src->error;
	}
	if ((src == NULL) || (dest == NULL)) return WAVE_ERR_NULL_HANDLER;


	memcpy(&dest->header, &src->header, sizeof(WAVEHeader));
	return WAVE_OK;
}

int frame_size_bytes(WAVE* wave, int frame_num) {
	if (wave == NULL) {
		return 0;
	}
	if (wave->header_init_done == 0) {
		wave->error = WAVE_ERR_HEADER_NOT_INITIALIZED;
		return 0;
	}
	return frame_num*wave->header.NumChannels*(wave->header.BitsPerSample/8);
}

WAVE* wopen(const char* fname, const char* mode) {
	WAVE* wave = (WAVE*)malloc(sizeof(WAVE));
	wave->fp = NULL;
	char open_mode[3] = "?b";
	open_mode[0] = mode[0];
	wave->fp = fopen(fname, open_mode);
	if (wave->fp != NULL) {
		wave->mode = mode[0];
		wave->header_init_done = 0;
		wave->size = 0;
		wave->error = WAVE_OK;
	} else {
		free(wave);
		wave = NULL;
	}
	return wave;
}

WAVE* wclose(WAVE* wave) {
	if (wave != NULL) {
		// Rewrite header in write mode, to store size
		if (wave->mode == 'w') {
			wave->header.Subchunk2Size = wave->size * wave->header.NumChannels * (wave->header.BitsPerSample/8); // NumSamples * NumChannels * BitsPerSample/8
			fseek(wave->fp, 0 , SEEK_SET);
			wave->header_init_done = 0;
			wsetheader(wave);
		}
		fclose(wave->fp);
		free(wave);
		wave = NULL;
	}
	return wave;
}

int wgetheader(WAVE* wave) {
	if (wave == NULL) return WAVE_ERR_NULL_HANDLER;
	if (wave->error == WAVE_ERR_HEADER_NOT_INITIALIZED) wave->error = WAVE_OK;
	if (wave->mode != 'r') wave->error = WAVE_ERR_READ_MODE_EXPECTED;
	if (wave->header_init_done != 0) wave->error = WAVE_ERR_HEADER_ALREADY_INITIALIZED;
	if (wave->error != WAVE_OK) return wave->error;
	int success = 1;
	success = success & fread(&wave->header.ChunkID,       4*sizeof(uint8_t),  1, wave->fp);
	success = success & fread(&wave->header.ChunkSize,      sizeof(uint32_t),  1, wave->fp);
	success = success & fread(&wave->header.Format,        4*sizeof(uint8_t),  1, wave->fp);

	// Subchunk "fmt "
	success = success & fread(&wave->header.Subchunk1ID,   4*sizeof(uint8_t),  1, wave->fp);
	success = success & fread(&wave->header.Subchunk1Size,  sizeof(uint32_t),  1, wave->fp);
	success = success & fread(&wave->header.AudioFormat,    sizeof(uint16_t),  1, wave->fp);
	success = success & fread(&wave->header.NumChannels,    sizeof(uint16_t),  1, wave->fp);
	success = success & fread(&wave->header.SampleRate,     sizeof(uint32_t),  1, wave->fp);
	success = success & fread(&wave->header.ByteRate,       sizeof(uint32_t),  1, wave->fp);
	success = success & fread(&wave->header.BlockAlign,     sizeof(uint16_t),  1, wave->fp);
	success = success & fread(&wave->header.BitsPerSample,  sizeof(uint16_t),  1, wave->fp);

	// Subchunk "data"
	success = success & fread(&wave->header.Subchunk2ID,   4*sizeof(uint8_t),  1, wave->fp);
	success = success & fread(&wave->header.Subchunk2Size,  sizeof(uint32_t),  1, wave->fp);

	if ((success == 1) && (is_valid_header(wave->header))) {
		wave->size = wave->header.Subchunk2Size / ((wave->header.BitsPerSample / 8) * wave->header.NumChannels);
		wave->header_init_done = 1;
		wave->error = WAVE_OK;
	} else {
		wave->error = WAVE_ERR_READ;
	}
	return wave->error;

}

int wread(int16_t* buf, int framenum, WAVE* wave) {
	if (wave == NULL) return 0;
	if (wave->error == WAVE_ERR_HEADER_ALREADY_INITIALIZED) wave->error = WAVE_OK;
	if (wave->mode != 'r') wave->error = WAVE_ERR_READ_MODE_EXPECTED;
	if (wave->header_init_done != 1) wave->error = WAVE_ERR_HEADER_NOT_INITIALIZED;
	if (wave->error != WAVE_OK) return 0;
	int result = fread(buf, (wave->header.BitsPerSample / 8)*wave->header.NumChannels, framenum, wave->fp);
	return result;
}

int wsetheader(WAVE* wave) {
	if (wave == NULL) return WAVE_ERR_NULL_HANDLER;
	if (wave->error == WAVE_ERR_HEADER_NOT_INITIALIZED) wave->error = WAVE_OK;
	if (wave->mode != 'w') wave->error = WAVE_ERR_WRITE_MODE_EXPECTED;
	if (wave->header_init_done != 0) wave->error = WAVE_ERR_HEADER_ALREADY_INITIALIZED;
	if (wave->error != WAVE_OK) return wave->error;
	int success = 1;
	success = success & fwrite(&wave->header.ChunkID,       4*sizeof(uint8_t),  1, wave->fp);
	success = success & fwrite(&wave->header.ChunkSize,      sizeof(uint32_t),  1, wave->fp);
	success = success & fwrite(&wave->header.Format,        4*sizeof(uint8_t),  1, wave->fp);

	// Subchunk "fmt "
	success = success & fwrite(&wave->header.Subchunk1ID,   4*sizeof(uint8_t),  1, wave->fp);
	success = success & fwrite(&wave->header.Subchunk1Size,  sizeof(uint32_t),  1, wave->fp);
	success = success & fwrite(&wave->header.AudioFormat,    sizeof(uint16_t),  1, wave->fp);
	success = success & fwrite(&wave->header.NumChannels,    sizeof(uint16_t),  1, wave->fp);
	success = success & fwrite(&wave->header.SampleRate,     sizeof(uint32_t),  1, wave->fp);
	success = success & fwrite(&wave->header.ByteRate,       sizeof(uint32_t),  1, wave->fp);
	success = success & fwrite(&wave->header.BlockAlign,     sizeof(uint16_t),  1, wave->fp);
	success = success & fwrite(&wave->header.BitsPerSample,  sizeof(uint16_t),  1, wave->fp);

	// Subchunk "data"
	success = success & fwrite(&wave->header.Subchunk2ID,   4*sizeof(uint8_t),  1, wave->fp);
	success = success & fwrite(&wave->header.Subchunk2Size,  sizeof(uint32_t),  1, wave->fp);

	if (success == 1) {
		wave->header_init_done = 1;
		wave->error = WAVE_OK;
	} else {
		wave->error = WAVE_ERR_WRITE;
	}
	return wave->error;

}

int wwrite(int16_t* buf, int framenum, WAVE* wave) {
	if (wave == NULL) return 0;
	if (wave->error == WAVE_ERR_HEADER_ALREADY_INITIALIZED) wave->error = WAVE_OK;
	if (wave->mode != 'w') wave->error = WAVE_ERR_WRITE_MODE_EXPECTED;
	if (wave->header_init_done != 1) wave->error = WAVE_ERR_HEADER_NOT_INITIALIZED;
	if (wave->error != WAVE_OK) return 0;
	int result = fwrite(buf, (wave->header.BitsPerSample / 8)*wave->header.NumChannels, framenum, wave->fp);
	wave->size += result;
	return result;
}

int werror(WAVE* wave) {
	if (wave == NULL) return WAVE_ERR_NULL_HANDLER;
	return wave->error;
}

