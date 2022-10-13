#ifndef _OPUS_SM_LABEL_H_
#define _OPUS_SM_LABEL_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>

typedef struct Label {
	char type;
	unsigned int frame_count;
} Label;

typedef struct Labeler {
	Label* labels;
	unsigned int count;
	unsigned int alloc_size;
	unsigned int sm_thresh;
	unsigned int b_thresh;
} Labeler;

Labeler* lb_init(unsigned int sm_thresh, unsigned int b_thresh);
void     lb_add_frame(Labeler* lb, float pmusic);
void     lb_finalize(Labeler* lb);
void     lb_print_to_file(Labeler* lb, FILE* ofile, double frame_duration);
Labeler* lb_destroy(Labeler* lb);

#endif /* _OPUS_SM_LABEL_H_ */