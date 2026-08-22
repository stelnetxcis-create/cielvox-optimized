---
license: apache-2.0
language:
  - en
  - zh
  - de
  - fr
  - it
  - es
  - pt
  - ja
  - ko
pipeline_tag: text-to-speech
tags:
  - tts
  - text-to-speech
  - gguf
  - stelnettts
  - cielvox2-tts
  - chatterbox
---


*Note: Documentation under construction. Parameters and file names may change.*

# CielVox 2 Chatterbox v3 T3 GGUF

Quantized TTS model for CielVox 2. Use with the `--backend chatterbox` CLI flag.

## Usage

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DGGML_VULKAN=ON
cmake --build build -j$(nproc)
```

```bash
STELNETTTS_CHATTERBOX_T3_GPU=1 GGML_VULKAN=1 LD_LIBRARY_PATH=build/src:build/ggml/src ./build/bin/stelnettts \
  --backend chatterbox \
  -m chatterbox-v3-t3-q8_0.gguf \
  --codec-model ../chatterbox-v3-s3gen-q8_0.gguf \
  --tts "Hello." \
  --tts-output /tmp/out.wav
```

## Companion model

Requires `chatterbox-v3-s3gen-q8_0.gguf` as `--codec-model`.
