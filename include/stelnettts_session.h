// stelnettts_session.h — forward declarations for the session C ABI.
//
// AUTO-GENERATED from CA_EXPORT definitions in src/stelnettts_c_api.cpp.
// Suppresses -Wmissing-declarations when stelnettts_c_api.cpp includes this.

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef STELNETTTS_SHARED
#ifdef _WIN32
#ifdef STELNETTTS_BUILD
#define STELNETTTS_SESSION_API __declspec(dllexport)
#else
#define STELNETTTS_SESSION_API __declspec(dllimport)
#endif
#else
#define STELNETTTS_SESSION_API __attribute__((visibility("default")))
#endif
#else
#define STELNETTTS_SESSION_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Whisper types (defined in whisper.h, forward-declared here so this
// header is self-contained without pulling in the full whisper API).
struct whisper_context;
struct whisper_context_params;
struct whisper_full_params;

struct stelnettts_align_result;
typedef struct stelnettts_align_result stelnettts_align_result;
struct stelnettts_diarize_opts_abi;
typedef struct stelnettts_diarize_opts_abi stelnettts_diarize_opts_abi;
struct stelnettts_diarize_seg_abi;
typedef struct stelnettts_diarize_seg_abi stelnettts_diarize_seg_abi;
struct stelnettts_open_params_v1;
typedef struct stelnettts_open_params_v1 stelnettts_open_params_v1;
struct stelnettts_session;
typedef struct stelnettts_session stelnettts_session;
struct stelnettts_session_result;
typedef struct stelnettts_session_result stelnettts_session_result;
struct stelnettts_stream;
typedef struct stelnettts_stream stelnettts_stream;
struct stelnettts_vad_abi_opts;
typedef struct stelnettts_vad_abi_opts stelnettts_vad_abi_opts;
struct parakeet_context;
typedef struct parakeet_context parakeet_context;
struct parakeet_result;
typedef struct parakeet_result parakeet_result;
struct whisper_context;
typedef struct whisper_context whisper_context;
struct whisper_context_params;
typedef struct whisper_context_params whisper_context_params;

STELNETTTS_SESSION_API int stelnettts_get_progress(void);
STELNETTTS_SESSION_API void stelnettts_reset_progress(void);

// 0.10.3+ (issue #208): per-session progress callback for long-form
// (chunked) transcription. Invoked once per finished window from within
// stelnettts_session_transcribe_chunked[_lang] (and any auto-chunked long
// Parakeet transcribe) with the number of input samples processed so far
// and the total. `processed` is monotonically non-decreasing and ends at
// `total`. It is invoked on the calling (transcribe) thread — keep the
// callback fast and non-blocking; do not re-enter the session from it.
// Single-pass and non-Parakeet backends do not fire it. The module-level
// atomic (stelnettts_get_progress) is updated in lockstep, so pure pollers
// (e.g. Dart FFI) get chunked progress without registering a callback.
typedef void (*stelnettts_progress_callback)(int processed, int total, void* user_data);

// Register (or clear, with cb == NULL) the session progress callback.
// user_data is passed back verbatim to every invocation. The pointer is
// stored, not copied; keep it valid for the duration of transcribe calls.
STELNETTTS_SESSION_API void stelnettts_session_set_progress_callback(stelnettts_session* s, stelnettts_progress_callback cb,
                                                                 void* user_data);

/// Per-segment streaming callback. Fired each time a new segment is
/// committed during transcription. The segment text and timing are
/// passed directly — the callback must copy any data it needs (the
/// pointers are valid only for the duration of the call).
typedef void (*stelnettts_segment_callback)(const char* text,  // segment text (UTF-8, null-terminated)
                                          int64_t t0_cs,     // start time in centiseconds
                                          int64_t t1_cs,     // end time in centiseconds
                                          int segment_index, // 0-based segment index within this transcription
                                          void* user_data    // opaque pointer from registration
);

/// Register a per-segment streaming callback on the session.
/// Pass NULL to clear. The callback is invoked on the transcription
/// thread — it must be fast and non-blocking.
STELNETTTS_SESSION_API void stelnettts_session_set_segment_callback(stelnettts_session* s, stelnettts_segment_callback cb,
                                                                void* user_data);

/// Number of streamed segments available for polling (Dart FFI path).
/// The polling buffers are per-session; this session-less API reads the
/// session that most recently streamed via the default callbacks.
STELNETTTS_SESSION_API int stelnettts_get_streamed_segment_count(void);

/// Drain all buffered streamed segments into a new result. Caller owns the
/// returned pointer (free with stelnettts_session_result_free). Returns NULL
/// when the buffer is empty.
STELNETTTS_SESSION_API stelnettts_session_result* stelnettts_drain_streamed_segments(void);

/// Clear the streamed-segment buffer. Call before starting a new
/// transcription to discard stale segments from the previous run.
STELNETTTS_SESSION_API void stelnettts_reset_streamed_segments(void);

/// Per-token streaming callback. Fired each time the decoder produces a
/// new text token during LLM-based ASR. The token text may be a partial
/// word (BPE subword). The callback must be fast and non-blocking.
typedef void (*stelnettts_token_callback)(const char* token_text, // decoded token text (UTF-8)
                                        int token_index,        // 0-based token index within current segment
                                        void* user_data);

STELNETTTS_SESSION_API void stelnettts_session_set_token_callback(stelnettts_session* s, stelnettts_token_callback cb,
                                                              void* user_data);

/// Number of streamed tokens available for polling (Dart FFI path).
STELNETTTS_SESSION_API int stelnettts_get_streamed_token_count(void);

/// Drain all buffered streamed tokens. Returns a buffer of null-separated
/// UTF-8 strings; *out_count receives the number of tokens. Returns NULL
/// when the buffer is empty. The returned pointer is valid until the next
/// drain call on the same session, or until the session is closed.
STELNETTTS_SESSION_API const char* stelnettts_drain_streamed_tokens(int* out_count);

/// Clear the streamed-token buffer.
STELNETTTS_SESSION_API void stelnettts_reset_streamed_tokens(void);

