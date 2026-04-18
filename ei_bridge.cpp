#include "ei_bridge.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

#include <cstdio>
#include <cstring>
#include <cfloat>

static float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
static size_t feature_ix = 0;

static int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}

extern "C" int ei_ping(void) {
    return EI_CLASSIFIER_LABEL_COUNT;
}

extern "C" int ei_add_sample(float ax, float ay, float az) {
    if (feature_ix + 3 > EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
        return -1;
    }

    features[feature_ix++] = ax;
    features[feature_ix++] = ay;
    features[feature_ix++] = az;

    return (feature_ix == EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) ? 1 : 0;
}

extern "C" int ei_predict(ei_result_brief_t *out) {
    if (!out) return -1;
    if (feature_ix != EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) return -2;

    signal_t signal;
    signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
    signal.get_data = &raw_feature_get_data;

    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR rc = run_classifier(&signal, &result, false);
    if (rc != EI_IMPULSE_OK) {
        return (int)rc;
    }

    size_t best_ix = 0;
    float best_val = -FLT_MAX;

    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (result.classification[i].value > best_val) {
            best_val = result.classification[i].value;
            best_ix = i;
        }
    }

    out->label = result.classification[best_ix].label;
    out->score = result.classification[best_ix].value;

    feature_ix = 0;
    return 0;
}