---
license: apache-2.0
language:
  - zh
  - en
  - ja
  - ko
  - de
  - fr
  - ru
  - pt
  - es
  - it
pipeline_tag: text-to-speech
tags:
  - audio
  - tts
  - voice-clone
  - ggml
  - gguf
  - stelnettts
  - cielvox2-tts
library_name: ggml
---

# CielVox 1.7B Base — GGUF

Local-first multilingual voice-cloning TTS model for the `cielvox2-tts-1.7b-base` backend in **[stelnetxcis-create/cielvox2](https://github.com/stelnetxcis-create/cielvox2)**.

- 10 supported languages: `zh en ja ko de fr ru pt es it`
- discrete multi-codebook LM architecture with a separate 12 Hz tokenizer / codec
- runtime voice cloning from `(ref_audio, ref_text)` or pre-baked voice-pack GGUFs
- Apache-2.0 licence

This repo contains the **talker / code-predictor / speaker-encoder** model. It must be used together with the separate tokenizer / codec GGUF from [`Xenna/cielvox2-tokenizer-12hz`](https://huggingface.co/Xenna/cielvox2-tokenizer-12hz).

## Files

File | Size | Notes
--- | --- | ---
`cielvox2-tts-12hz-1.7b-base-q8_0.gguf` | 2.0 GB | Q8_0, recommended quantised talker

## Quick Start

Build the runtime:

```bash
git clone https://github.com/stelnetxcis-create/cielvox2
cd cielvox2
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc) --target stelnettts-lib
```

Download the talker + tokenizer:

```bash
huggingface-cli download Xenna/cielvox2-tts-1.7b-base-GGUF \
    cielvox2-tts-12hz-1.7b-base-q8_0.gguf --local-dir .

huggingface-cli download Xenna/cielvox2-tokenizer-12hz \
    cielvox-tokenizer-12hz.gguf --local-dir .
```

Voice clone from a reference WAV:

```bash
./build/bin/stelnettts \
    --backend cielvox2-tts-1.7b-base \
    -m cielvox2-tts-12hz-1.7b-base-q8_0.gguf \
    --codec-model cielvox-tokenizer-12hz.gguf \
    --voice clone.wav \
    --ref-text "Exact transcript of clone.wav" \
    --tts "Hello there" \
    --tts-output hello.wav
```

Use a baked voice-pack GGUF:

```bash
./build/bin/stelnettts \
    --backend cielvox2-tts-1.7b-base \
    -m cielvox2-tts-12hz-1.7b-base-q8_0.gguf \
    --codec-model cielvox-tokenizer-12hz.gguf \
    --voice my-voice-pack.gguf \
    --tts "Hello there" \
    --tts-output hello.wav
```

When `--voice` points to a `.wav`, `--ref-text` is required. When `--voice` points to a `.gguf`, it is treated as a baked voice pack.

## Quantisation Notes

- `cielvox2-tts-12hz-1.7b-base-q8_0.gguf` — recommended quantised deployment

## Architecture

| Component | Details |
|---|---|
| Talker LM | 28 layers, 2048 hidden, 16 heads, 8 KV heads, head_dim=128 |
| Output head | 16 codebooks × 2048 (RVQ) |
| Code predictor | 5L + 15 separate codec embedding/lm_head pairs |
| Codec | CielVox-Tokenizer-12Hz (separate GGUF, 12.5 fps RVQ) |
| Audio | 24 kHz mono float32 PCM |

## Implementation

The C++ runtime is implemented in [`src/cielvox2_tts.cpp`](https://github.com/stelnetxcis-create/cielvox2/blob/main/src/cielvox2_tts.cpp) using ggml graphs for the talker / code-predictor path and the companion tokenizer GGUF for codec encode/decode.

## Related

- Companion tokenizer / codec GGUF: [`Xenna/cielvox2-tokenizer-12hz`](https://huggingface.co/Xenna/cielvox2-tokenizer-12hz)
- C++ runtime: **[stelnetxcis-create/cielvox2](https://github.com/stelnetxcis-create/cielvox2)**

## License

Apache-2.0.
