# stelnettts examples

`stelnettts` runs on-device speech recognition (Whisper-family and other ASR
models) via ggml. It needs the native `libstelnettts` at runtime (see the package
README for how to supply it).

## Transcribe an audio file

```dart
import 'package:stelnettts/stelnettts.dart';

void main() {
  final model = StelnetTTS.open('ggml-base.en.bin');
  try {
    final segments = model.transcribe('audio.wav');
    for (final seg in segments) {
      print('[${seg.start}s - ${seg.end}s] ${seg.text}');
    }
  } finally {
    model.close();
  }
}
```

The library also exposes a chat/LLM surface (`package:stelnettts/stelnettts.dart`
re-exports `src/chat.dart`) for the bundled generation backends.
