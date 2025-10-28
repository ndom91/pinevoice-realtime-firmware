// Copyright 2025 Marek Kraus (@gamelaster / @gamiee)
// SPDX-License-Identifier: Apache-2.0
// This code was inspired/based on ESPHome's micro_wake_word code, licensed as GPLv3

#ifndef MWW_STREAMING_MODEL_H_
#define MWW_STREAMING_MODEL_H_

#include <stdint.h>
#include <stddef.h>
#include "preprocessor_settings.h"

#ifdef CONFIG_CPU_C906
#define MWW_STREAMING_MODEL_SIZE 3536
#define MWW_STREAMING_MODEL_ALIGN 8
#elif defined(CONFIG_CPU_E907)
#define MWW_STREAMING_MODEL_SIZE 2840
#define MWW_STREAMING_MODEL_ALIGN 4
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct mww_detection_info_t
{
  bool detected;
  uint8_t max_probability;
  uint8_t average_probability;
};

struct mww_streaming_model_t
{
  unsigned char bytes[MWW_STREAMING_MODEL_SIZE];
} __attribute__((aligned(MWW_STREAMING_MODEL_ALIGN)));

int32_t
mww_streaming_model_init(struct mww_streaming_model_t* inst, uint8_t* model_start, uint8_t default_probability_cutoff,
                         size_t sliding_window_average_size, size_t tensor_arena_size);
int32_t mww_streaming_model_perform_inference(struct mww_streaming_model_t* inst, const int8_t features[PREPROCESSOR_FEATURE_SIZE]);
bool mww_streaming_model_get_unprocessed_probability_status(struct mww_streaming_model_t* inst);
void mww_streaming_model_determine_detected(struct mww_streaming_model_t* inst, struct mww_detection_info_t* info);
int32_t mww_streaming_model_deinit(struct mww_streaming_model_t* inst);

#ifdef __cplusplus
}
#endif

#endif