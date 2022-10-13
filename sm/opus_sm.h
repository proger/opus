#ifndef _OPUS_SM_H_
#define _OPUS_SM_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "src/analysis.h"
#include "opus_sm_copy.h"
#include "celt.h"

#define SM_SUPPORTED_SAMPLERATE   48000
//#define SM_FRAME_SIZE             (SM_SUPPORTED_SAMPLERATE/50)
#define SM_FRAME_SIZE             2880

#define SM_OK                              0
#define SM_ERR_NULL_HANDLER                1
#define SM_ERR_UNSUPPORTED_SAMPLERATE      2
#define SM_ERR_OPUS_ENC_CREATE_FAILED      3

typedef struct OpusSM {
	OpusEncoder* opus_enc;
	CELTEncoder* celt_enc;
	AnalysisInfo analysis_info;
	const CELTMode* celt_mode;
	int lsb_depth;
	int delay_compensation;
	int error;
} OpusSM;


int     sm_error(OpusSM* sm);
OpusSM* sm_init(int samplerate, int channels);
OpusSM* sm_destroy(OpusSM* sm);
float   sm_pmusic(OpusSM* sm, float* frame);

#endif /* _OPUS_SM_H_ */