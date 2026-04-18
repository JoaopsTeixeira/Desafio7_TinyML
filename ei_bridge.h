#ifndef EI_BRIDGE_H
#define EI_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *label;
    float score;
} ei_result_brief_t;

int ei_ping(void);
int ei_add_sample(float ax, float ay, float az);
int ei_predict(ei_result_brief_t *out);

#ifdef __cplusplus
}
#endif

#endif