# stelnettts

Dart / Flutter FFI bindings for [StelnetTTS](https://github.com/Cyna/StelnetTTS) — lightweight on-device speech recognition and speech tooling via ggml.

The native project currently ships 43 ASR backends, 48 TTS engines, language detection, punctuation, diarization, translation and chat/text helpers. This package exposes the Dart FFI API and expects the native `libstelnettts` library to be installed or bundled by the application.

## Install

```yaml
dependencies:
  stelnettts: ^0.8.10
```

This package is **pure Dart FFI** and does **not** bundle the native library. Install `libstelnettts` separately or ship it with your app:

```bash
git clone https://github.com/Cyna/StelnetTTS
cd StelnetTTS
cmake -B build && cmake --build build -j
sudo cmake --install build   # installs libstelnettts.{so,dylib,dll}
```

If `libstelnettts` is in a non-standard location, pass `libPath:` to the constructors.

## Quick start

```dart
import 'package:stelnettts/stelnettts.dart';

final model = StelnetTTS.open('ggml-base.en.bin');
final segments = model.transcribe('audio.wav');
for (final seg in segments) {
  print('[${seg.start}s - ${seg.end}s] ${seg.text}');
}
model.close();
```

See the [main repo](https://github.com/Cyna/StelnetTTS) for full API docs, the model registry, and the CLI.

## License

MIT — see [LICENSE](LICENSE).
