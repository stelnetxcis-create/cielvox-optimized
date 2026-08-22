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

# CielVox 0.6B CustomVoice — GGUF

Local-first fixed-speaker TTS model for the `cielvox2-tts-customvoice` backend in **[stelnetxcis-create/cielvox2](https://github.com/stelnetxcis-create/cielvox2)**.

- 9 baked speaker tokens picked via `--voice <name>`
- No ECAPA forward, no codec encoder, no reference audio required
- Apache-2.0 licence

Pair this with the codec at [`Xenna/cielvox2-tokenizer-12hz`](https://huggingface.co/Xenna/cielvox2-tokenizer-12hz) — the talker emits 16-codebook RVQ codes that the codec decoder renders to 24 kHz PCM.

| Speaker | Language / dialect |
|---|---|
| `<default_voice>` (default) | English (M) |
| `<voice_name>` | English (M) |

## Files

| File | Quant | Size | Notes |
|---|---|---:|---|
| `cielvox2-tts-12hz-0.6b-customvoice-q8_0.gguf` | Q8_0 | 968 MB | **Recommended** |

## Quick start

```bash
# 1. Build
git clone https://github.com/stelnetxcis-create/cielvox2
cd cielvox2
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j --target stelnettts-lib

# 2. Pull the talker + codec
huggingface-cli download Xenna/cielvox2-tts-0.6b-customvoice-GGUF \
    cielvox2-tts-12hz-0.6b-customvoice-q8_0.gguf --local-dir .
huggingface-cli download Xenna/cielvox2-tokenizer-12hz \
    cielvox-tokenizer-12hz.gguf --local-dir .

# 3. Synthesise — pick a speaker by name
./build/bin/stelnettts --backend cielvox2-tts-customvoice \
    -m cielvox2-tts-12hz-0.6b-customvoice-q8_0.gguf \
    --codec-model cielvox-tokenizer-12hz.gguf \
    --voice <voice_name> \
    --tts "Hello, this is the <voice> speaker." \
    --tts-output <voice_name>.wav
```

For **auto-download** simply pass `-m auto`:

```bash
./build/bin/stelnettts --backend cielvox2-tts-customvoice -m auto \
    --tts "Auto-download fetches both files." \
    --tts-output out.wav
```

## Quality verification

ASR roundtrip via [`Xenna/parakeet-tdt-0.6b-v3-GGUF`](https://huggingface.co/Xenna/parakeet-tdt-0.6b-v3-GGUF):

| Speaker | Synthesised text | Parakeet output |
|---|---|---|
| `vivian` | `"Hello, this is a CustomVoice test using the vivian speaker."` | `"Hello! This is a custom voice test using the Vivian speaker."` |
| `<default_voice>` | `"The quick brown fox jumps over the lazy dog."` | `"The quick brown fox jumps over the lazy dog."` |
| `serena` | `"Testing the new backend alias and the serena speaker."` | `"Testing the new back end Ilias and the Serena speaker."` |

## Architecture

| Component | Details |
|---|---|
| Talker LM | 28 layers, 1024 hidden, 16 heads, 8 KV heads, head_dim=64 |
| Output head | 16 codebooks × 1024 (RVQ) |
| Code predictor | 5L + 15 separate codec_embedding/lm_head pairs (top-k=50, temp=0.9) |
| Codec | CielVox-Tokenizer-12Hz (separate GGUF, 12.5 fps RVQ) |
| Audio | 24 kHz mono float32 PCM |

## Attribution

- **GGUF conversion + ggml runtime:** **[stelnetxcis-create/cielvox2](https://github.com/stelnetxcis-create/cielvox2)** — see `src/cielvox2_tts.cpp`, `models/convert-cielvox2-tts-to-gguf.py`.

## License

Apache 2.0.
