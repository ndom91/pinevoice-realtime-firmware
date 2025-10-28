# How to generate the basic TensorFlow Lite Micro structure

In tflite-micro repo:

```
python3 tensorflow/lite/micro/tools/project_generation/create_tflm_tree.py \
  -e hello_world \
  -e micro_speech \
  -e person_detection \
  FULL_PATH/microwakeword-pv
```

# Notes

-DTF_LITE_STATIC_MEMORY
-DTF_LITE_DISABLE_X86_NEON
