// Copyright 2025 Marek Kraus (@gamelaster / @gamiee)
// SPDX-License-Identifier: Apache-2.0
// This code was inspired/based on ESPHome's micro_wake_word code, licensed as GPLv3

#include "mww_streaming_model.h"

#include <tensorflow/lite/core/c/common.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include "preprocessor_settings.h"
#include "mww.h"

#define STREAMING_MODEL_VARIABLE_ARENA_SIZE 2048
#define MIN_SLICES_BEFORE_DETECTION 100

class StreamingModel {
public:
  bool register_streaming_ops();
  int32_t load(uint8_t* model_start);
  int32_t perform_streaming_inference(const int8_t features[PREPROCESSOR_FEATURE_SIZE]);
  bool get_unprocessed_probability_status() const { return this->unprocessed_probability_status; }
  void determine_detected(mww_detection_info_t* info);

  // Configuration
  uint8_t probability_cutoff;
  size_t sliding_window_size;
  uint8_t recent_streaming_probabilities[20];
  size_t tensor_arena_size;
  uint8_t current_stride_step;
  int16_t ignore_windows{-MIN_SLICES_BEFORE_DETECTION};
  size_t last_n_index{0};
  bool unprocessed_probability_status{false};


  uint8_t* tensor_arena{nullptr}; // TODO: Free & Static
  uint8_t var_arena[STREAMING_MODEL_VARIABLE_ARENA_SIZE];

  tflite::MicroMutableOpResolver<20> op_resolver;
  tflite::MicroInterpreter* interpreter; // TODO: Free & Static
  tflite::MicroResourceVariables *mrv_{nullptr}; // TODO: Free & Static
  tflite::MicroAllocator *ma_{nullptr}; // TODO: Free & Static
  void reset_probabilities();
};

bool StreamingModel::register_streaming_ops()
{
  if (op_resolver.AddCallOnce() != kTfLiteOk)
    return false;
  if (op_resolver.AddVarHandle() != kTfLiteOk)
    return false;
  if (op_resolver.AddReshape() != kTfLiteOk)
    return false;
  if (op_resolver.AddReadVariable() != kTfLiteOk)
    return false;
  if (op_resolver.AddStridedSlice() != kTfLiteOk)
    return false;
  if (op_resolver.AddConcatenation() != kTfLiteOk)
    return false;
  if (op_resolver.AddAssignVariable() != kTfLiteOk)
    return false;
  if (op_resolver.AddConv2D() != kTfLiteOk)
    return false;
  if (op_resolver.AddMul() != kTfLiteOk)
    return false;
  if (op_resolver.AddAdd() != kTfLiteOk)
    return false;
  if (op_resolver.AddMean() != kTfLiteOk)
    return false;
  if (op_resolver.AddFullyConnected() != kTfLiteOk)
    return false;
  if (op_resolver.AddLogistic() != kTfLiteOk)
    return false;
  if (op_resolver.AddQuantize() != kTfLiteOk)
    return false;
  if (op_resolver.AddDepthwiseConv2D() != kTfLiteOk)
    return false;
  if (op_resolver.AddAveragePool2D() != kTfLiteOk)
    return false;
  if (op_resolver.AddMaxPool2D() != kTfLiteOk)
    return false;
  if (op_resolver.AddPad() != kTfLiteOk)
    return false;
  if (op_resolver.AddPack() != kTfLiteOk)
    return false;
  if (op_resolver.AddSplitV() != kTfLiteOk)
    return false;

  return true;
}

int32_t StreamingModel::load(uint8_t* model_start)
{
  this->tensor_arena = static_cast<uint8_t*>(malloc(this->tensor_arena_size));
  if (this->tensor_arena == NULL) {
    return -1;
  }

  this->ma_ = tflite::MicroAllocator::Create(this->var_arena, STREAMING_MODEL_VARIABLE_ARENA_SIZE);
  this->mrv_ = tflite::MicroResourceVariables::Create(this->ma_, 20);

  const tflite::Model *model = tflite::GetModel(model_start);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    return -2;
  }

  this->interpreter =
    new tflite::MicroInterpreter(tflite::GetModel(model_start), this->op_resolver,
                                 this->tensor_arena, this->tensor_arena_size, this->mrv_);
  if (this->interpreter->AllocateTensors() != kTfLiteOk) {
    return -3;
  }

  // Verify input tensor matches expected values
  // Dimension 3 will represent the first layer stride, so skip it may vary
  TfLiteTensor *input = this->interpreter->input(0);
  if ((input->dims->size != 3) || (input->dims->data[0] != 1) ||
      (input->dims->data[2] != PREPROCESSOR_FEATURE_SIZE)) {
    // Streaming model tensor input dimensions has improper dimensions.
    return -4;
  }

  if (input->type != kTfLiteInt8) {
    // Streaming model tensor input is not int8.
    return -5;
  }

  // Verify output tensor matches expected values
  TfLiteTensor *output = this->interpreter->output(0);
  if ((output->dims->size != 2) || (output->dims->data[0] != 1) || (output->dims->data[1] != 1)) {
    // Streaming model tensor output dimension is not 1x1.
    return -6;
  }

  if (output->type != kTfLiteUInt8) {
    // Streaming model tensor output is not uint8.
    return -7;
  }

  this->reset_probabilities();

  return 0;
}