STELNETTTS_SESSION_API void stelnettts_params_set_language(whisper_full_params* p, const char* lang);
STELNETTTS_SESSION_API void stelnettts_params_set_translate(whisper_full_params* p, int v);
STELNETTTS_SESSION_API void stelnettts_params_set_detect_language(whisper_full_params* p, int v);
STELNETTTS_SESSION_API void stelnettts_params_set_token_timestamps(whisper_full_params* p, int v);
STELNETTTS_SESSION_API void stelnettts_params_set_n_threads(whisper_full_params* p, int n);
STELNETTTS_SESSION_API void stelnettts_params_set_max_len(whisper_full_params* p, int n);
STELNETTTS_SESSION_API void stelnettts_params_set_best_of(whisper_full_params* p, int n);
STELNETTTS_SESSION_API void stelnettts_params_set_split_on_word(whisper_full_params* p, int v);
STELNETTTS_SESSION_API void stelnettts_params_set_no_context(whisper_full_params* p, int v);
STELNETTTS_SESSION_API void stelnettts_params_set_single_segment(whisper_full_params* p, int v);
STELNETTTS_SESSION_API void stelnettts_params_set_print_realtime(whisper_full_params* p, int v);
STELNETTTS_SESSION_API void stelnettts_params_set_print_progress(whisper_full_params* p, int v);
STELNETTTS_SESSION_API void stelnettts_params_set_print_timestamps(whisper_full_params* p, int v);
STELNETTTS_SESSION_API void stelnettts_params_set_print_special(whisper_full_params* p, int v);
STELNETTTS_SESSION_API void stelnettts_params_set_suppress_blank(whisper_full_params* p, int v);
STELNETTTS_SESSION_API void stelnettts_params_set_temperature(whisper_full_params* p, float t);
STELNETTTS_SESSION_API void stelnettts_params_set_max_tokens(whisper_full_params* p, int n);
STELNETTTS_SESSION_API void stelnettts_params_set_initial_prompt(whisper_full_params* p, const char* prompt);
STELNETTTS_SESSION_API void stelnettts_params_set_vad(whisper_full_params* p, int v);
STELNETTTS_SESSION_API void stelnettts_params_set_vad_model_path(whisper_full_params* p, const char* path);
STELNETTTS_SESSION_API void stelnettts_params_set_vad_threshold(whisper_full_params* p, float t);
STELNETTTS_SESSION_API void stelnettts_params_set_vad_min_speech_ms(whisper_full_params* p, int ms);
STELNETTTS_SESSION_API void stelnettts_params_set_vad_min_silence_ms(whisper_full_params* p, int ms);
STELNETTTS_SESSION_API void stelnettts_params_set_tdrz(whisper_full_params* p, int v);
STELNETTTS_SESSION_API void stelnettts_ctx_params_set_dtw(whisper_context_params* p, bool enable, int aheads_preset,
                                                      int n_top);
STELNETTTS_SESSION_API int64_t stelnettts_token_t0(whisper_context* ctx, int i_seg, int i_tok);
STELNETTTS_SESSION_API int64_t stelnettts_token_t1(whisper_context* ctx, int i_seg, int i_tok);
STELNETTTS_SESSION_API float stelnettts_token_p(whisper_context* ctx, int i_seg, int i_tok);
STELNETTTS_SESSION_API int64_t stelnettts_token_dtw_t(whisper_context* ctx, int i_segment, int i_token);
STELNETTTS_SESSION_API void stelnettts_params_set_alt_n(whisper_full_params* p, int n);
STELNETTTS_SESSION_API int stelnettts_token_n_alts(whisper_context* ctx, int i_seg, int i_tok);
STELNETTTS_SESSION_API int32_t stelnettts_token_alt_id(whisper_context* ctx, int i_seg, int i_tok, int i_alt);
STELNETTTS_SESSION_API float stelnettts_token_alt_p(whisper_context* ctx, int i_seg, int i_tok, int i_alt);
STELNETTTS_SESSION_API int stelnettts_token_alt_text(whisper_context* ctx, int i_seg, int i_tok, int i_alt, char* out,
                                                 int out_cap);
STELNETTTS_SESSION_API float stelnettts_detect_language(whisper_context* ctx, const float* pcm, int n_samples,
                                                    int n_threads, char* out_code, int out_cap);
STELNETTTS_SESSION_API int stelnettts_vad_segments(const char* vad_model_path, const float* pcm, int n_samples,
                                               int sample_rate, float threshold, int min_speech_ms, int min_silence_ms,
                                               int n_threads, bool use_gpu, float** out_spans);
// Returns the slice count (>= 0), or negative on error: -1 bad arguments,
// -2 allocation failed, -3 the VAD model could not be loaded. -3 matters
// because 0 ("loaded fine, found no speech") used to be the answer for a
// missing model too, which made every binding read a broken install as silence.
STELNETTTS_SESSION_API int stelnettts_vad_slices(const char* vad_model_path, const float* pcm, int n_samples,
                                             int sample_rate, float threshold, int min_speech_ms, int min_silence_ms,
                                             int speech_pad_ms, float max_chunk_duration_s, int n_threads,
                                             float** out_spans);
STELNETTTS_SESSION_API void stelnettts_vad_free(float* spans);
STELNETTTS_SESSION_API int stelnettts_watermark_load_model(const char* gguf_path);
STELNETTTS_SESSION_API float stelnettts_watermark_detect(const float* pcm, int n_samples);
STELNETTTS_SESSION_API void stelnettts_watermark_embed(float* pcm, int n_samples, float alpha);
STELNETTTS_SESSION_API int stelnettts_lcs_dedup_prefix_count(const int32_t* prev_tail_tokens, int n_prev,
                                                         const int32_t* curr_tokens, int n_curr, int min_lcs_length);
STELNETTTS_SESSION_API stelnettts_stream* stelnettts_stream_open(whisper_context* ctx, int n_threads, int step_ms,
                                                           int length_ms, int keep_ms, const char* language,
                                                           int translate);
STELNETTTS_SESSION_API void stelnettts_stream_close(stelnettts_stream* s);
STELNETTTS_SESSION_API int stelnettts_stream_feed(stelnettts_stream* s, const float* pcm, int n_samples);
STELNETTTS_SESSION_API int stelnettts_stream_get_text(stelnettts_stream* s, char* out_text, int out_cap, double* out_t0_s,
                                                  double* out_t1_s, int64_t* out_counter);
STELNETTTS_SESSION_API int stelnettts_stream_flush(stelnettts_stream* s);
STELNETTTS_SESSION_API void stelnettts_stream_set_live_decode(stelnettts_stream* s, int enabled);
STELNETTTS_SESSION_API parakeet_context* stelnettts_parakeet_init(const char* model_path, int n_threads, int use_flash);
STELNETTTS_SESSION_API void stelnettts_parakeet_free(parakeet_context* ctx);
STELNETTTS_SESSION_API parakeet_result* stelnettts_parakeet_transcribe(parakeet_context* ctx, const float* pcm,
                                                                   int n_samples, int64_t t_offset_cs);
STELNETTTS_SESSION_API const char* stelnettts_parakeet_result_text(parakeet_result* r);
STELNETTTS_SESSION_API int stelnettts_parakeet_result_n_words(parakeet_result* r);
STELNETTTS_SESSION_API const char* stelnettts_parakeet_result_word_text(parakeet_result* r, int i);
STELNETTTS_SESSION_API int64_t stelnettts_parakeet_result_word_t0(parakeet_result* r, int i);
STELNETTTS_SESSION_API int64_t stelnettts_parakeet_result_word_t1(parakeet_result* r, int i);
STELNETTTS_SESSION_API int stelnettts_parakeet_result_n_tokens(parakeet_result* r);
STELNETTTS_SESSION_API const char* stelnettts_parakeet_result_token_text(parakeet_result* r, int i);
STELNETTTS_SESSION_API int64_t stelnettts_parakeet_result_token_t0(parakeet_result* r, int i);
STELNETTTS_SESSION_API int64_t stelnettts_parakeet_result_token_t1(parakeet_result* r, int i);
STELNETTTS_SESSION_API float stelnettts_parakeet_result_token_p(parakeet_result* r, int i);
STELNETTTS_SESSION_API void stelnettts_parakeet_result_free(parakeet_result* r);
// Issue #214: set the preferred GPU backend name ("cuda", "vulkan",
// "metal"). Call before any stelnettts_session_open*. NULL or "" = auto.
STELNETTTS_SESSION_API void stelnettts_set_gpu_backend(const char* name);

STELNETTTS_SESSION_API int stelnettts_detect_backend_from_gguf(const char* path, char* out_name, int out_cap);
STELNETTTS_SESSION_API stelnettts_session* stelnettts_session_open_explicit(const char* model_path, const char* backend_name,
                                                                      int n_threads);
