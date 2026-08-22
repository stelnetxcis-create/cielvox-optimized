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

# CielVox 2 Chatterbox v3 S3Gen GGUF

Codec/companion model for CielVox 2 Chatterbox v3. Pass this as `--codec-model` alongside the T3 model.

## Usage

```bash
STELNETTTS_CHATTERBOX_S3GEN_CPU=0 GGML_VULKAN=1 LD_LIBRARY_PATH=build/src:build/ggml/src \
./build/bin/stelnettts \
  --backend chatterbox \
  -m ../chatterbox-v3-t3-q8_0.gguf \
  --codec-model chatterbox-v3-s3gen-q8_0.gguf \
  --tts "Hello." \
  --tts-output /tmp/out.wav
```

## T3 model

Requires `chatterbox-v3-t3-q8_0.gguf` as the main `-m` model.
