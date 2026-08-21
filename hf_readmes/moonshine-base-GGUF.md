---
license: mit
language:
- en
pipeline_tag: automatic-speech-recognition
tags:
- audio
- speech-recognition
- transcription
- gguf
- moonshine
- lightweight
library_name: ggml
base_model: UsefulSensors/moonshine-base
---

# Moonshine Base -- GGUF

GGUF conversions and quantisations of [`UsefulSensors/moonshine-base`](https://huggingface.co/UsefulSensors/moonshine-base) for use with **[Cyna/StelnetTTS](https://github.com/Cyna/StelnetTTS)**.

## Available variants

| File | Quant | Size | Notes |
|---|---|---|---|
| `moonshine-base.gguf` | F32 | 235 MB | Full precision |
| `moonshine-base-q4_k.gguf` | Q4_K | 40 MB | Best size/quality tradeoff |

## Model details

- **Architecture:** Conv1d stem + 8L transformer encoder + 8L transformer decoder (416d, 8 heads, partial RoPE, SiLU/GELU)
- **Parameters:** 61M
- **Languages:** English only
- **License:** MIT
- **Source:** [`UsefulSensors/moonshine-base`](https://huggingface.co/UsefulSensors/moonshine-base)

## Usage with StelnetTTS

```bash
# Auto-download (English tiny only)
./build/bin/stelnettts --backend moonshine -m auto -f audio.wav

# Explicit model path
./build/bin/stelnettts --backend moonshine -m moonshine-base-q4_k.gguf -f audio.wav
```

## Notes

- Moonshine models run on CPU only (GPU not needed for these small models)
- Tokenizer (`tokenizer.bin`) must be in the same directory as the model file
- Base models use head_dim=52 which works on CPU flash_attn
