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
base_model:
- Qwen/Qwen3-TTS-12Hz-1.7B-CustomVoice
pipeline_tag: text-to-speech
tags:
- tts
- text-to-speech
- qwen3
- cielvox2-tts
- gguf
- stelnettts
library_name: ggml
---

# CielVox 1.7B CustomVoice — GGUF

GGUF / ggml conversion of the fixed-speaker model for the `cielvox2-tts-1.7b-customvoice` backend in **[stelnetxcis-create/cielvox2](https://github.com/stelnetxcis-create/cielvox2)**.

The 1.7B sibling of [`Xenna/cielvox2-tts-0.6b-customvoice-GGUF`](https://huggingface.co/Xenna/cielvox2-tts-0.6b-customvoice-GGUF). Same fixed-speaker pattern (9 baked speakers, no ECAPA, no reference WAV), but on the 1.7B talker — `hidden=2048`, `head_dim=128`, with a `small_to_mtp_projection` MLP that maps the 2048-d talker hidden state down to the 1024-d code predictor input space.

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

Pair this with the codec at [`Xenna/cielvox2-tts-tokenizer-12hz-GGUF`](https://huggingface.co/Xenna/cielvox2-tts-tokenizer-12hz-GGUF) — the talker emits 16-codebook RVQ codes that the codec decoder renders to 24 kHz PCM.

## Files

| File | Quant | Size | Notes |
|---|---|---:|---|
| `cielvox2-tts-12hz-1.7b-customvoice-f16.gguf`  | F16  | 3.84 GB | Reference quality |
| `cielvox2-tts-12hz-1.7b-customvoice-q8_0.gguf` | Q8_0 | 2.04 GB | **Recommended** — ASR-roundtrip word-exact vs F16 |

The 0.6B-CustomVoice variant ships at [`Xenna/cielvox2-tts-0.6b-customvoice-GGUF`](https://huggingface.co/Xenna/cielvox2-tts-0.6b-customvoice-GGUF). The 1.7B-Base (ICL voice clone) variant ships at [`Xenna/cielvox2-tts-1.7b-base-GGUF`](https://huggingface.co/Xenna/cielvox2-tts-1.7b-base-GGUF).

## Quick start

```bash
# 1. Build StelnetTTS
git clone https://github.com/stelnetxcis-create/cielvox2
cd cielvox2
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j --target stelnettts-lib

# 2. Pull the talker + the codec
huggingface-cli download Xenna/cielvox2-tts-1.7b-customvoice-GGUF cielvox2-tts-12hz-1.7b-customvoice-q8_0.gguf --local-dir .
huggingface-cli download Xenna/cielvox2-tts-tokenizer-12hz-GGUF cielvox2-tts-tokenizer-12hz.gguf --local-dir .

# 3. Synthesise — pick a speaker by name
./build/bin/stelnettts --backend cielvox2-tts-1.7b-customvoice \
    -m cielvox2-tts-12hz-1.7b-customvoice-q8_0.gguf \
    --codec-model cielvox2-tts-tokenizer-12hz.gguf \
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

ASR roundtrip via [`Xenna/parakeet-tdt-0.6b-v3-GGUF`](https://huggingface.co/Xenna/parakeet-tdt-0.6b-v3-GGUF):

| Variant | Speaker | Output (verbatim) |
|---|---|---|
| Q8_0 | `ryan` | `"Hello, this is the Ryan speaker."` |
| F16  | `vivian` | `"This is a longer prompt to verify the 1.7B CustomVoice path through the talker."` |

Both roundtrips are exact-match on a long prompt (the regression test for the `small_to_mtp_projection` per-step fix in commit `2cc7aeb`).

## Architecture

| Component | Details |
|---|---|
| Talker LM | Qwen3 (28 layers, **2048 hidden**, 16 heads, 8 KV heads, head_dim=128) |
| Output head | 16 codebooks × 2048 (RVQ) — emits codes for the codec |
| `small_to_mtp_projection` | 2048-d → 1024-d MLP (input bridge to code predictor) — **applied per-step** at decode (steps 1..14, fix in `2cc7aeb`) |
| Code predictor | 5L Qwen3 (1024 hidden) + 15 separate codec_embedding/lm_head pairs (top-k=50, temp=0.9) |
| Codec | Qwen3-TTS-Tokenizer-12Hz (separate GGUF, 12.5 fps RVQ) |
| Audio | 24 kHz mono float32 PCM |

CustomVoice 1.7B uses the same fixed-speaker contract as 0.6B-CV: `talker.get_input_embeddings()(spk_id)` retrieves a row from the codec embedding table directly. The 2048-d hidden state then gets projected down to 1024-d via `small_to_mtp_projection` before each code predictor step.

## StelnetTTS backend integration

| Flag | Purpose |
|---|---|
| `--backend cielvox2-tts-1.7b-customvoice` | Selects the 1.7B CustomVoice runtime path |
| `--voice <name>` | Picks a fixed speaker by name (default: first speaker, `aiden`) |
| `--tts "..."` | Text to synthesise |
| `--codec-model PATH` | Qwen3-TTS-Tokenizer-12Hz GGUF |
| `--tts-output PATH` | Output WAV (24 kHz mono) |

The runtime branches on `cielvox2tts.tts_model_type` and on the GGUF talker hidden_dim (`spk_enc_dim`) so the same backend object handles 0.6B/1.7B Base/CustomVoice/VoiceDesign automatically.

## Conversion

```bash
python models/convert-cielvox2-tts-to-gguf.py \
    --input Qwen/Qwen3-TTS-12Hz-1.7B-CustomVoice \
    --output cielvox2-tts-12hz-1.7b-customvoice-f16.gguf \
    --outtype f16

build/bin/stelnettts-quantize cielvox2-tts-12hz-1.7b-customvoice-f16.gguf \
    cielvox2-tts-12hz-1.7b-customvoice-q8_0.gguf q8_0
```

## Attribution

- **Talker base:** [`Qwen/Qwen3-TTS-12Hz-1.7B-CustomVoice`](https://huggingface.co/Qwen/Qwen3-TTS-12Hz-1.7B-CustomVoice) (Apache 2.0).
- **Codec base:** [`Qwen/Qwen3-TTS-Tokenizer-12Hz`](https://huggingface.co/Qwen/Qwen3-TTS-Tokenizer-12Hz) (Apache 2.0) — see [`Xenna/cielvox2-tts-tokenizer-12hz-GGUF`](https://huggingface.co/Xenna/cielvox2-tts-tokenizer-12hz-GGUF).
- **Reference engine:** [QwenLM/Qwen3-TTS](https://github.com/QwenLM/Qwen3-TTS) (`modeling_cielvox2_tts.py`).
- **GGUF conversion + ggml runtime:** [`Cyna/StelnetTTS`](https://github.com/Cyna/StelnetTTS) — see `src/cielvox2_tts.cpp`, `models/convert-cielvox2-tts-to-gguf.py`.

## License

Apache 2.0 — both the upstream talker and codec, and the GGUF conversion.
