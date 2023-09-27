#ifndef _FILTERS_H_
#define _FILTERS_H_

#include <stdint.h>

struct lpf {
    float tau;
    float y;
};

struct hpf {
    float tau;
    float y;
    float x;
};

void lpf_init(struct lpf *lpf, float tau);
float lpf_filter(struct lpf *lpf, float x, float dt);

void hpf_init(struct hpf *hpf, float tau);
float hpf_filter(struct hpf *hpf, float x, float dt);

#endif /* _FILTERS_H_ */