---
license: cc-by-4.0
base_model: nvidia/stt_ka_fastconformer_hybrid_large_pc
language:
  - ka
tags:
  - automatic-speech-recognition
  - forced-alignment
  - gguf
  - stelnettts
  - fastconformer
  - ctc
  - nemo
pipeline_tag: automatic-speech-recognition
---


*Note: Documentation under construction. Parameters and file names may change.*

# stt-ka-fastconformer-hybrid-ctc-large-GGUF

GGUF conversions of the **CTC branch** of [nvidia/stt_ka_fastconformer_hybrid_large_pc](https://huggingface.co/nvidia/stt_ka_fastconformer_hybrid_large_pc) for [StelnetTTS](https://github.com/Cyna/StelnetTTS). The upstream model is a hybrid transducer+CTC Georgian ASR release; the shared FastConformer encoder plus the auxiliary CTC head are extracted here as a standalone CTC model (the RNNT prediction network and joint are dropped), giving a compact Georgian ASR **and forced-alignment** model with punctuation + capitalisation.

| Quant | Size | Description |
|---|---|---|
| F16 | 219 MB | Full precision |
| Q8_0 | 130 MB | 8-bit |
| Q4_K | 82 MB | 4-bit K-quant (recommended) |

## Architecture

17-layer NeMo FastConformer encoder + Conv1d CTC head. d_model=512, 8 heads, SentencePiece vocab, 80 log-mel features, ~115M params.

## Usage

```bash
# Georgian ASR:
stelnettts --backend fastconformer-ctc -m stt-ka-fastconformer-hybrid-ctc-large-q4_k.gguf -f audio.wav

# Forced alignment (word timestamps for known text, or re-timing an .srt):
stelnettts --align-only -am stt-ka-fastconformer-hybrid-ctc-large-q4_k.gguf \
    -f audio.wav --text-file subtitles.srt --align-output retimed.srt
```

## Attribution

All credit for the model goes to NVIDIA's NeMo team; this repository only repackages the CTC branch in GGUF form under the same CC-BY-4.0 license. Conversion: `models/convert-stt-fastconformer-ctc-to-gguf.py` in StelnetTTS.
