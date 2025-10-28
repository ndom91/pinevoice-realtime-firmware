// Copyright 2025 Marek Kraus (@gamelaster / @gamiee)
// SPDX-License-Identifier: Apache-2.0
// This code was inspired/based on ESPHome's micro_wake_word code, licensed as GPLv3

#include <stdio.h>
#include "mww.h"
#include "preprocessor_settings.h"
#include "mww_streaming_model.h"

#define MIC_SAMPLE_RATE 16000

int32_t mww_init(struct mww_t* inst)
{
  inst->features_step_size = 10; // TODO: Maybe make it configurable?
  inst->frontend_config.window.size_ms = FEATURE_DURATION_MS;
  inst->frontend_config.window.step_size_ms = inst->features_step_size;
  inst->frontend_config.filterbank.num_channels = PREPROCESSOR_FEATURE_SIZE;
  inst->frontend_config.filterbank.lower_band_limit = FILTERBANK_LOWER_BAND_LIMIT;
  inst->frontend_config.filterbank.upper_band_limit = FILTERBANK_UPPER_BAND_LIMIT;
  inst->frontend_config.noise_reduction.smoothing_bits = NOISE_REDUCTION_SMOOTHING_BITS;
  inst->frontend_config.noise_reduction.even_smoothing = NOISE_REDUCTION_EVEN_SMOOTHING;
  inst->frontend_config.noise_reduction.odd_smoothing = NOISE_REDUCTION_ODD_SMOOTHING;
  inst->frontend_config.noise_reduction.min_signal_remaining = NOISE_REDUCTION_MIN_SIGNAL_REMAINING;
  inst->frontend_config.pcan_gain_control.enable_pcan = PCAN_GAIN_CONTROL_ENABLE_PCAN;
  inst->frontend_config.pcan_gain_control.strength = PCAN_GAIN_CONTROL_STRENGTH;
  inst->frontend_config.pcan_gain_control.offset = PCAN_GAIN_CONTROL_OFFSET;
  inst->frontend_config.pcan_gain_control.gain_bits = PCAN_GAIN_CONTROL_GAIN_BITS;
  inst->frontend_config.log_scale.enable_log = LOG_SCALE_ENABLE_LOG;
  inst->frontend_config.log_scale.scale_shift = LOG_SCALE_SCALE_SHIFT;

  // TODO: Make Frontend malloc free :)
  if (!FrontendPopulateState(&inst->frontend_config, &inst->frontend_state, MIC_SAMPLE_RATE)) {
    return -1;
  }

  return 0;
}

static uint32_t generate_features(struct mww_t* inst, int16_t *audio_buffer, size_t samples_available)
{
  size_t processed_samples = 0;
  struct FrontendOutput frontend_output =
    FrontendProcessSamples(&inst->frontend_state, audio_buffer, samples_available, &processed_samples);
  for (size_t i = 0; i < frontend_output.size; ++i) {
    // These scaling values are set to match the TFLite audio frontend int8 output.
    // The feature pipeline outputs 16-bit signed integers in roughly a 0 to 670
    // range. In training, these are then arbitrarily divided by 25.6 to get
    // float values in the rough range of 0.0 to 26.0. This scaling is performed
    // for historical reasons, to match up with the output of other feature
    // generators.
    // The process is then further complicated when we quantize the model. This
    // means we have to scale the 0.0 to 26.0 real values to the -128 (INT8_MIN)
    // to 127 (INT8_MAX) signed integer numbers.
    // All this means that to get matching values from our integer feature
    // output into the tensor input, we have to perform:
    // input = (((feature / 25.6) / 26.0) * 256) - 128
    // To simplify this and perform it in 32-bit integer math, we rearrange to:
    // input = (feature * 256) / (25.6 * 26.0) - 128
    static const int32_t value_scale = 256;
    static const int32_t value_div = 666;  // 666 = 25.6 * 26.0 after rounding
    int32_t value = ((frontend_output.values[i] * value_scale) + (value_div / 2)) / value_div;

    value += INT8_MIN;  // Adds a -128; i.e., subtracts 128

#define CLAMP(a, x, b) (((x) < (a)) ? (a) : \
			((b) < (x)) ? (b) : (x))

    inst->features_buffer[i] = CLAMP(value, INT8_MIN, INT8_MAX);
  }

  return processed_samples;
}

static void process_probabilities(struct mww_t* inst, struct mww_detection_info_t* info)
{
  bool status = mww_streaming_model_get_unprocessed_probability_status(inst->model);
  if (status) {
    mww_streaming_model_determine_detected(inst->model, info);
    //if (info.detected) {
    //  printf("Wake word found!!!\n");
    //    static const float uint8_to_float_divisor =
    //      255.0f;  // Converting a quantized uint8 probability to floating point
    //    printf("Detected Hey Jarvis with sliding average probability is %.2f and max probability is %.2f\n",
    //           (info.average_probability / uint8_to_float_divisor),
    //           (info.max_probability / uint8_to_float_divisor));
    //}

    // Only detect wake words if there is a new probability since the last check
    //DetectionEvent wake_word_state = model->determine_detected();
    //if (wake_word_state.detected) {
    //  constexpr float uint8_to_float_divisor =
    //    255.0f;  // Converting a quantized uint8 probability to floating point
    //  printf("Detected Hey Jarvis with sliding average probability is %.2f and max probability is %.2f\n",
    //         (wake_word_state.average_probability / uint8_to_float_divisor),
    //         (wake_word_state.max_probability / uint8_to_float_divisor));
    //  model->reset_probabilities();
    //}
  }
}

int32_t mww_do_inference(struct mww_t* inst, void* data, uint32_t data_size, struct mww_detection_info_t* info)
{
  int32_t res = 0;
  size_t new_samples_to_get = inst->features_step_size * (MIC_SAMPLE_RATE / 1000);
  info->detected = false;

  if (data_size < new_samples_to_get) {
    return -1; // Not enough data, try again
  }
  uint32_t processed_samples = generate_features(inst, (int16_t *)data, data_size / sizeof(int16_t));

  res = mww_streaming_model_perform_inference(inst->model, inst->features_buffer);
  if (res != 0) {
    return -2;
  }

  process_probabilities(inst, info);
  return processed_samples * 2;
}

void mww_set_model(struct mww_t* inst, struct mww_streaming_model_t* model)
{
  inst->model = model;
}

int32_t mww_deinit(struct mww_t* inst)
{
  FrontendFreeStateContents(&inst->frontend_state);
  return 0;
}