---
license: cc-by-4.0
language:
- en
pipeline_tag: automatic-speech-recognition
tags:
- audio
- speech-recognition
- transcription
- ggml
- gguf
- parakeet
- tdt
- fastconformer
- english
library_name: ggml
base_model: nvidia/parakeet-tdt-1.1b
---


*Note: Documentation under construction. Parameters and file names may change.*

# Parakeet TDT 1.1B — GGUF (ggml-quantised)

GGUF / ggml conversions of [`nvidia/parakeet-tdt-1.1b`](https://huggingface.co/nvidia/parakeet-tdt-1.1b) for use with the `stelnettts` CLI from **[Cyna/StelnetTTS](https://github.com/Cyna/StelnetTTS)**.

The **larger** Parakeet TDT — 1.1 B parameters, 42-layer FastConformer encoder. The biggest pure-English TDT variant in the family. Pick this when you want maximum WER quality on long-tail English vocabulary and don't mind paying 2× the compute relative to 0.6 B.

- **English-only**, lowercase output without punctuation
- **Built-in word-level timestamps** from the TDT decoder
- **CC-BY-4.0** licence

This repo provides three quantisations, all converted from the same `.nemo` checkpoint via the `convert-parakeet-to-gguf.py` script and quantised with `stelnettts-quantize`.

## Files

| File | Size | Notes |
| --- | ---: | --- |
| `parakeet-tdt-1.1b.gguf`        | 2.14 GB | F16, full precision |
| `parakeet-tdt-1.1b-q8_0.gguf`   | 1.27 GB | Q8_0, near-lossless |
| `parakeet-tdt-1.1b-q4_k.gguf`   | 808 MB  | **Q4_K — recommended default** |

Smoke test on `samples/jfk.wav` (11 s clip, M1 Metal):

| Quant | Time | Realtime | Output |
| --- | ---: | ---: | --- |
| F16  | 0.68 s | 16.1× | "and so my fellow americans ask not what your country can do for you ask what you can do for your country" |
| Q8_0 | 0.67 s | 16.4× | (identical) |
| Q4_K | 0.69 s | 16.0× | (identical) |

Output is **lowercase, no punctuation** by design — the upstream vocab is lowercase-only. If you need proper casing/punctuation, pipe the output through a punctuation-restoration post-processor (`--punc-model fullstop-punc` or `fireredpunc`).

## Quick Start

```bash
# 1. Build the runtime
git clone https://github.com/Cyna/StelnetTTS
cd StelnetTTS
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc) --target stelnettts-lib

# 2a. Auto-download via the registry key
./build/bin/stelnettts -m parakeet-tdt-1.1b --auto-download -f your-audio.wav

# 2b. Or explicit download + load
hf download Xenna/parakeet-tdt-1.1b-GGUF \
    parakeet-tdt-1.1b-q4_k.gguf --local-dir .
./build/bin/stelnettts -m parakeet-tdt-1.1b-q4_k.gguf -f your-audio.wav

# 2c. Lowercase output → add punctuation
./build/bin/stelnettts -m parakeet-tdt-1.1b --punc-model fullstop-punc -f your-audio.wav
```

## When to pick this over the other Parakeet variants

| Scenario | Pick |
| --- | --- |
| English, long-tail vocab, fine with 2× compute | **tdt-1.1b** (this repo) |
| English, best WER per FLOP, mixed-case output | [`Xenna/parakeet-tdt-0.6b-v2-GGUF`](https://huggingface.co/Xenna/parakeet-tdt-0.6b-v2-GGUF) |
| Multilingual (25 EU languages) | [`Xenna/parakeet-tdt-0.6b-v3-GGUF`](https://huggingface.co/Xenna/parakeet-tdt-0.6b-v3-GGUF) |
| Tight RAM, English | [`Xenna/parakeet-tdt_ctc-110m-GGUF`](https://huggingface.co/Xenna/parakeet-tdt_ctc-110m-GGUF) |
| English 1.1B with proper casing/punct in output | [`Xenna/parakeet-tdt_ctc-1.1b-GGUF`](https://huggingface.co/Xenna/parakeet-tdt_ctc-1.1b-GGUF) |

## Model architecture

| Component | Details |
| --- | --- |
| Encoder       | **42-layer** FastConformer, d=1024, 8 heads, head_dim=128, FFN=4096, conv kernel=9 |
| Subsampling   | Conv2d dw_striding stack, 8× temporal (100 → 12.5 fps) |
| Predictor     | 2-layer LSTM, hidden 640 |
| Joint head    | enc(1024 → 640) + pred(640 → 640) → ReLU → linear(640 → 1029) — TDT, 5 durations |
| Vocab         | 1024 SentencePiece tokens (English, lowercase) + blank |
| Audio         | 16 kHz mono, **80 mel bins**, n_fft=512, hop=160, win=400 |
| Parameters    | ~1.1 B |

42 layers vs 24 for 0.6b — same encoder design, just deeper.

## Attribution

- **Original model:** [`nvidia/parakeet-tdt-1.1b`](https://huggingface.co/nvidia/parakeet-tdt-1.1b) (CC-BY-4.0). NVIDIA NeMo team.
- **GGUF conversion + ggml runtime:** [`Cyna/StelnetTTS`](https://github.com/Cyna/StelnetTTS).

## License

CC-BY-4.0, inherited from the base model.
