#include <assert.h>
#include <string.h>
#include "opus_sm_label.h"


#define ALLOC_SIZE (16)

Labeler* lb_init(unsigned int sm_thresh, unsigned int b_thresh) {
	Labeler* lb = (Labeler*)malloc(sizeof(Labeler));
	lb->labels = (Label*)malloc(sizeof(Label)*ALLOC_SIZE);
	lb->count = 0;
	lb->alloc_size = ALLOC_SIZE;
	lb->sm_thresh = sm_thresh;
	lb->b_thresh = b_thresh;
	return lb;
}

/* Add label to array */
void lb_add_to_arr(Labeler* lb, Label label) {
	assert(ALLOC_SIZE > 0);
	if (lb->count == lb->alloc_size) {
		lb->alloc_size += ALLOC_SIZE;
		lb->labels = (Label*)realloc(lb->labels, sizeof(Label)*lb->alloc_size);
	}
	lb->labels[lb->count] = label;
	lb->count++;
}

char get_label_type(float pmusic, float pactivity) {
	if (pactivity < 0.5f) {
		return '.';
	} else {
		return (pmusic > 0.5f) ? 'm' : 's';
	}
}

void lb_add_frame(Labeler* lb, float pmusic, float pactivity) {
	Label actual_label;
	actual_label.type = get_label_type(pmusic, pactivity);
	actual_label.frame_count = 1;
	int prev = lb->count - 1;
	int prevprev = lb->count - 2;
	
	if (lb->count == 0) {
		lb_add_to_arr(lb, actual_label);
		return;
	}
	
	if (actual_label.type == lb->labels[prev].type) {
		lb->labels[prev].frame_count++;
		return;
	}
	
	if (lb->labels[prev].frame_count < lb->sm_thresh) {
		if (prevprev >= 0) {
			if (lb->labels[prevprev].type == 'b') {
				lb->labels[prevprev].frame_count += lb->labels[prev].frame_count;
				lb->labels[prev] = actual_label;
				return;
			}
		}
		lb->labels[prev].type = 'b';
	}
	
	lb_add_to_arr(lb, actual_label);
	return;
}

void lb_remove(Labeler* lb, unsigned int index) {
	assert(lb->count > index);
	
	memmove(&lb->labels[index], &lb->labels[index+1], (lb->count-index-1)*sizeof(Label));
	lb->count--;
}

unsigned int lb_remove_merge_b(Labeler* lb, unsigned int index) {
	if (lb->count == 1) {
	    return index + 1;
	}

	if (index == 0) {
		lb->labels[index+1].frame_count += lb->labels[index].frame_count;
		lb_remove(lb, index);
		return index;
	}
	
	lb->labels[index-1].frame_count += lb->labels[index].frame_count;
	lb_remove(lb, index);
	if ((lb->count > index) &&
	   (lb->labels[index-1].type == lb->labels[index].type)) {
		lb->labels[index-1].frame_count += lb->labels[index].frame_count;
		lb_remove(lb, index);
	}
	return index;
}

void lb_remove_short_b(Labeler* lb) {
	for (unsigned int ii = 0; ii < lb->count; ii++) {
		if ((lb->labels[ii].type == 'b') &&
		   (lb->labels[ii].frame_count < lb->b_thresh)) {
			// intended possible overflow, ii++ will recover it
			ii = lb_remove_merge_b(lb, ii) - 1;
		}
	}
}

void lb_finalize(Labeler* lb) {
	lb_remove_short_b(lb);
}

void lb_print_to_file(Labeler* lb, FILE* ofile, double frame_duration) {
	if (ofile == NULL) {
	    return;
	}

	double start = 0;
	for (unsigned int ii = 0; ii < lb->count; ii++) {
		fprintf(ofile, "%f %f %c\n",
		        (double)start,
		        (double)start + frame_duration * lb->labels[ii].frame_count,
		        lb->labels[ii].type);
		start = start + frame_duration * lb->labels[ii].frame_count;
	}
}

Labeler* lb_destroy(Labeler* lb) {
	if (lb == NULL) {
		return lb;
	}
	if (lb->labels != NULL) {
		free(lb->labels);
	}

	free(lb);
	return lb;
}
