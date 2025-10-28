// Copyright 2025 Marek Kraus (@gamelaster / @gamiee)
// SPDX-License-Identifier: Apache-2.0
// This code was inspired/based on ESPHome's micro_wake_word code, licensed as GPLv3

#ifndef MWW_H_
#define MWW_H_

#include <stdint.h>
#include <frontend_util.h>
#include "mww_streaming_model.h"
#include "preprocessor_settings.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mww_t
{
  struct FrontendConfig frontend_config;
  struct FrontendState frontend_state;
  uint8_t features_step_size;
  struct mww_streaming_model_t* model;
  int8_t features_buffer[PREPROCESSOR_FEATURE_SIZE];
};

int32_t mww_init(struct mww_t* inst);
void mww_set_model(struct mww_t* inst, struct mww_streaming_model_t* model);
int32_t mww_do_inference(struct mww_t* inst, void* data, uint32_t data_size, struct mww_detection_info_t* info);
int32_t mww_deinit(struct mww_t* inst);

#ifdef __cplusplus
}
#endif

#endif