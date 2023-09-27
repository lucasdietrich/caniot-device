#include "filters.h"

void lpf_init(struct lpf *lpf, float tau)
{
    lpf->tau = tau;
    lpf->y   = 0.0f;
}

float lpf_filter(struct lpf *lpf, float x, float dt)
{
    const float alpha = dt / (lpf->tau + dt);

    lpf->y = alpha * x + (1.0f - alpha) * lpf->y;

    return lpf->y;
}

void hpf_init(struct hpf *hpf, float tau)
{
    hpf->tau = tau;
    hpf->y   = 0.0f;
    hpf->x   = 0.0f;
}

float hpf_filter(struct hpf *hpf, float x, float dt)
{
    const float alpha = hpf->tau / (hpf->tau + dt);

    hpf->y = alpha * hpf->y + alpha * (x - hpf->x);
    hpf->x = x;

    return hpf->y;
}
