# CielVox 2

Local-first text-to-speech and automatic speech recognition, optimized for AMD APUs with Vulkan acceleration. Built on whisper.cpp by Cyna, shaped by Stelnet.

## Vision

Keep voice processing local, fast, and private. No cloud dependencies. No forced accounts. Run on a 8700G iGPU at real-time factor ~0.618 on the 0.6b base model.

## Mission

Ship a single binary that handles every mode — base voice cloning, custom voices, and voice design — with clean env-var control and no wrapper latency.

## Authors

Stelnet & Cyna

## Requirements

- Linux (tested on Arch)
- AMD Radeon Vulkan driver (radeonsi/radv)
- `LD_LIBRARY_PATH` pointing to the build `src/` and `ggml/src/` directories

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DGGML_VULKAN=ON
cmake --build build -j$(nproc)
```

## Usage

Required environment variables:

```bash
export LD_LIBRARY_PATH=/path/to/cielvox2/build/src:/path/to/cielvox2/build/ggml/src:$LD_LIBRARY_PATH
export GGML_VULKAN=1
export STELNETTTS_CIELVOX2_TTS_VULKAN_NATIVE=1
export STELNETTTS_CIELVOX2_TTS_CODEC_GPU=1
export STELNETTTS_CIELVOX2_TTS_CP_BACKEND=cpu
```

## Modes

### Base (0.6b) — voice cloning

```bash
./build/bin/stelnettts \
  --backend cielvox2-tts \
  -m models/TTS/cielvox2-tts-0.6b-base-q8_0.gguf \
  --codec-model models/TTS/cielvox2-tokenizer-12hz.gguf \
  --voice-dir <voice_dir> \
  --voice <voice_name> \
  --ref-text "<reference_transcript>" \
  --tts "Hello." \
  --tts-output /tmp/out.wav \
  --i-have-rights \
  --tts-trim-silence \
  --speaker-identity synthetic
```

### Base (1.7b) — voice cloning

```bash
./build/bin/stelnettts \
  --backend cielvox2-tts-1.7b-base \
  -m models/TTS/cielvox2-tts-12hz-1.7b-base-q8_0.gguf \
  --codec-model models/TTS/cielvox2-tokenizer-12hz.gguf \
  --voice-dir <voice_dir> \
  --voice <voice_name> \
  --ref-text "<reference_transcript>" \
  --tts "Hello." \
  --tts-output /tmp/out.wav \
  --i-have-rights \
  --tts-trim-silence \
  --speaker-identity synthetic
```

### CustomVoice (0.6b / 1.7b) — style-controlled synthesis

*Note: CustomVoice mode is under active development. Parameters may change.* 

```bash
./build/bin/stelnettts \
  --backend cielvox2-tts-customvoice \
  -m models/TTS/cielvox2-tts-1.7b-customvoice-q8_0.gguf \
  --codec-model models/TTS/cielvox2-tokenizer-12hz.gguf \
  --voice <preset_name> \
  --tts "Hello." \
  --tts-output /tmp/out.wav \
  --i-have-rights \
  --tts-trim-silence
```

### VoiceDesign (1.7b) — text-described voice

```bash
./build/bin/stelnettts \
  --backend cielvox2-tts-1.7b-voicedesign \
  -m models/TTS/cielvox2-tts-1.7b-voicedesign-q8_0.gguf \
  --codec-model models/TTS/cielvox2-tokenizer-12hz.gguf \
  --instruct "A young female voice with a slight British accent, energetic, slightly fast paced" \
  --tts "Hello." \
  --tts-output /tmp/out.wav \
  --i-have-rights \
  --tts-trim-silence \
  --speaker-identity synthetic