STELNETTTS_SESSION_API stelnettts_session* stelnettts_session_open(const char* model_path, int n_threads);
STELNETTTS_SESSION_API stelnettts_session* stelnettts_session_open_with_params(const char* model_path,
                                                                         const char* backend_name,
                                                                         const stelnettts_open_params_v1* params);
STELNETTTS_SESSION_API const char* stelnettts_session_backend(stelnettts_session* s);
STELNETTTS_SESSION_API int stelnettts_session_available_backends(char* out_csv, int out_cap);
// Acoustic language detected by the last transcribe, as an ISO-639-1 code
// (whisper only; other backends fall back to the source-language hint, then
// "unknown"). Writes into out_buf (NUL-terminated, truncated to out_cap) and
// returns the code length in bytes, or -1 on bad args. Distinct from the
// text-LID pass stelnettts_text_detect_language.
STELNETTTS_SESSION_API int stelnettts_session_detected_language(stelnettts_session* s, char* out_buf, int out_cap);
// CTC vocabulary access (Omni CTC backend). stelnettts_session_n_vocab returns
// the number of SentencePiece pieces in the loaded model (0 for backends that
// don't expose a CTC vocab); stelnettts_session_token_text maps a token id in
// [0, n_vocab) to its raw piece (U+2581 word-boundary marker intact), or ""
// when out of range or unsupported. Pairs with stelnettts_session_result_logits
// to detokenize a greedy CTC decode.
STELNETTTS_SESSION_API int stelnettts_session_n_vocab(stelnettts_session* s);
STELNETTTS_SESSION_API const char* stelnettts_session_token_text(stelnettts_session* s, int id);
STELNETTTS_SESSION_API stelnettts_session_result* stelnettts_session_transcribe_lang(stelnettts_session* s, const float* pcm,
                                                                               int n_samples, const char* language);
STELNETTTS_SESSION_API stelnettts_session_result* stelnettts_session_transcribe(stelnettts_session* s, const float* pcm,
                                                                          int n_samples);
// 0.8.7+: chunked-encode transcribe (issue #208). Forces the Parakeet
// backend through its bounded long-form path (overlapping short-window
// transcribe-and-merge for non-JA models, streamed encoder for the JA-only
// model) regardless of audio length, so long files transcribe in bounded
// time AND recover the sections a single full-length pass drops.
// `chunk_seconds <= 0` keeps the per-model defaults; otherwise it sets the
// non-JA window length / the JA streamed window. `overlap_seconds < 0`
// uses the default. For non-Parakeet backends the chunk params are inert
// and this behaves exactly like stelnettts_session_transcribe[_lang].
//
// 0.8.29+ (issue #350): `chunk_seconds = 0` is "per-model defaults", NOT
// "no chunking" — reaching this entry point at all is a request for bounded
// long-form, so the non-JA single-pass cap drops from 300 s to the decoder's
// reliable ~30 s window for this call. Between 0.8.24 and 0.8.28 the unified
// dispatch collapsed the two, and such a call took ONE full-length decode
// that silently dropped whole spans of speech.
STELNETTTS_SESSION_API stelnettts_session_result* stelnettts_session_transcribe_chunked_lang(
    stelnettts_session* s, const float* pcm, int n_samples, int chunk_seconds, int overlap_seconds, const char* language);
STELNETTTS_SESSION_API stelnettts_session_result* stelnettts_session_transcribe_chunked(stelnettts_session* s, const float* pcm,
                                                                                  int n_samples, int chunk_seconds,
                                                                                  int overlap_seconds);
STELNETTTS_SESSION_API stelnettts_session_result* stelnettts_session_transcribe_vad_lang(
    stelnettts_session* s, const float* pcm, int n_samples, int sample_rate, const char* vad_model_path,
    const stelnettts_vad_abi_opts* opts_or_null, const char* language);
STELNETTTS_SESSION_API stelnettts_session_result* stelnettts_session_transcribe_vad(
    stelnettts_session* s, const float* pcm, int n_samples, int sample_rate, const char* vad_model_path,
    const stelnettts_vad_abi_opts* opts_or_null);
STELNETTTS_SESSION_API int stelnettts_diarize_segments_abi(const float* left_pcm, const float* right_pcm, int32_t n_samples,
                                                       int32_t is_stereo, stelnettts_diarize_seg_abi* segs,
                                                       int32_t n_segs, const stelnettts_diarize_opts_abi* opts);
STELNETTTS_SESSION_API int stelnettts_detect_language_pcm(const float* samples, int32_t n_samples, int32_t method,
                                                      const char* model_path, int32_t n_threads, int32_t use_gpu,
                                                      int32_t gpu_device, int32_t flash_attn, char* out_lang_buf,
                                                      int32_t out_lang_cap, float* out_confidence);
STELNETTTS_SESSION_API int stelnettts_enhance_audio_rnnoise(const float* in_pcm, int32_t n_samples, float* out_pcm,
                                                        int32_t out_cap);
STELNETTTS_SESSION_API int stelnettts_text_detect_language(const char* text, const char* model_path, int32_t n_threads,
                                                       char* out_label_buf, int32_t out_label_cap,
                                                       float* out_confidence);
STELNETTTS_SESSION_API stelnettts_align_result* stelnettts_align_words_abi(const char* aligner_model, const char* transcript,
                                                                     const float* samples, int32_t n_samples,
                                                                     int64_t t_offset_cs, int32_t n_threads);
STELNETTTS_SESSION_API int stelnettts_align_result_n_words(stelnettts_align_result* r);
STELNETTTS_SESSION_API const char* stelnettts_align_result_word_text(stelnettts_align_result* r, int i);
STELNETTTS_SESSION_API int64_t stelnettts_align_result_word_t0(stelnettts_align_result* r, int i);
STELNETTTS_SESSION_API int64_t stelnettts_align_result_word_t1(stelnettts_align_result* r, int i);
STELNETTTS_SESSION_API void stelnettts_align_result_free(stelnettts_align_result* r);
STELNETTTS_SESSION_API int stelnettts_cache_ensure_file_abi(const char* filename, const char* url, int32_t quiet,
                                                        const char* cache_dir_override, char* out_buf, int32_t out_cap);
STELNETTTS_SESSION_API int stelnettts_cache_dir_abi(const char* cache_dir_override, char* out_buf, int32_t out_cap);
STELNETTTS_SESSION_API int stelnettts_registry_lookup_abi(const char* backend, char* out_filename, int32_t filename_cap,
                                                      char* out_url, int32_t url_cap, char* out_size, int32_t size_cap);
STELNETTTS_SESSION_API int stelnettts_registry_lookup_by_filename_abi(const char* filename, char* out_filename,
                                                                  int32_t filename_cap, char* out_url, int32_t url_cap,
                                                                  char* out_size, int32_t size_cap);
STELNETTTS_SESSION_API int stelnettts_registry_list_backends_abi(char* out_csv, int32_t out_cap);
typedef enum stelnettts_registry_artifact_kind {
    STELNETTTS_REGISTRY_ARTIFACT_PRIMARY = 0,
    STELNETTTS_REGISTRY_ARTIFACT_COMPANION = 1,
    STELNETTTS_REGISTRY_ARTIFACT_EXTRA = 2,
} stelnettts_registry_artifact_kind;
// Return the artifact count for a backend's exact `-m auto` default bundle,
// or 0 when the backend is unknown. On success, also writes the canonical
// backend key and registry licence (which may be empty). Negative values are
// argument/buffer errors.
STELNETTTS_SESSION_API int stelnettts_registry_default_bundle_info_abi(const char* backend, char* out_backend,
                                                                   int32_t backend_cap, char* out_license,
                                                                   int32_t license_cap,
                                                                   int32_t* out_requires_acceptance);
