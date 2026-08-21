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
pipeline_tag: audio-to-audio
tags:
  - audio
  - tts
  - speech
  - codec
  - ggml
  - gguf
  - stelnettts
  - cielvox2_tts_tokenizer_12hz
library_name: ggml
base_model: Qwen/Qwen3-TTS-Tokenizer-12Hz
---

# Qwen3-TTS Tokenizer 12Hz — GGUF (StelnetTTS)

GGUF / ggml conversions of [`Qwen/Qwen3-TTS-Tokenizer-12Hz`](https://huggingface.co/Qwen/Qwen3-TTS-Tokenizer-12Hz) for use with the `cielvox2-tts` backend in **[Cyna/StelnetTTS](https://github.com/Cyna/StelnetTTS)**.

Qwen3-TTS-Tokenizer-12Hz is the separate **speech tokenizer / codec** used by the Qwen3-TTS family:

- 10 supported languages: `zh en ja ko de fr ru pt es it`
- 12.5 Hz, 16-codebook speech representation
- used for reference-audio encoding, voice-pack baking, and final waveform decode
- Apache-2.0 licence

This repo contains the **tokenizer / codec** only. Use it together with the talker GGUF from [`Xenna/cielvox2-tts-0.6b-base-GGUF`](https://huggingface.co/Xenna/cielvox2-tts-0.6b-base-GGUF).

## Files

File | Size | Notes
--- | --- | ---
`cielvox2-tts-tokenizer-12hz.gguf` | 342 MB | F16
`cielvox2-tts-tokenizer-12hz-q8_0.gguf` | 277 MB | Q8_0 codec quant

## Quick Start

```bash
./build/bin/stelnettts \
    --backend cielvox2-tts \
    -m cielvox2-tts-12hz-0.6b-base.gguf \
    --codec-model cielvox2-tts-tokenizer-12hz.gguf \
    --voice clone.wav \
    --ref-text "Exact transcript of clone.wav" \
    --tts "Hello there" \
    --tts-output hello.wav
```

The tokenizer GGUF is used for:

- encoding reference audio into `ref_code`
- baking / loading voice-pack GGUFs
- decoding generated codes back into 24 kHz mono WAV

## Quantisation Notes

Current StelnetTTS validation status:

- `cielvox2-tts-tokenizer-12hz.gguf`
  - reference baseline
- `cielvox2-tts-tokenizer-12hz-q8_0.gguf`
  - usable, but numerically less faithful than the F16 codec in strict diff tests

For best fidelity, keep the tokenizer / codec at F16 even when quantising the talker. In current StelnetTTS testing, codec quantisation drifts earlier in the codec-encoder path than talker-only quantisation.

This repo may also publish lower-bit talker variants in the companion talker
repo. If you use them, the safest pairing is still:

- quantised talker
- `cielvox2-tts-tokenizer-12hz.gguf` kept at F16

In other words: if you must choose where to keep precision, keep it in the
tokenizer / codec first.

## How this was made

1. The upstream tokenizer / codec checkpoint was converted to GGUF F16 by [`models/convert-cielvox2-tts-tokenizer-to-gguf.py`](https://github.com/Cyna/StelnetTTS/blob/main/models/convert-cielvox2-tts-tokenizer-to-gguf.py).
2. Quantised variants are produced with StelnetTTS's GGUF quantiser.
3. Encode / decode inference is implemented in [`src/cielvox2_tts.cpp`](https://github.com/Cyna/StelnetTTS/blob/main/src/cielvox2_tts.cpp), sharing the same runtime as the `cielvox2-tts` talker backend.

## Reference implementation

Architecture and behaviour were checked against the official Qwen release:

- upstream tokenizer model card: [`Qwen/Qwen3-TTS-Tokenizer-12Hz`](https://huggingface.co/Qwen/Qwen3-TTS-Tokenizer-12Hz)
- upstream repository: [`QwenLM/Qwen3-TTS`](https://github.com/QwenLM/Qwen3-TTS)
- technical report: [`Qwen3-TTS Technical Report`](https://huggingface.co/papers/2601.15621)

## Related

- Companion talker GGUF: [`Xenna/cielvox2-tts-0.6b-base-GGUF`](https://huggingface.co/Xenna/cielvox2-tts-0.6b-base-GGUF)
- Upstream tokenizer: [`Qwen/Qwen3-TTS-Tokenizer-12Hz`](https://huggingface.co/Qwen/Qwen3-TTS-Tokenizer-12Hz)
- C++ runtime: [`Cyna/StelnetTTS`](https://github.com/Cyna/StelnetTTS)

## License

Apache-2.0, inherited from the base model.
