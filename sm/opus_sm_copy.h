/*
 * This header file provides the src/opus_encoder.c
 * private declarations (functions, structs, defines) which are
 * manditory for calling the run_analysis() function in src/analysis.h.
 *
 * It is important to keep it up to date and remove when those
 * declarations are moved to a separate .h file in the opus codec.
 *
 */


#ifndef _OPUS_SM_COPY_H_
#define _OPUS_SM_COPY_H_

#include "src/analysis.h"
#include "silk/API.h"
#include "celt.h"

// These structs/defines/function headers are copied from src/opus_encoder.c

#define MAX_ENCODER_BUFFER 480

typedef struct {
   opus_val32 XX, XY, YY;
   opus_val16 smoothed_width;
   opus_val16 max_follower;
} StereoWidthState;


struct OpusEncoder {
    int          celt_enc_offset;
    int          silk_enc_offset;
    silk_EncControlStruct silk_mode;
    int          application;
    int          channels;
    int          delay_compensation;
    int          force_channels;
    int          signal_type;
    int          user_bandwidth;
    int          max_bandwidth;
    int          user_forced_mode;
    int          voice_ratio;
    opus_int32   Fs;
    int          use_vbr;
    int          vbr_constraint;
    int          variable_duration;
    opus_int32   bitrate_bps;
    opus_int32   user_bitrate_bps;
    int          lsb_depth;
    int          encoder_buffer;
    int          lfe;
    int          arch;
    int          use_dtx;                 /* general DTX for both SILK and CELT */
    int          fec_config;
#ifndef DISABLE_FLOAT_API
    TonalityAnalysisState analysis;
#endif

#define OPUS_ENCODER_RESET_START stream_channels
    int          stream_channels;
    opus_int16   hybrid_stereo_width_Q14;
    opus_int32   variable_HP_smth2_Q15;
    opus_val16   prev_HB_gain;
    opus_val32   hp_mem[4];
    int          mode;
    int          prev_mode;
    int          prev_channels;
    int          prev_framesize;
    int          bandwidth;
    /* Bandwidth determined automatically from the rate (before any other adjustment) */
    int          auto_bandwidth;
    int          silk_bw_switch;
    /* Sampling rate (at the API level) */
    int          first;
    opus_val16 * energy_masking;
    StereoWidthState width_mem;
    opus_val16   delay_buffer[MAX_ENCODER_BUFFER*2];
#ifndef DISABLE_FLOAT_API
    int          detected_bandwidth;
    int          nb_no_activity_ms_Q1;
    opus_val32   peak_signal_energy;
#endif
    int          nonfinal_frame; /* current frame is not the final in a packet */
    opus_uint32  rangeFinal;
};

void downmix_float(const void *_x, opus_val32 *sub, int subframe, int offset, int c1, int c2, int C);

#endif /* _OPUS_SM_COPY_H_ */