// Return one default-bundle artifact by index. 0 = success, 1 = unknown
// backend/index, -1 = invalid arguments, 2 = an output buffer is too small.
STELNETTTS_SESSION_API int stelnettts_registry_default_bundle_artifact_abi(const char* backend, int32_t index,
                                                                       int32_t* out_kind, char* out_filename,
                                                                       int32_t filename_cap, char* out_url,
                                                                       int32_t url_cap, char* out_size,
                                                                       int32_t size_cap);
STELNETTTS_SESSION_API int stelnettts_session_result_n_segments(stelnettts_session_result* r);
STELNETTTS_SESSION_API const char* stelnettts_session_result_segment_text(stelnettts_session_result* r, int i);
STELNETTTS_SESSION_API int64_t stelnettts_session_result_segment_t0(stelnettts_session_result* r, int i);
STELNETTTS_SESSION_API int64_t stelnettts_session_result_segment_t1(stelnettts_session_result* r, int i);
// #300: native per-segment speaker label from a backend that diarizes on its
// own — the "(Speaker N) " form (with the trailing space, as the CLI prefixes
// it), or "" when the backend produced none. Never NULL, so a caller can print
// it unconditionally. Populated today by vibevoice, whose model answers with a
// Start/End/Speaker/Content array; other backends leave it empty. The ordinals
// are CHUNK-LOCAL: "Speaker 1" in one transcribe call is not guaranteed to be
// the same voice as "Speaker 1" in the next, since no cross-chunk clustering
// runs here (use stelnettts_diarize_* for that).
STELNETTTS_SESSION_API const char* stelnettts_session_result_segment_speaker(stelnettts_session_result* r, int i);
STELNETTTS_SESSION_API int stelnettts_session_result_n_words(stelnettts_session_result* r, int i_seg);
STELNETTTS_SESSION_API const char* stelnettts_session_result_word_text(stelnettts_session_result* r, int i_seg, int i_word);
STELNETTTS_SESSION_API int64_t stelnettts_session_result_word_t0(stelnettts_session_result* r, int i_seg, int i_word);
STELNETTTS_SESSION_API int64_t stelnettts_session_result_word_t1(stelnettts_session_result* r, int i_seg, int i_word);
STELNETTTS_SESSION_API float stelnettts_session_result_word_p(stelnettts_session_result* r, int i_seg, int i_word);
// Whisper's per-segment no-speech probability (the <|nospeech|> token
// posterior) in [0, 1]. Only the whisper backend populates it; other backends
// and out-of-range indices return the -1.0 sentinel ("no data").
STELNETTTS_SESSION_API float stelnettts_session_result_segment_no_speech_prob(stelnettts_session_result* r, int i_seg);
// Per-frame CTC logits (opted in via stelnettts_session_set_return_logits) for
// backends that produce a dense CTC grid (Omni CTC, wav2vec2/hubert/data2vec,
// canary-ctc). Frame-major: logits[t * n_logit_vocab + v]. Raw pre-softmax for
// Omni & wav2vec2; log-probabilities for canary-ctc. _logits returns NULL when
// none captured.
STELNETTTS_SESSION_API int stelnettts_session_result_n_logit_frames(stelnettts_session_result* r);
STELNETTTS_SESSION_API int stelnettts_session_result_n_logit_vocab(stelnettts_session_result* r);
STELNETTTS_SESSION_API const float* stelnettts_session_result_logits(stelnettts_session_result* r);
STELNETTTS_SESSION_API int stelnettts_session_result_word_n_alts(stelnettts_session_result* r, int i_seg, int i_word);
STELNETTTS_SESSION_API const char* stelnettts_session_result_word_alt_text(stelnettts_session_result* r, int i_seg,
                                                                       int i_word, int i_alt);
STELNETTTS_SESSION_API float stelnettts_session_result_word_alt_p(stelnettts_session_result* r, int i_seg, int i_word,
                                                              int i_alt);
STELNETTTS_SESSION_API void stelnettts_session_result_free(stelnettts_session_result* r);
// Issue #257: parakeet/canary local-attention window in encoder frames (~80 ms
// each) — NeMo change_attention_model("rel_pos_local_attn", [left, right]).
// Bounds long-audio encoder memory to O(T·window) instead of O(T²). Negative
// values = full attention; INT_MIN,INT_MIN = clear (use the model default).
// No-op for non-parakeet backends. Returns 0 on success.
STELNETTTS_SESSION_API int stelnettts_session_set_parakeet_att_context(stelnettts_session* s, int left, int right);
// Set the active backend's companion codec/tokenizer model. For OmniVoice this
// is its HiggsAudioV2 tokenizer; for Chatterbox it is S3Gen.
STELNETTTS_SESSION_API int stelnettts_session_set_codec_path(stelnettts_session* s, const char* path);
// Set the active TTS backend's voice from its native format. Chatterbox accepts
// a conditioning GGUF or reference WAV; OmniVoice accepts a reference WAV and
// uses ref_text_or_null as its transcript. Returns -3 when the active backend
// has no voice-setting implementation.
STELNETTTS_SESSION_API int stelnettts_session_set_voice(stelnettts_session* s, const char* path,
                                                    const char* ref_text_or_null);
// #201: configure the TADA encoder + aligner GGUFs used for on-the-fly voice
// cloning, i.e. stelnettts_session_set_voice(s, "ref.wav", "<transcript>") on a
// TADA session. The .wav clone path is opt-in (experimental) — enable it with
// the env var STELNETTTS_TADA_WAV_CLONE=1; otherwise a .wav voice is rejected as
// before. Either path may be NULL to clear it and fall back to
// auto-resolution (next to the model, then the cache dir). The aligner is
// language-specific (tada-aligner-<lang>.gguf); the language follows the
// session's source-language hint. No-op (returns -1) for non-TADA sessions.
STELNETTTS_SESSION_API int stelnettts_session_tada_set_makeref_models(stelnettts_session* s, const char* encoder_gguf,
                                                                  const char* aligner_gguf);
