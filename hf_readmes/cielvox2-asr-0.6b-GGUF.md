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
  - sv
  - th
  - tr
  - vi
  - yue
  - zh
pipeline_tag: automatic-speech-recognition
tags:
  - audio
  - speech-recognition
  - transcription
  - ggml
  - gguf
  - stelnettts
  - cielvox2-asr
library_name: ggml
---

# CielVox ASR 0.6B — GGUF

Local-first speech-LLM ASR model for the `cielvox2-asr-main` CLI in **[stelnetxcis-create/cielvox2](https://github.com/stelnetxcis-create/cielvox2)**.

- **30 languages + 22 Chinese dialects** with automatic language detection
- **6.42 % avg WER** on the HuggingFace Open ASR Leaderboard
- **Apache-2.0** licence
- Speech-LLM architecture: Whisper-style audio encoder feeds a 0.6B LLM via embedding splice at `<|audio_pad|>` placeholder positions

## Files

| File | Size | Notes |
|---|---|---:|---|
| `cielvox2-asr-0.6b-q8_0.gguf` | 961 MB | Q8_0, near-lossless |
| `cielvox2-asr-0.6b-q4_k.gguf` | 631 MB | **Q4_K — recommended default**, faster than realtime on a 4-core CPU |

## Quick Start

Build the runtime:

```bash
git clone https://github.com/stelnetxcis-create/cielvox2
cd cielvox2
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc) --target cielvox2-asr-main
```

Download a quantisation:

```bash
huggingface-cli download Xenna/cielvox2-asr-0.6b-GGUF \
    cielvox2-asr-0.6b-q4_k.gguf --local-dir .
```

Transcribe:

```bash
./build/bin/cielvox2-asr-main \
    -m cielvox2-asr-0.6b-q4_k.gguf \
    -f your-audio.wav -t 8
```

Audio must be 16 kHz mono 16-bit PCM WAV. Pre-convert with:

```bash
ffmpeg -i input.mp3 -ar 16000 -ac 1 -c:a pcm_s16le output.wav
```

## Architecture

| Component | Details |
|---|---|
| Audio encoder | 18-layer Whisper-style pre-LN Transformer, d=896, heads=14, head_dim=64, FFN=3584 |
| Conv subsampler | 3 × Conv2D stride-2, then linear (7680 → 896). Output ~13 frames/sec |
| Projector | ln_post → proj1 (896→896) → GELU → proj2 (896→1024) |
| LLM | 28 layers, hidden=1024, 16 Q heads / 8 KV heads (GQA), head_dim=128, FFN=3072 |
| Vocab | 151,936 tokens |
| Audio | 16 kHz mono, 128 mel bins |

The mel filterbank is baked into the GGUF as `audio.mel_filters` / `audio.mel_window`, so the C++ runtime computes the log-mel spectrogram natively without needing torch / librosa / scipy at inference time.

## Supported languages

`ar cs da de el en es fa fi fil fr hu id it ja ko ms nl pl pt ro ru sv th tr vi yue zh` plus 22 Chinese dialects (auto-detected at inference time).

## Implementation

Inference is implemented in [`src/cielvox2_asr.cpp`](https://github.com/stelnetxcis-create/cielvox2/blob/main/src/cielvox2_asr.cpp): the encoder and the LLM each run as one ggml graph, with a persistent F32 KV cache shared between prefill and per-token decode steps.

## Related

- C++ runtime: **[stelnetxcis-create/cielvox2](https://github.com/stelnetxcis-create/cielvox2)**
- Sister releases in the same family:
  - [`Xenna/cielvox2-tts-0.6b-base-GGUF`](https://huggingface.co/Xenna/cielvox2-tts-0.6b-base-GGUF) — CielVox 0.6B Base TTS
  - [`Xenna/cielvox2-tokenizer-12hz`](https://huggingface.co/Xenna/cielvox2-tokenizer-12hz) — shared codec

## License

Apache-2.0, inherited from the base model.
