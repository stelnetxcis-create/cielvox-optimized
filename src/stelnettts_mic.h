// stelnettts_mic.h — library-level microphone capture (PLAN #62d).
//
// Cross-platform live PCM capture via miniaudio's `ma_device` (Core
// Audio on macOS, ALSA/PulseAudio on Linux, WASAPI on Windows). The
// runtime symbols are already linked into libstelnettts because
// `stelnettts_audio.cpp` defines `MINIAUDIO_IMPLEMENTATION`.
//
// Library consumers get raw float32 mono PCM in their callback, sized
// per-driver (typically 256-4096 samples per call). To do dictation /
// streaming ASR, combine with `stelnettts_session_stream_open()` +
// `stelnettts_stream_feed()` from inside the callback.
//
// Wrappers add a `Session.start_mic_streaming(callback)` convenience
// helper that wires mic + stream + per-callback feed in one shot.

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct stelnettts_mic;

// Per-frame callback. `pcm` is `n_samples` mono float32 in [-1, 1].
// The callback runs on miniaudio's audio thread — keep it short and
// non-blocking. To process the audio further (e.g. push into a
// streaming decoder), copy into a thread-safe queue and process on
// another thread.
typedef void (*stelnettts_mic_callback)(const float* pcm, int n_samples, void* userdata);

// Open a microphone capture device. Returns nullptr on failure.
//
//   sample_rate: target sample rate in Hz. 16000 matches every ASR
//                backend. miniaudio resamples on the fly when the
//                device's native rate differs.
//   channels:    1 (mono) recommended. Passing 2 hands the callback
//                interleaved L/R; the caller is responsible for mixing.
//   cb:          callback fired per-frame on the audio thread.
//   userdata:    opaque pointer passed to `cb` unchanged.
//
// The handle is NOT started — call `stelnettts_mic_start()` to begin
// capture. Use `stelnettts_mic_close()` to release the device + handle.
struct stelnettts_mic* stelnettts_mic_open(int sample_rate, int channels, stelnettts_mic_callback cb, void* userdata);

// Begin capture. Returns 0 on success, non-zero on driver error.
int stelnettts_mic_start(struct stelnettts_mic* m);

// Stop capture. Idempotent. The callback may still fire briefly while
// the driver drains.
int stelnettts_mic_stop(struct stelnettts_mic* m);

// Free the handle + release the device. Implies `_stop()`.
void stelnettts_mic_close(struct stelnettts_mic* m);

// Human-readable name of the default capture device, or empty string
// if no input device is available. Returns a static buffer; not
// thread-safe; valid until the next call.
const char* stelnettts_mic_default_device_name(void);

#ifdef __cplusplus
}
#endif
