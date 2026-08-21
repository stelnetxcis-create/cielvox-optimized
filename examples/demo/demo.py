#!/usr/bin/env python3
"""StelnetTTS Python demo — speech-to-text transcription."""

import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'python'))
from stelnettts import StelnetTTS

MODEL = os.environ.get("STELNETTTS_MODEL", "models/ggml-tiny.en.bin")
AUDIO = os.environ.get("STELNETTTS_AUDIO", "samples/jfk.wav")
LIB = os.environ.get("STELNETTTS_LIB", None)
HELPERS = os.environ.get("STELNETTTS_HELPERS", None)

print("=== StelnetTTS Python Demo ===\n")

kwargs = {}
if LIB: kwargs["lib_path"] = LIB
if HELPERS: kwargs["helpers_lib_path"] = HELPERS

model = StelnetTTS(MODEL, **kwargs)
print(f"Model loaded: {MODEL}")

# Transcribe
print(f"Transcribing: {AUDIO}")
segments = model.transcribe(AUDIO)

print(f"\nResult: {len(segments)} segments")
for seg in segments:
    print(f"  [{seg.start:.1f}s - {seg.end:.1f}s] {seg.text}")

print(f"\nDetected language: {model.detected_language}")
model.close()
print("\nAll Python demo tests passed!")
