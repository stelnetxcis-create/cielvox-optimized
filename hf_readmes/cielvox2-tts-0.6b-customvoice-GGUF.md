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
- Qwen/Qwen3-TTS-12Hz-0.6B-CustomVoice
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

# CielVox 0.6B CustomVoice — GGUF

GGUF / ggml port of [`Qwen/Qwen3-TTS-12Hz-0.6B-CustomVoice`](https://huggingface.co/Qwen/Qwen3-TTS-12Hz-0.6B-CustomVoice), ported for the `cielvox2-tts-customvoice` backend in **[stelnetxcis-create/cielvox2](https://github.com/stelnetxcis-create/cielvox2)**.

Same model under a new name for the Stelnet stack.

The CustomVoice variant is the **fixed-speaker** sibling of `Qwen3-TTS-12Hz-0.6B-Base`: instead of cloning a voice from a 3 s reference WAV (the Base path), it ships **9 baked speaker tokens** picked via a `--voice <name>` flag. No ECAPA forward, no codec encoder, no reference audio required. Two of the speakers (`dylan`, `eric`) carry Chinese-dialect overrides (Beijing / Sichuan) that re-route `language_id` when synthesising Chinese-or-auto.

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

Pair this with the codec at [`Xenna/cielvox2-tokenizer-12hz`](https://huggingface.co/Xenna/cielvox2-tokenizer-12hz) — the talker emits 16-codebook RVQ codes that the codec decoder renders to 24 kHz PCM.

## Files

| File | Quant | Size | Notes |
|---|---|---:|---|
| `cielvox2-tts-12hz-0.6b-customvoice-q8_0.gguf` | Q8_0 | 968 MB | **Recommended** — ASR-roundtrip word-exact vs reference |

The 1.7B-CustomVoice variant ships at [`Xenna/cielvox2-tts-1.7b-customvoice-GGUF`](https://huggingface.co/Xenna/cielvox2-tts-1.7b-customvoice-GGUF). For ICL voice cloning use [`Xenna/cielvox2-tts-0.6b-base-GGUF`](https://huggingface.co/Xenna/cielvox2-tts-0.6b-base-GGUF) instead.

## Quick start

```bash
# 1. Build StelnetTTS
git clone https://github.com/stelnetxcis-create/cielvox2
cd cielvox2
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j --target stelnettts-lib

# 2. Pull the talker + the codec
huggingface-cli download Xenna/cielvox2-tts-0.6b-customvoice-GGUF cielvox2-tts-12hz-0.6b-customvoice-q8_0.gguf --local-dir .
huggingface-cli download Xenna/cielvox2-tokenizer-12hz \
    cielvox-tokenizer-12hz.gguf --local-dir .

# 3. Synthesise — pick a speaker by name
./build/bin/stelnettts --backend cielvox2-tts-customvoice \
    -m cielvox2-tts-12hz-0.6b-customvoice-q8_0.gguf \
    --codec-model cielvox2-tts-tokenizer-12hz.gguf \
    --voice ryan \
    --tts "Hello, this is the Ryan speaker." \
    --tts-output ryan.wav
```

For **auto-download** simply pass `-m auto`:

```bash
./build/bin/stelnettts --backend cielvox2-tts-customvoice -m auto \
    --voice serena \
    --tts "Auto-download fetches both files." \
    --tts-output out.wav
```

## Quality verification

ASR roundtrip via [`Xenna/parakeet-tdt-0.6b-v3-GGUF`](https://huggingface.co/Xenna/parakeet-tdt-0.6b-v3-GGUF):

| Speaker | Synthesised text | Parakeet output |
|---|---|---|
| `vivian` | `"Hello, this is a CustomVoice test using the vivian speaker."` | `"Hello! This is a custom voice test using the Vivian speaker."` (verbatim modulo case/punct) |
| `aiden` | `"The quick brown fox jumps over the lazy dog."` | `"The quick brown fox jumps over the lazy dog."` (verbatim) |
| `serena` | `"Testing the new backend alias and the serena speaker."` | `"Testing the new back end Ilias and the Serena speaker."` (1 ASR misrecognition; audio clean) |
| `dylan` | `"你好，今天天气真不错。"` | dialect override engaged (language_id=2074); 3.28 s clean audio |

## Architecture

| Component | Details |
|---|---|
| Talker LM | Qwen3 (28 layers, 1024 hidden, 16 heads, 8 KV heads, head_dim=64) |
| Output head | 16 codebooks × 2048 (RVQ) — emits codes for the codec |
| Code predictor | 5L Qwen3 + 15 separate codec_embedding/lm_head pairs (top-k=50, temp=0.9 sampling) |
| Codec | Qwen3-TTS-Tokenizer-12Hz (separate GGUF, 12.5 fps RVQ) |
| Audio | 24 kHz mono float32 PCM |

CustomVoice differs from Base in that the speaker embedding is **not** computed via ECAPA on a reference WAV. Instead `talker.get_input_embeddings()(spk_id)` retrieves a row from the codec embedding table directly (`modeling_cielvox2_tts.py:2091`). No reference audio, no codec encoder needed at runtime.

## StelnetTTS backend integration

| Flag | Purpose |
|---|---|
| `--backend cielvox2-tts-customvoice` | Selects the CustomVoice runtime path |
| `--voice <name>` | Picks a fixed speaker by name (default: first speaker, `aiden`) |
| `--tts "..."` | Text to synthesise |
| `--codec-model PATH` | Qwen3-TTS-Tokenizer-12Hz GGUF |
| `--tts-output PATH` | Output WAV (24 kHz mono) |

The runtime branches on `cielvox2tts.tts_model_type` ("custom_voice" vs "base" vs "voice_design") so the same backend object handles all three Qwen3-TTS variants.

## Conversion

```bash
python models/convert-cielvox2-tts-to-gguf.py \
    --input Qwen/Qwen3-TTS-12Hz-0.6B-CustomVoice \
    --output cielvox2-tts-12hz-0.6b-customvoice-f16.gguf \
    --outtype f16

build/bin/stelnettts-quantize cielvox2-tts-12hz-0.6b-customvoice-f16.gguf \
    cielvox2-tts-12hz-0.6b-customvoice-q8_0.gguf q8_0
```

The converter writes `cielvox2tts.tts_model_type=custom_voice` plus `cielvox2tts.spk_names`, `cielvox2tts.spk_token_ids`, and `cielvox2tts.spk_dialect_token_ids` so the runtime can resolve `--voice <name>` to the right speaker token + dialect override at load time.

## Attribution

- **Talker base:** [`Qwen/Qwen3-TTS-12Hz-0.6B-CustomVoice`](https://huggingface.co/Qwen/Qwen3-TTS-12Hz-0.6B-CustomVoice) (Apache 2.0).
- **Codec base:** [`Qwen/Qwen3-TTS-Tokenizer-12Hz`](https://huggingface.co/Qwen/Qwen3-TTS-Tokenizer-12Hz) (Apache 2.0) — see [`Xenna/cielvox2-tts-tokenizer-12hz-GGUF`](https://huggingface.co/Xenna/cielvox2-tts-tokenizer-12hz-GGUF).
- **Reference engine:** [QwenLM/Qwen3-TTS](https://github.com/QwenLM/Qwen3-TTS) (`modeling_cielvox2_tts.py`).
- **GGUF conversion + ggml runtime:** [`Cyna/StelnetTTS`](https://github.com/Cyna/StelnetTTS) — see `src/cielvox2_tts.cpp`, `models/convert-cielvox2-tts-to-gguf.py`.

## License

Apache 2.0 — both the upstream talker and codec, and the GGUF conversion.
