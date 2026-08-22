---
license: apache-2.0
language:
  - ar
  - cs
  - da
  - de
  - el
  - en
  - es
  - fa
  - fi
  - fil
  - fr
  - hi
  - hu
  - id
  - it
  - ja
  - ko
  - mk
  - ms
  - nl
  - pl
  - pt
  - ro
  - ru
  - th
  - tr
  - uk
  - vi
  - zh
pipeline_tag: automatic-speech-recognition
tags:
  - asr
  - speech-recognition
  - gguf
  - stelnettts
  - cielvox2-asr
---


*Note: Documentation under construction. Parameters and file names may change.*

# CielVox 2 ASR 0.6B GGUF

Quantized automatic speech recognition model for CielVox 2. Use with `--backend cielvox2-asr`.

## Usage

```bash
LD_LIBRARY_PATH=build/src:build/ggml/src \
./build/bin/stelnettts \
  --backend cielvox2-asr \
  -m cielvox-asr-0.6b-q8_0.gguf \
  -f input.wav \
  --output-file /tmp/transcript \
  -l en \
  -t $(nproc) \
  --i-have-rights
```

Achieves ~8.3x realtime on AMD Radeon 780M.