STELNETTTS_SESSION_API int stelnettts_session_set_speaker_name(stelnettts_session* s, const char* name);
STELNETTTS_SESSION_API int stelnettts_session_set_speaker_id(stelnettts_session* s, int id);
STELNETTTS_SESSION_API int stelnettts_session_n_speakers(stelnettts_session* s);
STELNETTTS_SESSION_API const char* stelnettts_session_get_speaker_name(stelnettts_session* s, int i);
STELNETTTS_SESSION_API int stelnettts_session_set_instruct(stelnettts_session* s, const char* instruct);
// #316: synthesize `phonemes` verbatim instead of phonemizing the text — the
// seam between text processing and the acoustic model. Use it to reproduce
// another implementation's pronunciation exactly, or to separate "the G2P is
// wrong" from "the model is wrong". Empty clears. Returns -2 (soft no-op) when
// the active backend exposes no phonemes-in call; kokoro and piper do.
STELNETTTS_SESSION_API int stelnettts_session_set_tts_phonemes(stelnettts_session* s, const char* phonemes);
STELNETTTS_SESSION_API int stelnettts_session_is_custom_voice(stelnettts_session* s);
STELNETTTS_SESSION_API int stelnettts_session_is_voice_design(stelnettts_session* s);
// UNMARKED synthesis — hard-refused unless stelnettts_session_accept_marking_responsibility() was called first.
STELNETTTS_SESSION_API float* stelnettts_session_synthesize_raw(stelnettts_session* s, const char* text, int* out_n_samples);
STELNETTTS_SESSION_API float* stelnettts_session_synthesize(stelnettts_session* s, const char* text, int* out_n_samples);
// Attest acceptance of AI-content marking/disclosure duty (required for _raw); recorded for audit.
STELNETTTS_SESSION_API int stelnettts_session_accept_marking_responsibility(stelnettts_session* s, const char* attestation);
// Declare whose voice the current PRESET voice is: "real_person" | "synthetic" |
// "unknown". A preset can be an identifiable individual (a named donor, a corpus
// speaker), which makes its output a deep fake under Art. 3(60) even though no
// recording passed through a baker — so "not a clone" is not the same as
// "nothing to disclose". Setting real_person makes the Art. 50(4) reminder fire
// for a non-cloned voice; it does NOT require a consent attestation, because the
// donor's agreement to the training is settled upstream and you cannot attest to
// it. Returns 0, -1 on a bad session, -2 on an unrecognised value.
STELNETTTS_SESSION_API int stelnettts_session_set_speaker_identity(stelnettts_session* s, const char* identity);

// ─── Spoken AI-disclosure for voice clones (EU AI Act Art. 50(4)) ──────
//
// synthesize() watermarks every clip, which discharges the machine-readable
// marking duty (Art. 50(2)). Art. 50(4) additionally requires a VISIBLE OR
// AUDIBLE disclosure when the output is a deepfake — a watermark alone does not
// satisfy it. The CLI and the HTTP server prepend a spoken disclaimer to cloned
// output automatically; the ABI does not, because the neutral-voice synthesis
// that requires cannot be done portably once a clone voice is applied to the
// backend. On the ABI that duty is yours, and these two calls are the tools.
//
// Synthesizing with a clone voice set and no accept_marking_responsibility()
// logs a one-time [MARKING] warning rather than refusing.

// The canonical disclosure string, identical to the CLI's. Static; never NULL.
// Use it for a visible label — Art. 50(5) requires disclosures to meet
// accessibility requirements, and audio-only is not accessible to a deaf user.
STELNETTTS_SESSION_API const char* stelnettts_session_disclaimer_text(void);

// The disclosure synthesized in this session's NEUTRAL voice, for you to
// prepend to cloned output. MUST be called BEFORE stelnettts_session_set_voice()
// installs a cloning voice — it returns NULL afterwards (see
// stelnettts_session_last_synth_error), because a disclaimer spoken in the cloned voice
// would make the output more deceptive rather than less. Caller owns the
// buffer; free with stelnettts_pcm_free().
STELNETTTS_SESSION_API float* stelnettts_session_get_disclaimer_pcm(stelnettts_session* s, int* out_n_samples);

// Streaming synthesis: fires `cb` once per sentence chunk with that chunk's
// watermarked PCM (backend-native sample rate, same as synthesize) as it is
// produced. The PCM is owned by the call and freed after `cb` returns — copy
// it if you need to keep it. `is_final` is 1 on the last chunk. Returns 0 on
// success, -1 on bad args.
typedef void (*stelnettts_pcm_stream_cb)(const float* pcm, int n_samples, int is_final, void* user_data);
STELNETTTS_SESSION_API int stelnettts_session_synthesize_streaming(stelnettts_session* s, const char* text,
                                                               stelnettts_pcm_stream_cb cb, void* user_data);

STELNETTTS_SESSION_API void stelnettts_pcm_free(float* pcm);
STELNETTTS_SESSION_API float* stelnettts_session_speech_to_speech(stelnettts_session* s, const float* in_samples,
                                                              int n_in_samples, char** out_text, int* out_n_samples);
STELNETTTS_SESSION_API int stelnettts_session_set_hotwords(stelnettts_session* s, const char* hotwords, float boost);

// Source separation: split audio into N stems (drums, bass, other, vocals).
// Input: stereo interleaved PCM at the model's native rate (44100 Hz for htdemucs).
// Returns stem count (>0) on success, -1 on error. Call stelnettts_session_separate_stem
// to retrieve individual stem PCM after a successful separate call.
STELNETTTS_SESSION_API int stelnettts_session_separate(stelnettts_session* s, const float* pcm_stereo, int n_samples);
STELNETTTS_SESSION_API int stelnettts_session_separate_n_stems(stelnettts_session* s);
STELNETTTS_SESSION_API const char* stelnettts_session_separate_stem_name(stelnettts_session* s, int stem_idx);
// Returns pointer to interleaved stereo PCM for stem_idx. Owned by the session;
// valid until the next separate() call or session close. *out_n_samples receives
// the per-channel sample count.
STELNETTTS_SESSION_API const float* stelnettts_session_separate_stem(stelnettts_session* s, int stem_idx, int* out_n_samples);
STELNETTTS_SESSION_API int stelnettts_session_separate_sample_rate(stelnettts_session* s);

// Pitch (F0) estimation: monophonic pitch track from mono PCM at the model's
// native rate (16000 Hz for crepe). `hop_ms` <= 0 uses the model default (10 ms).
// Returns the frame count (>0) on success, -1 on error. Frames stay valid until
// the next pitch() call or session close.
STELNETTTS_SESSION_API int stelnettts_session_pitch(stelnettts_session* s, const float* pcm_16k, int n_samples, float hop_ms);
STELNETTTS_SESSION_API int stelnettts_session_pitch_n_frames(stelnettts_session* s);
// Single frame by index. Any out_* pointer may be NULL. Returns 0 on success.
STELNETTTS_SESSION_API int stelnettts_session_pitch_frame(stelnettts_session* s, int idx, float* out_time_ms,
                                                      float* out_f0_hz, float* out_voiced_prob);
// Flat view of every frame: 3 floats per frame {time_ms, f0_hz, voiced_prob},
// frame-major. Owned by the session. *out_n_frames receives the frame count.
STELNETTTS_SESSION_API const float* stelnettts_session_pitch_frames(stelnettts_session* s, int* out_n_frames);
STELNETTTS_SESSION_API int stelnettts_session_pitch_sample_rate(stelnettts_session* s);

// Voice conversion (SVC, RVC). Input is CONTENTVEC FEATURES, not audio: the
// consumer owns the content encoder, which is why this has no CLI verb.
// `content` is n_frames * content_dim frame-major; `f0_hz` is n_frames values
// in Hz with 0.0 marking unvoiced (the coarse mel-quantised pitch is derived
// internally — those constants are model-side and replicating them in the
// caller guarantees drift). Returns the sample count (>0), -1 on error.
//
// STOCHASTIC BY DESIGN. Pass NULL for both noise buffers in production. Passing
// explicit buffers replays a specific draw, which is the only way to compare
// against another implementation — waveform correlation against a reference run
// is invalid here because the reference disagrees with itself.
//   noise_zp   : inter_channels * n_frames, or NULL
//   noise_sine : n_frames * upsample_product, or NULL
STELNETTTS_SESSION_API int stelnettts_session_convert(stelnettts_session* s, const float* content, int n_frames,
                                                  const float* f0_hz, int speaker_id, const float* noise_zp,
                                                  const float* noise_sine);
