#include "opus_sm.h"
#include <stdlib.h>


int sm_error(OpusSM* sm) {
	if (sm == NULL) return SM_ERR_NULL_HANDLER;
	return sm->error;
}

OpusSM* sm_init(int samplerate, int channels)
{
	OpusSM* sm = (OpusSM*)malloc(sizeof(OpusSM));
	sm->error = SM_OK;
	if (samplerate != SM_SUPPORTED_SAMPLERATE) {
		sm->error = SM_ERR_UNSUPPORTED_SAMPLERATE;
		sm->opus_enc = NULL;
		return sm;
	}
	/* application can be: (see opus_encoder_init() in src/opus_encoder.c)
	     OPUS_APPLICATION_VOIP
	     OPUS_APPLICATION_AUDIO
	     OPUS_APPLICATION_RESTRICTED_LOWDELAY */
	int error;
	sm->opus_enc = opus_encoder_create(samplerate, channels, OPUS_APPLICATION_VOIP, &error);
	if (error != 0) {
		sm->error = SM_ERR_OPUS_ENC_CREATE_FAILED;
		return sm;
	}

	sm->celt_enc = (CELTEncoder*)((char*)sm->opus_enc + sm->opus_enc->celt_enc_offset);
	celt_encoder_ctl(sm->celt_enc, CELT_GET_MODE(&sm->celt_mode));
	sm->lsb_depth = IMIN(16, sm->opus_enc->lsb_depth);
	sm->delay_compensation = 0;
	if (sm->opus_enc->application != OPUS_APPLICATION_RESTRICTED_LOWDELAY) sm->delay_compensation = sm->opus_enc->delay_compensation;
	return sm;
}

OpusSM* sm_destroy(OpusSM* sm) {
	if (sm == NULL) {
		return NULL;
	}


	if (sm->opus_enc != NULL) {
		opus_encoder_destroy(sm->opus_enc);
	}
	free(sm);
	sm = NULL;
	return sm;
}

float sm_pmusic(OpusSM* sm, float* frame) {
	if (sm == NULL) {
		return 0;
	}
	/* Values are from opus_encode_float(), opus_encode_native() functions, see src/opus_encoder.c */
	int c1 = 0;
	int c2 = -2;
	int frame_size = frame_size_select(SM_FRAME_SIZE, sm->opus_enc->variable_duration, sm->opus_enc->Fs);
	run_analysis(&sm->opus_enc->analysis, sm->celt_mode, frame, SM_FRAME_SIZE, frame_size, c1, c2,
	             sm->opus_enc->channels, sm->opus_enc->Fs, sm->lsb_depth, downmix_float,
	             &sm->analysis_info);
	return sm->analysis_info.music_prob;
}
