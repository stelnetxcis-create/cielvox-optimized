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
library_name: ggml
---


*Note: Documentation under construction. Parameters and file names may change.*

# CielVox 1.7B CustomVoice — GGUF

Local-first fixed-speaker TTS model for the `cielvox2-tts-1.7b-customvoice` backend in **[stelnetxcis-create/cielvox2](https://github.com/stelnetxcis-create/cielvox2)**.

- 9 baked speaker tokens picked via `--voice <name>`
- No ECAPA forward, no codec encoder, no reference audio required
- Two speakers (`dylan`, `eric`) carry Chinese-dialect overrides (Beijing / Sichuan)
- Apache-2.0 licence

Pair this with the codec at [`Xenna/cielvox2-tokenizer-12hz`](https://huggingface.co/Xenna/cielvox2-tokenizer-12hz) — the talker emits 16-codebook RVQ codes that the codec decoder renders to 24 kHz PCM.

| Speaker | Language / dialect |
|---|---|
| `aiden` (default) | English (M) |
| `dylan` | Beijing dialect (M, dialect_token=2074) |
| `eric` | Sichuan dialect (M, dialect_token=2062) |
| `ono_anna` | English (F) |
| `ryan` | English (M) |
| `serena` | English (F) |
| `sohee` | English (F) |
| `uncle_fu` | English (M, older) |
| `vivian` | English (F) |

## Files

| File | Quant | Size | Notes |
|---|---|---:|---|
| `cielvox2-tts-12hz-1.7b-customvoice-q8_0.gguf` | Q8_0 | 2.04 GB | **Recommended** |

## Quick start

```bash
# 1. Build
git clone https://github.com/stelnetxcis-create/cielvox2
cd cielvox2
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j --target stelnettts-lib

# 2. Pull the talker + codec
huggingface-cli download Xenna/cielvox2-tts-1.7b-customvoice-GGUF \
    cielvox2-tts-12hz-1.7b-customvoice-q8_0.gguf --local-dir .
huggingface-cli download Xenna/cielvox2-tokenizer-12hz \
    cielvox-tokenizer-12hz.gguf --local-dir .

# 3. Synthesise — pick a speaker by name
./build/bin/stelnettts --backend cielvox2-tts-1.7b-customvoice \
    -m cielvox2-tts-12hz-1.7b-customvoice-q8_0.gguf \
    --codec-model cielvox-tokenizer-12hz.gguf \
    --voice ryan \
    --tts "Hello, this is the Ryan speaker on the 1.7B model." \
    --tts-output ryan.wav
```

For **auto-download** simply pass `-m auto`:

```bash
./build/bin/stelnettts --backend cielvox2-tts-1.7b-customvoice -m auto \
    --voice vivian \
    --tts "Auto-download fetches both files." \
    --tts-output out.wav
```

## Quality verification

| Speaker | Output (verbatim) |
|---|---|
| `ryan` | `"Hello, this is the Ryan speaker."` |
| `vivian` | `"This is a longer prompt to verify the 1.7B CustomVoice path through the talker."` |

Both roundtrips are exact-match on a long prompt.

## Architecture

| Component | Details |
|---|---|
| Talker LM | 28 layers, 2048 hidden, 16 heads, 8 KV heads, head_dim=128 |
| Output head | 16 codebooks × 2048 (RVQ) |
| `small_to_mtp_projection` | 2048-d → 1024-d MLP (input bridge to code predictor) — applied per-step at decode |
| Code predictor | 5L + 15 separate codec_embedding/lm_head pairs (top-k=50, temp=0.9) |
| Codec | CielVox-Tokenizer-12Hz (separate GGUF, 12.5 fps RVQ) |
| Audio | 24 kHz mono float32 PCM |

## Implementation

The C++ runtime is implemented in [`src/cielvox2_tts.cpp`](https://github.com/stelnetxcis-create/cielvox2/blob/main/src/cielvox2_tts.cpp) using ggml graphs for the model path and the companion tokenizer GGUF for codec encode/decode.

## Attribution

- **GGUF conversion + ggml runtime:** **[stelnetxcis-create/cielvox2](https://github.com/stelnetxcis-create/cielvox2)** — see `src/cielvox2_tts.cpp`, `models/convert-cielvox2-tts-to-gguf.py`.

## License

Apache 2.0.