```

### Chatterbox (v3) — s3gen + t3 codec

```bash
STELNETTTS_CHATTERBOX_T3_GPU=1 \
STELNETTTS_CHATTERBOX_S3GEN_CPU=0 \
GGML_VULKAN=1 \
LD_LIBRARY_PATH=/path/to/cielvox2/build/src:/path/to/cielvox2/build/ggml/src:$LD_LIBRARY_PATH \
./build/bin/stelnettts \
  --backend chatterbox \
  -m models/TTS/chatterbox/chatterbox-v3-t3-q8_0.gguf \
  --codec-model models/TTS/chatterbox/chatterbox-v3-s3gen-q8_0.gguf \
  --voice data/TTS/<voice>/<voice>.wav \
  --ref-text "<reference_transcript>" \
  --tts "Hello." \
  --tts-output /tmp/out.wav \
  --i-have-rights \
  --tts-trim-silence \
  --speaker-identity synthetic \
  --no-spoken-disclaimer \
  --accept-marking-responsibility
```

### ASR (0.6b) — local speech recognition

```bash
LD_LIBRARY_PATH=/path/to/cielvox2/build/src:/path/to/cielvox2/build/ggml/src \
./build/bin/stelnettts \
  --backend cielvox2-asr \
  -m models/ASR/cielvox-asr-0.6b-q8_0.gguf \
  -f input.wav \
  --output-file /tmp/transcript \
  -l en \
  -t $(nproc) \
  --i-have-rights
```

Achieves ~8.3x realtime on AMD Radeon 780M.

## Flags explained

- `--backend` — selects the model type. Use `cielvox2-tts` for 0.6b base, `cielvox2-tts-1.7b-base` for 1.7b base, `cielvox2-tts-customvoice` for presets, `cielvox2-tts-1.7b-voicedesign` for voice design.
- `-m` — path to the model GGUF.
- `--codec-model` — path to the tokenizer/codec GGUF. Same file works for all modes.
- `--voice` — for base modes: path to reference `.wav`. For customvoice: preset name. For voicedesign: not used.
- `--ref-text` — transcript of the reference audio. Auto-transcribed if omitted.
- `--instruct` — natural-language voice description. Only for VoiceDesign mode.
- `--tts` — text to synthesize.
- `--tts-output` — where to write the `.wav`.
- `--i-have-rights` — required for voice cloning.
- `--tts-trim-silence` — strip leading silence.
- `--speaker-identity synthetic` — suppress spoken AI disclosure when the voice is not a real person.
- `--list-backends` — print all compiled backends.

## Env flags explained

- `GGML_VULKAN=1` — enable Vulkan backend.
- `STELNETTTS_CIELVOX2_TTS_VULKAN_NATIVE=1` — use native Vulkan memory.
- `STELNETTTS_CIELVOX2_TTS_CODEC_GPU=1` — run codec on GPU.
- `STELNETTTS_CIELVOX2_TTS_CP_BACKEND=cpu` — pin the code predictor to CPU.

## License

MIT

## ASR (Automatic Speech Recognition)

```bash
LD_LIBRARY_PATH=/path/to/cielvox2/build/src:/path/to/cielvox2/build/ggml/src \
./build/bin/stelnettts \
  --backend cielvox2-asr \
  -m models/ASR/cielvox-asr-0.6b-q8_0.gguf \
  -f <audio_file.wav> \
  --output-file /tmp/transcript \
  -l en \
  -t $(nproc) \
  --i-have-rights
```

Tested at 8.3x realtime on AMD R7 8700G with Radeon 780M iGPU.

## Voice Cloning Disclaimer

Voice cloning requires explicit consent. Use `--speaker-identity synthetic` for synthetic voices, `real_person` for real speakers (adds AI disclosure), or `unknown` when unsure. Always include `--accept-marking-responsibility` when disabling watermarks.

## Notes

- Build target: use `--target stelnettts-cli` for the main binary, or no target for all
- HuggingFace model READMEs: see `hf_readmes/` directory for individual model documentation
- Chatterbox models: `chatterbox-v3-t3-q8_0.gguf` (T3 model) + `chatterbox-v3-s3gen-q8_0.gguf` (S3Gen codec)
- ASR backend: `cielvox2-asr` (Whisper-family, 0.6b default, 1.7b available)