void StreamingModel::reset_probabilities()
{
  for (size_t i = 0; i < this->sliding_window_size; i++) {
    this->recent_streaming_probabilities[i] = 0;
  }
  this->ignore_windows = -MIN_SLICES_BEFORE_DETECTION;
}

int32_t StreamingModel::perform_streaming_inference(const int8_t* features)
{
  TfLiteTensor *input = this->interpreter->input(0);

  uint8_t stride = this->interpreter->input(0)->dims->data[1];
  this->current_stride_step = this->current_stride_step % stride;
  std::memmove(
    (int8_t *) (tflite::GetTensorData<int8_t>(input)) + PREPROCESSOR_FEATURE_SIZE * this->current_stride_step,
    features, PREPROCESSOR_FEATURE_SIZE);
  ++this->current_stride_step;

  if (this->current_stride_step >= stride) {
    TfLiteStatus invoke_status = this->interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
      return -1;
    }
    TfLiteTensor *output = this->interpreter->output(0);
    ++this->last_n_index;
    if (this->last_n_index == this->sliding_window_size)
      this->last_n_index = 0;
    this->recent_streaming_probabilities[this->last_n_index] = output->data.uint8[0];  // probability;
    this->unprocessed_probability_status = true;
  }
  if (this->recent_streaming_probabilities[this->last_n_index] < this->probability_cutoff) {
    // Only increment ignore windows if less than the probability cutoff; this forces the model to "cool-off" from a
    // previous detection and calling ``reset_probabilities`` so it avoids duplicate detections
    this->ignore_windows = std::min(this->ignore_windows + 1, 0);
  }

  return 0;
}

void StreamingModel::determine_detected(mww_detection_info_t* info)
{
  if (this->ignore_windows < 0) {
    info->detected = false;
    return;
  }

  uint32_t sum = 0;
  uint8_t max_probability = 0;
  for (size_t i = 0; i < this->sliding_window_size; i++) {
    uint8_t prob = this->recent_streaming_probabilities[i];
    max_probability = std::max(max_probability, prob);
    sum += prob;
  }


  bool detected = sum > this->probability_cutoff * this->sliding_window_size;
  info->average_probability = sum / this->sliding_window_size;
  info->max_probability = max_probability;
  info->detected = detected;

  this->unprocessed_probability_status = false;

  if (detected) {
    this->reset_probabilities();
  }
}

#if 0
#define MWW_DIAG_ARRAY(TAG, EXPR)        \
  extern int TAG[(int)(EXPR)];           \
  extern int TAG[(int)(EXPR) + 1]

MWW_DIAG_ARRAY(mww_sz,   sizeof(StreamingModel));
MWW_DIAG_ARRAY(mww_align,__alignof__(StreamingModel));
#endif

static_assert(sizeof(struct mww_streaming_model_t)  == sizeof(StreamingModel),  "size mismatch");
static_assert(alignof(struct mww_streaming_model_t) == alignof(StreamingModel), "align mismatch");

extern "C" int32_t
mww_streaming_model_init(struct mww_streaming_model_t* inst, uint8_t* model_start, uint8_t default_probability_cutoff,
                         size_t sliding_window_average_size, size_t tensor_arena_size)
{
  StreamingModel* model = reinterpret_cast<StreamingModel*>(inst);
  new (model) StreamingModel();

  model->probability_cutoff = default_probability_cutoff;
  model->sliding_window_size = sliding_window_average_size;
  // TODO: Check for sliding_window_average_size maximum value
  model->tensor_arena_size = tensor_arena_size;
  model->current_stride_step = 0;


  if (!model->register_streaming_ops()) {
    return -1; // Failed to register streaming operations
  }

  int32_t res = model->load(model_start);
  if (res != 0) {
    return -2;
  }

  return 0;
}

extern "C" int32_t mww_streaming_model_perform_inference(struct mww_streaming_model_t* inst, const int8_t features[PREPROCESSOR_FEATURE_SIZE])
{
  StreamingModel* model = reinterpret_cast<StreamingModel*>(inst);
  return model->perform_streaming_inference(features);
}

extern "C" bool mww_streaming_model_get_unprocessed_probability_status(struct mww_streaming_model_t* inst)
{
  StreamingModel* model = reinterpret_cast<StreamingModel*>(inst);
  return model->get_unprocessed_probability_status();
}

extern "C" void mww_streaming_model_determine_detected(struct mww_streaming_model_t* inst, struct mww_detection_info_t* info)
{
  StreamingModel* model = reinterpret_cast<StreamingModel*>(inst);
  return model->determine_detected(info);
}

extern "C" int32_t mww_streaming_model_deinit(struct mww_streaming_model_t* inst)
{
  // TODO:
  return 0;
}