// Session-owned mono PCM from the last convert(), at convert_sample_rate().
STELNETTTS_SESSION_API const float* stelnettts_session_convert_audio(stelnettts_session* s, int* out_n_samples);
// 256 (v1, layer 9 + final_proj) or 768 (v2, final layer). Check this against
// your encoder before calling: a v1/v2 mismatch is silent otherwise.
STELNETTTS_SESSION_API int stelnettts_session_convert_content_dim(stelnettts_session* s);
STELNETTTS_SESSION_API int stelnettts_session_convert_n_speakers(stelnettts_session* s);
// The checkpoint's native rate (32k/40k/48k) — not a constant.
STELNETTTS_SESSION_API int stelnettts_session_convert_sample_rate(stelnettts_session* s);

// Chord recognition: mono PCM at any rate (resampled internally to the
// model's 22050 Hz) -> a chord timeline. Returns the span count (>0) on
// success, 0 for "ran, found nothing", -1 on error or a backend with no
// chord arm.
//
// NOTE ON WEIGHTS: the shipped BTC weights are CC-BY-NC-SA (trained on
// Isophonics et al.), so they are NOT licensed for commercial use even though
// this library is MIT. The registry refuses to download them without an
// explicit licence acceptance (CLI: --accept-license cc-by-nc-sa-4.0; env:
// STELNETTTS_ACCEPT_LICENSE). A commercial product must ship its own weights.
// --- Guitar tablature (tabcnn) -------------------------------------------
//
// EMISSION SCORES, not a decided tablature. stelnettts_session_tab() runs the
// model and returns the frame count; stelnettts_session_tab_emissions() hands
// back a flat [frame][string][class] grid of LOG-probabilities, valid until the
// next call or session close. The constrained Viterbi/DP that turns those into
// a playable fingering (one note per string, fret range, capo, hand span) is
// yours — argmaxing this grid ignores every playability constraint.
//
// Weights are CC BY 4.0 (EGSet12, https://zenodo.org/records/11406378): attribution
// required when redistributing.
STELNETTTS_SESSION_API int stelnettts_session_tab(stelnettts_session* s, const float* pcm, int n_samples, int sample_rate);
STELNETTTS_SESSION_API int stelnettts_session_tab_n_frames(stelnettts_session* s);
STELNETTTS_SESSION_API const float* stelnettts_session_tab_emissions(stelnettts_session* s, int* out_n_frames,
                                                                 int* out_n_strings, int* out_n_classes);
// Class index meaning "string not played" — read it, never assume it.
STELNETTTS_SESSION_API int stelnettts_session_tab_silent_class(stelnettts_session* s);
STELNETTTS_SESSION_API float stelnettts_session_tab_frame_period(stelnettts_session* s);
// Open-string MIDI pitch per string (0 = lowest), or -1 if unknown.
STELNETTTS_SESSION_API int stelnettts_session_tab_string_open_midi(stelnettts_session* s, int string);

STELNETTTS_SESSION_API int stelnettts_session_chords(stelnettts_session* s, const float* pcm, int n_samples, int sample_rate);
STELNETTTS_SESSION_API int stelnettts_session_chords_n_spans(stelnettts_session* s);
// Flat, session-owned view of the last result: 4 floats per span, span-major,
// as {start_ms, end_ms, label, confidence}. Valid until the next
// stelnettts_session_chords call or session close. `label` is an index into the
// vocabulary and is a float for the same reason piano_notes' midi_note is —
// a mixed int/float struct misreads through a flat float view. Resolve it to
// a chord name with stelnettts_session_chords_span_name.
STELNETTTS_SESSION_API const float* stelnettts_session_chords_spans(stelnettts_session* s, int* out_n_spans);
// Chord name for span `idx`, e.g. "C", "Am", "G:7", or "N" for no-chord.
// Session-owned; NULL if idx is out of range.
STELNETTTS_SESSION_API const char* stelnettts_session_chords_span_name(stelnettts_session* s, int idx);
// 25 (maj/min + N) or 170 (full quality set). The shipped default is 170; set
// STELNETTTS_BTC_MAJ_MIN=1 to collapse the output to maj/min.
STELNETTTS_SESSION_API int stelnettts_session_chords_vocab_size(stelnettts_session* s);

// Beat and downbeat tracking: mono PCM at the model's native rate (22050 Hz
// for beat-this) -> a beat grid. Returns the beat count, or -1 on error, on a
// sample-rate mismatch, or on a backend with no beat arm.
//
// NO DBN. The postprocessing is peak-picking only, which is the point of the
// model: madmom's Dynamic Bayesian Network is Boeck-patented and licensed
// non-commercially, so a beat tracker that used one could not ship in a
// commercial product. beat-this is MIT for code AND weights and depends on no
// part of madmom, so unlike the chord arm above there is no licence gate here.
STELNETTTS_SESSION_API int stelnettts_session_beats(stelnettts_session* s, const float* pcm, int n_samples, int sample_rate);
STELNETTTS_SESSION_API int stelnettts_session_beats_n_events(stelnettts_session* s);
// Flat, session-owned view of the last result: 2 floats per beat, beat-major,
// as {time_s, is_downbeat}. Valid until the next stelnettts_session_beats call
// or session close. is_downbeat is 0.0f or 1.0f — a float, for the same reason
// chords' label is: a mixed int/float struct misreads through a flat view.
//
// EVERY DOWNBEAT IS ALSO A BEAT. The postprocessor snaps each downbeat onto
// its nearest beat, so the downbeats are a strict subset and callers never
// have to merge two lists to reconstruct the grid.
STELNETTTS_SESSION_API const float* stelnettts_session_beats_events(stelnettts_session* s, int* out_n_events);
// Median-interval tempo estimate in BPM from the last result, or 0 with fewer
// than two beats. Median rather than mean: a single missed or doubled beat
// skews a mean badly and both are routine.
STELNETTTS_SESSION_API float stelnettts_session_beats_tempo_bpm(stelnettts_session* s);
// Native input rate the loaded beat model expects, in Hz (22050 for
// beat-this), or 0 when the backend has no beat arm — so it doubles as a
// capability probe. stelnettts_session_beats REJECTS a mismatch rather than
// resampling: silently resampling audio would move every beat time.
STELNETTTS_SESSION_API int stelnettts_session_beats_sample_rate(stelnettts_session* s);

