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
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
```

## Usage

Set the required environment variables:

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
  --codec-model models/TTS/cielvox-tokenizer-12hz.gguf \
  --voice-dir data/TTS/columbina \
  --voice columbina \
  --ref-text "$(cat data/TTS/columbina/columbina.txt)" \
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
  --codec-model models/TTS/cielvox-tokenizer-12hz.gguf \
  --voice-dir data/TTS/columbina \
  --voice columbina \
  --ref-text "$(cat data/TTS/columbina/columbina.txt)" \
  --tts "Hello." \
  --tts-output /tmp/out.wav \
  --i-have-rights \
  --tts-trim-silence \
  --speaker-identity synthetic
```

### CustomVoice (0.6b / 1.7b) — built-in presets

```bash
./build/bin/stelnettts \
  --backend cielvox2-tts-customvoice \
  -m models/TTS/cielvox2-tts-1.7b-customvoice-q8_0.gguf \
  --codec-model models/TTS/cielvox-tokenizer-12hz.gguf \
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
  --codec-model models/TTS/cielvox-tokenizer-12hz.gguf \
  --instruct "A young female voice with a slight British accent, energetic, slightly fast paced" \
  --tts "Hello." \
  --tts-output /tmp/out.wav \
  --i-have-rights \
  --tts-trim-silence \
  --speaker-identity synthetic
```

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