// Polyphonic piano transcription: mono PCM at the model's native rate
// (16000 Hz for piano-transcription) -> note events.
//
// Returns note count (>0) on success, 0 for "ran, found nothing", -1 on error
// or a backend with no piano arm. Retrieve the notes with
// stelnettts_session_piano_notes after a successful call.
//
// This exists so consumers get STRUCTURED note events. The CLI adapter renders
// each note into a stelnettts_segment whose text reads like "C4 v=80"; parsing
// that string back into a note is lossy and was never the intended seam.
STELNETTTS_SESSION_API int stelnettts_session_piano(stelnettts_session* s, const float* pcm_16k, int n_samples);
STELNETTTS_SESSION_API int stelnettts_session_piano_n_notes(stelnettts_session* s);
// Flat, session-owned view of the last result: 4 floats per note, note-major,
// as {onset_ms, offset_ms, midi_note, velocity}. Valid until the next
// stelnettts_session_piano call or session close — copy if you need to keep it.
//
// All four fields are float even though midi_note and velocity are logically
// integers. That is deliberate: a mixed int/float struct read through a flat
// float view misreads the int lanes, and this layout lets a binding do one
// typed-array read (the same reason stelnettts_session_pitch_frames is flat).
// midi_note is 21-108 (A0-C8); velocity is 0-127.
STELNETTTS_SESSION_API const float* stelnettts_session_piano_notes(stelnettts_session* s, int* out_n_notes);
STELNETTTS_SESSION_API int stelnettts_session_piano_sample_rate(stelnettts_session* s);
STELNETTTS_SESSION_API const char* stelnettts_session_last_synth_error(stelnettts_session* s);
STELNETTTS_SESSION_API char* stelnettts_session_translate_text(stelnettts_session* s, const char* text, const char* src_lang,
                                                           const char* tgt_lang, int max_tokens);
STELNETTTS_SESSION_API void stelnettts_session_translate_text_free(char* text);
STELNETTTS_SESSION_API stelnettts_stream* stelnettts_session_stream_open(stelnettts_session* s, int n_threads, int step_ms,
                                                                   int length_ms, int keep_ms, const char* language,
                                                                   int translate);
STELNETTTS_SESSION_API void stelnettts_session_close(stelnettts_session* s);
STELNETTTS_SESSION_API void* stelnettts_punc_init(const char* model_path);
STELNETTTS_SESSION_API const char* stelnettts_punc_process(void* ctx, const char* text);
STELNETTTS_SESSION_API void stelnettts_punc_free_text(const char* text);
STELNETTTS_SESSION_API void stelnettts_punc_free(void* ctx);
STELNETTTS_SESSION_API void* stelnettts_punc_init(const char*);
STELNETTTS_SESSION_API const char* stelnettts_punc_process(void*, const char*);
STELNETTTS_SESSION_API void stelnettts_punc_free_text(const char*);
STELNETTTS_SESSION_API void stelnettts_punc_free(void*);
STELNETTTS_SESSION_API void* stelnettts_truecase_init(const char* model_path);
STELNETTTS_SESSION_API const char* stelnettts_truecase_process(void* ctx, const char* text);
STELNETTTS_SESSION_API void stelnettts_truecase_free_text(const char* text);
STELNETTTS_SESSION_API void stelnettts_truecase_free(void* ctx);
STELNETTTS_SESSION_API void* stelnettts_truecase_init(const char* model_path);
STELNETTTS_SESSION_API const char* stelnettts_truecase_process(void* ctx, const char* text);
STELNETTTS_SESSION_API void stelnettts_truecase_free_text(const char* text);
STELNETTTS_SESSION_API void stelnettts_truecase_free(void* ctx);
STELNETTTS_SESSION_API void* stelnettts_truecase_init(const char*);
STELNETTTS_SESSION_API const char* stelnettts_truecase_process(void*, const char*);
STELNETTTS_SESSION_API void stelnettts_truecase_free_text(const char*);
STELNETTTS_SESSION_API void stelnettts_truecase_free(void*);
STELNETTTS_SESSION_API void* stelnettts_pcs_init(const char* model_path);
STELNETTTS_SESSION_API const char* stelnettts_pcs_process(void* ctx, const char* text);
STELNETTTS_SESSION_API void stelnettts_pcs_free_text(const char* text);
STELNETTTS_SESSION_API void stelnettts_pcs_free(void* ctx);
STELNETTTS_SESSION_API void* stelnettts_pcs_init(const char*);
STELNETTTS_SESSION_API const char* stelnettts_pcs_process(void*, const char*);
STELNETTTS_SESSION_API void stelnettts_pcs_free_text(const char*);
STELNETTTS_SESSION_API void stelnettts_pcs_free(void*);
STELNETTTS_SESSION_API int stelnettts_transcribe_parallel(struct whisper_context* ctx, struct whisper_full_params params,
                                                      const float* samples, int n_samples, int n_processors);
STELNETTTS_SESSION_API const char* stelnettts_c_api_version(void);
STELNETTTS_SESSION_API const char* stelnettts_dart_helpers_version(void);
STELNETTTS_SESSION_API bool stelnettts_kokoro_lang_is_german_abi(const char* lang);
STELNETTTS_SESSION_API bool stelnettts_kokoro_lang_has_native_voice_abi(const char* lang);
STELNETTTS_SESSION_API int stelnettts_kokoro_resolve_model_for_lang_abi(const char* model_path, const char* lang,
                                                                    char* out_path, int out_path_len);
STELNETTTS_SESSION_API int stelnettts_kokoro_resolve_fallback_voice_abi(const char* model_path, const char* lang,
                                                                    char* out_path, int out_path_len, char* out_picked,
                                                                    int out_picked_len);
STELNETTTS_SESSION_API int stelnettts_session_kokoro_clear_phoneme_cache(stelnettts_session* s);
STELNETTTS_SESSION_API bool stelnettts_kokoro_lang_is_german_abi(const char*);
STELNETTTS_SESSION_API bool stelnettts_kokoro_lang_has_native_voice_abi(const char*);
STELNETTTS_SESSION_API int stelnettts_kokoro_resolve_model_for_lang_abi(const char*, const char*, char*, int);
STELNETTTS_SESSION_API int stelnettts_kokoro_resolve_fallback_voice_abi(const char*, const char*, char*, int, char*, int);
STELNETTTS_SESSION_API int stelnettts_session_kokoro_clear_phoneme_cache(stelnettts_session*);
STELNETTTS_SESSION_API int stelnettts_session_set_source_language(stelnettts_session* s, const char* lang);
STELNETTTS_SESSION_API int stelnettts_session_set_target_language(stelnettts_session* s, const char* lang);
// #329 — the language a voice-cloning REFERENCE clip is spoken in (ISO-ish;
// "" or NULL clears). Cross-lingual TTS backends (cosyvoice3) compare it to the
// requested output language and, when they differ, drop the reference
// transcript so the clone speaks the target language instead of carrying the
// reference's accent. Optional: the backend otherwise infers it from the voice
// bank or the reference transcript, which cannot answer for a short one.
STELNETTTS_SESSION_API int stelnettts_session_set_tts_reference_language(stelnettts_session* s, const char* lang);
STELNETTTS_SESSION_API int stelnettts_session_set_punctuation(stelnettts_session* s, int enable);
// Select + load a punctuation-restoration model (alias auto|firered|fullstop|
// punctuate-all|pcs, or a .gguf path; "none"/NULL unloads). Auto-downloads on
// first use. Restores punctuation on backends that emit none (parakeet, CTC).
// Returns 0 on success/unload, -1 bad handle, -2 load failed, -3 not compiled.
STELNETTTS_SESSION_API int stelnettts_session_set_punc_model(stelnettts_session* s, const char* punc_model);
STELNETTTS_SESSION_API int stelnettts_session_set_translate(stelnettts_session* s, int enable);
STELNETTTS_SESSION_API int stelnettts_session_set_ask(stelnettts_session* s, const char* prompt);
STELNETTTS_SESSION_API int stelnettts_session_set_temperature(stelnettts_session* s, float temperature, uint64_t seed);
STELNETTTS_SESSION_API int stelnettts_session_set_tts_seed(stelnettts_session* s, uint64_t seed);
STELNETTTS_SESSION_API int stelnettts_session_set_tts_steps(stelnettts_session* s, int steps);
STELNETTTS_SESSION_API int stelnettts_session_set_tts_cfg_scale(stelnettts_session* s, float scale);
STELNETTTS_SESSION_API int stelnettts_session_set_tts_num_candidates(stelnettts_session* s, int n);
STELNETTTS_SESSION_API int stelnettts_session_set_g2p_dict(stelnettts_session* s, const char* source);
STELNETTTS_SESSION_API int stelnettts_session_set_top_p(stelnettts_session* s, float top_p);
STELNETTTS_SESSION_API int stelnettts_session_set_min_p(stelnettts_session* s, float min_p);
STELNETTTS_SESSION_API int stelnettts_session_set_repetition_penalty(stelnettts_session* s, float r);
STELNETTTS_SESSION_API int stelnettts_session_set_top_k(stelnettts_session* s, int top_k);
STELNETTTS_SESSION_API int stelnettts_session_set_do_sample(stelnettts_session* s, int enable);
STELNETTTS_SESSION_API int stelnettts_session_set_cfg_weight(stelnettts_session* s, float cfg_weight);
STELNETTTS_SESSION_API int stelnettts_session_set_tts_noise_temp(stelnettts_session* s, float noise_temp);
STELNETTTS_SESSION_API int stelnettts_session_set_exaggeration(stelnettts_session* s, float exaggeration);
STELNETTTS_SESSION_API int stelnettts_session_set_max_speech_tokens(stelnettts_session* s, int n);
// Issue #360: the floor counterpart to set_max_speech_tokens. UNITS are the
// backend's own AR decode step — NOT samples, NOT milliseconds. Today only the
// MOSS TTS backends consume it, where one unit is an audio-codec frame at
// sampling_rate / downsample_rate (24000 / 1920 = 12.5 Hz on the shipped
// models), i.e. 80 ms per frame, so n = 25 floors the output at ~2 s. It works
// by masking the audio-end token until n frames exist, so it bounds the decode
// rather than padding the result. Other backends return -2.
STELNETTTS_SESSION_API int stelnettts_session_set_min_speech_tokens(stelnettts_session* s, int n);
STELNETTTS_SESSION_API int stelnettts_session_set_length_scale(stelnettts_session* s, float scale);
STELNETTTS_SESSION_API int stelnettts_session_set_best_of(stelnettts_session* s, int n);
STELNETTTS_SESSION_API int stelnettts_session_set_max_new_tokens(stelnettts_session* s, int n);
STELNETTTS_SESSION_API int stelnettts_session_set_frequency_penalty(stelnettts_session* s, float penalty);
STELNETTTS_SESSION_API int stelnettts_session_set_beam_size(stelnettts_session* s, int n);
STELNETTTS_SESSION_API int stelnettts_session_set_return_logits(stelnettts_session* s, int enable);
STELNETTTS_SESSION_API int stelnettts_session_set_grammar_text(stelnettts_session* s, const char* gbnf_text,
                                                           const char* root_rule, float penalty);
STELNETTTS_SESSION_API int stelnettts_session_set_fallback_thresholds(stelnettts_session* s, float entropy_thold,
                                                                  float logprob_thold, float no_speech_thold,
                                                                  float temperature_inc);
STELNETTTS_SESSION_API int stelnettts_session_set_alt_n(stelnettts_session* s, int n);
STELNETTTS_SESSION_API int stelnettts_session_set_whisper_decode_extras(stelnettts_session* s, int suppress_nst,
                                                                    const char* suppress_regex,
                                                                    int carry_initial_prompt);
STELNETTTS_SESSION_API int stelnettts_session_detect_language(stelnettts_session* s, const float* pcm, int n_samples,
                                                          const char* lid_model_path, int method, char* out_lang,
                                                          int out_lang_cap, float* out_prob);
STELNETTTS_SESSION_API void* stelnettts_titanet_init(const char* model_path, int32_t n_threads);
STELNETTTS_SESSION_API void stelnettts_titanet_free(void* ctx);
STELNETTTS_SESSION_API int32_t stelnettts_titanet_embed(void* ctx, const float* pcm_16k, int32_t n_samples, float* out);
STELNETTTS_SESSION_API float stelnettts_titanet_cosine_sim(const float* a, const float* b, int32_t dim);
// Speaker database — closed-roster, consent-gated (issue #266).
// `expected_names_csv` is the comma-separated roster of enrolled
// participants the caller asserts are present in the audio (e.g.
// "Alice,Bob"); the db is narrowed to exactly those profiles.
// `consent_attested` affirms a lawful basis + explicit consent from
// every enrolled person (GDPR Art. 9). Open 1:N identification is
// deliberately unsupported: stelnettts_speaker_db_open refuses without
// both, and the legacy ungated _load/_enroll symbols below refuse at
// runtime (kept so old callers fail loudly, not at link time).
STELNETTTS_SESSION_API void* stelnettts_speaker_db_open(const char* dir_path, const char* expected_names_csv,
                                                    int32_t consent_attested);
STELNETTTS_SESSION_API void stelnettts_speaker_db_free(void* db);
STELNETTTS_SESSION_API int32_t stelnettts_speaker_db_count(const void* db);
STELNETTTS_SESSION_API float stelnettts_speaker_db_match(const void* db, const float* embedding, int32_t dim,
                                                     float threshold, char* out_name, int32_t out_cap);
STELNETTTS_SESSION_API int32_t stelnettts_speaker_db_enroll2(const char* dir_path, const char* name, const float* embedding,
                                                         int32_t dim, int32_t consent_attested);
// Legacy (pre-#266) entry points — always refuse at runtime.
STELNETTTS_SESSION_API void* stelnettts_speaker_db_load(const char* dir_path);
STELNETTTS_SESSION_API int32_t stelnettts_speaker_db_enroll(const char* dir_path, const char* name, const float* embedding,
                                                        int32_t dim);
STELNETTTS_SESSION_API void* stelnettts_speaker_embedder_make_abi(const char* model_spec, int32_t n_threads,
                                                              const char* cache_dir);
STELNETTTS_SESSION_API void stelnettts_speaker_embedder_free_abi(void* embedder);
STELNETTTS_SESSION_API int32_t stelnettts_speaker_embedder_dim_abi(const void* embedder);
STELNETTTS_SESSION_API int32_t stelnettts_speaker_embedder_embed_abi(void* embedder, const float* pcm_16k,
                                                                 int32_t n_samples, float* out);
STELNETTTS_SESSION_API const char* stelnettts_speaker_embedder_name_abi(const void* embedder);
STELNETTTS_SESSION_API int32_t stelnettts_speaker_cluster_abi(const float* embeddings, int32_t n, int32_t dim,
                                                          float merge_threshold, int32_t max_speakers,
                                                          int32_t* labels_out);
STELNETTTS_SESSION_API void* stelnettts_pyannote_cache_compute_abi(const float* full_audio, int32_t n_samples,
                                                               const char* model_path, int32_t n_threads);
STELNETTTS_SESSION_API void stelnettts_pyannote_cache_free_abi(void* cache);
STELNETTTS_SESSION_API int32_t stelnettts_pyannote_cache_apply_abi(const void* cache, int64_t slice_t0_cs,
                                                               stelnettts_diarize_seg_abi* segs, int32_t n_segs);

#ifdef __cplusplus
}
#endif
