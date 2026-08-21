#ifndef STELNETTTS_H
#define STELNETTTS_H

#include "ggml.h"
#include "ggml-cpu.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __GNUC__
#define STELNETTTS_DEPRECATED(func, hint) func __attribute__((deprecated(hint)))
#elif defined(_MSC_VER)
#define STELNETTTS_DEPRECATED(func, hint) __declspec(deprecated(hint)) func
#else
#define STELNETTTS_DEPRECATED(func, hint) func
#endif

#ifdef STELNETTTS_SHARED
#ifdef _WIN32
#ifdef STELNETTTS_BUILD
#define STELNETTTS_API __declspec(dllexport)
#else
#define STELNETTTS_API __declspec(dllimport)
#endif
#else
#define STELNETTTS_API __attribute__((visibility("default")))
#endif
#else
#define STELNETTTS_API
#endif

#define STELNETTTS_SAMPLE_RATE 16000
#define STELNETTTS_N_FFT 400
#define STELNETTTS_HOP_LENGTH 160
#define STELNETTTS_CHUNK_SIZE 30

#ifdef __cplusplus
extern "C" {
#endif

//
// C interface
//
// The following interface is thread-safe as long as the sample whisper_context is not used by multiple threads
// concurrently.
//
// Basic usage:
//
//     #include "stelnettts.h"
//
//     ...
//
//     whisper_context_params cparams = whisper_context_default_params();
//
//     struct whisper_context * ctx = whisper_init_from_file_with_params("/path/to/ggml-base.en.bin", cparams);
//
//     if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
//         fprintf(stderr, "failed to process audio\n");
//         return 7;
//     }
//
//     const int n_segments = whisper_full_n_segments(ctx);
//     for (int i = 0; i < n_segments; ++i) {
//         const char * text = whisper_full_get_segment_text(ctx, i);
//         printf("%s", text);
//     }
//
//     whisper_free(ctx);
//
//     ...
//
// This is a demonstration of the most straightforward usage of the library.
// "pcmf32" contains the RAW audio data in 32-bit floating point format.
//
// The interface also allows for more fine-grained control over the computation, but it requires a deeper
// understanding of how the model works.
//

struct whisper_context;
struct whisper_state;
struct whisper_full_params;
struct stelnettts_session;
struct stelnettts_session_result;

typedef int32_t whisper_pos;
typedef int32_t whisper_token;
typedef int32_t whisper_seq_id;

enum whisper_alignment_heads_preset {
    STELNETTTS_AHEADS_NONE,
    STELNETTTS_AHEADS_N_TOP_MOST, // All heads from the N-top-most text-layers
    STELNETTTS_AHEADS_CUSTOM,
    STELNETTTS_AHEADS_TINY_EN,
    STELNETTTS_AHEADS_TINY,
    STELNETTTS_AHEADS_BASE_EN,
    STELNETTTS_AHEADS_BASE,
    STELNETTTS_AHEADS_SMALL_EN,
    STELNETTTS_AHEADS_SMALL,
    STELNETTTS_AHEADS_MEDIUM_EN,
    STELNETTTS_AHEADS_MEDIUM,
    STELNETTTS_AHEADS_LARGE_V1,
    STELNETTTS_AHEADS_LARGE_V2,
    STELNETTTS_AHEADS_LARGE_V3,
    STELNETTTS_AHEADS_LARGE_V3_TURBO,
};

typedef struct whisper_ahead {
    int n_text_layer;
    int n_head;
} whisper_ahead;

typedef struct whisper_aheads {
    size_t n_heads;
    const whisper_ahead* heads;
} whisper_aheads;

struct whisper_context_params {
    bool use_gpu;
    bool flash_attn;
    int gpu_device; // CUDA device

    // [EXPERIMENTAL] Token-level timestamps with DTW
    bool dtw_token_timestamps;
    enum whisper_alignment_heads_preset dtw_aheads_preset;

    int dtw_n_top;
    struct whisper_aheads dtw_aheads;

    size_t dtw_mem_size; // TODO: remove
};

typedef struct whisper_token_data {
    whisper_token id;  // token id
    whisper_token tid; // forced timestamp token id

    float p;     // probability of the token
    float plog;  // log probability of the token
    float pt;    // probability of the timestamp token
    float ptsum; // sum of probabilities of all timestamp tokens

    // token-level timestamp data
    // do not use if you haven't computed token-level timestamps
    int64_t t0; // start time of the token
    int64_t t1; //   end time of the token

    // [EXPERIMENTAL] Token-level timestamps with DTW
    // do not use if you haven't computed token-level timestamps with dtw
    // Roughly corresponds to the moment in audio in which the token was output
    int64_t t_dtw;

    float vlen; // voice length of the token
} whisper_token_data;

// Top-N alternative candidate for a sampled token. Populated when
// wparams.alt_n > 0 (greedy decode only). The `p` is the softmax
// probability at the same decode step the chosen token came from.
// Surfaced via whisper_full_get_token_alt_* and the session-result
// stelnettts_session_result_word_alt_* accessors so consumers can build
// tap-to-pick UIs for ambiguous words (Whisper's first-choice token
// is often plausible but wrong for proper nouns / technical jargon).
typedef struct whisper_alt_token {
    whisper_token id; // alternate candidate token id
    float p;          // probability at the same decode step in [0, 1]
} whisper_alt_token;

typedef struct whisper_model_loader {
    void* context;

    size_t (*read)(void* ctx, void* output, size_t read_size);
    bool (*eof)(void* ctx);
    void (*close)(void* ctx);
} whisper_model_loader;

// grammar element type
enum whisper_gretype {
    // end of rule definition
    STELNETTTS_GRETYPE_END = 0,

    // start of alternate definition for rule
    STELNETTTS_GRETYPE_ALT = 1,

    // non-terminal element: reference to rule
    STELNETTTS_GRETYPE_RULE_REF = 2,

    // terminal element: character (code point)
    STELNETTTS_GRETYPE_CHAR = 3,

    // inverse char(s) ([^a], [^a-b] [^abc])
    STELNETTTS_GRETYPE_CHAR_NOT = 4,

    // modifies a preceding STELNETTTS_GRETYPE_CHAR or LLAMA_GRETYPE_CHAR_ALT to
    // be an inclusive range ([a-z])
    STELNETTTS_GRETYPE_CHAR_RNG_UPPER = 5,

    // modifies a preceding STELNETTTS_GRETYPE_CHAR or
    // STELNETTTS_GRETYPE_CHAR_RNG_UPPER to add an alternate char to match ([ab], [a-zA])
    STELNETTTS_GRETYPE_CHAR_ALT = 6,
};

typedef struct whisper_grammar_element {
    enum whisper_gretype type;
    uint32_t value; // Unicode code point or rule ID
} whisper_grammar_element;

typedef struct whisper_vad_params {
    float threshold;             // Probability threshold to consider as speech.
    int min_speech_duration_ms;  // Min duration for a valid speech segment.
    int min_silence_duration_ms; // Min silence duration to consider speech as ended.
    float max_speech_duration_s; // Max duration of a speech segment before forcing a new segment.
    int speech_pad_ms;           // Padding added before and after speech segments.
    float samples_overlap;       // Overlap in seconds when copying audio samples from speech segment.
} whisper_vad_params;

STELNETTTS_API const char* whisper_version(void);

// Various functions for loading a ggml whisper model.
// Allocate (almost) all memory needed for the model.
// Return NULL on failure
STELNETTTS_API struct whisper_context* whisper_init_from_file_with_params(const char* path_model,
                                                                        struct whisper_context_params params);
STELNETTTS_API struct whisper_context* whisper_init_from_buffer_with_params(void* buffer, size_t buffer_size,
                                                                          struct whisper_context_params params);
STELNETTTS_API struct whisper_context* whisper_init_with_params(struct whisper_model_loader* loader,
                                                              struct whisper_context_params params);

// These are the same as the above, but the internal state of the context is not allocated automatically
// It is the responsibility of the caller to allocate the state using whisper_init_state() (#523)
STELNETTTS_API struct whisper_context* whisper_init_from_file_with_params_no_state(const char* path_model,
                                                                                 struct whisper_context_params params);
STELNETTTS_API struct whisper_context* whisper_init_from_buffer_with_params_no_state(
    void* buffer, size_t buffer_size, struct whisper_context_params params);
STELNETTTS_API struct whisper_context* whisper_init_with_params_no_state(struct whisper_model_loader* loader,
                                                                       struct whisper_context_params params);

STELNETTTS_DEPRECATED(STELNETTTS_API struct whisper_context* whisper_init_from_file(const char* path_model),
                    "use whisper_init_from_file_with_params instead");
STELNETTTS_DEPRECATED(STELNETTTS_API struct whisper_context* whisper_init_from_buffer(void* buffer, size_t buffer_size),
                    "use whisper_init_from_buffer_with_params instead");
STELNETTTS_DEPRECATED(STELNETTTS_API struct whisper_context* whisper_init(struct whisper_model_loader* loader),
                    "use whisper_init_with_params instead");
STELNETTTS_DEPRECATED(STELNETTTS_API struct whisper_context* whisper_init_from_file_no_state(const char* path_model),
                    "use whisper_init_from_file_with_params_no_state instead");
STELNETTTS_DEPRECATED(STELNETTTS_API struct whisper_context* whisper_init_from_buffer_no_state(void* buffer,
                                                                                           size_t buffer_size),
                    "use whisper_init_from_buffer_with_params_no_state instead");
STELNETTTS_DEPRECATED(STELNETTTS_API struct whisper_context* whisper_init_no_state(struct whisper_model_loader* loader),
                    "use whisper_init_with_params_no_state instead");

STELNETTTS_API struct whisper_state* whisper_init_state(struct whisper_context* ctx);

// Given a context, enable use of OpenVINO for encode inference.
// model_path: Optional path to OpenVINO encoder IR model. If set to nullptr,
//                      the path will be generated from the ggml model path that was passed
//                      in to whisper_init_from_file. For example, if 'path_model' was
//                      "/path/to/ggml-base.en.bin", then OpenVINO IR model path will be
//                      assumed to be "/path/to/ggml-base.en-encoder-openvino.xml".
// device: OpenVINO device to run inference on ("CPU", "GPU", etc.)
// cache_dir: Optional cache directory that can speed up init time, especially for
//                     GPU, by caching compiled 'blobs' there.
//                     Set to nullptr if not used.
// Returns 0 on success. If OpenVINO is not enabled in build, this simply returns 1.
STELNETTTS_API int whisper_ctx_init_openvino_encoder_with_state(struct whisper_context* ctx, struct whisper_state* state,
                                                              const char* model_path, const char* device,
                                                              const char* cache_dir);

STELNETTTS_API int whisper_ctx_init_openvino_encoder(struct whisper_context* ctx, const char* model_path,
                                                   const char* device, const char* cache_dir);

// Frees all allocated memory
STELNETTTS_API void whisper_free(struct whisper_context* ctx);
STELNETTTS_API void whisper_free_state(struct whisper_state* state);
STELNETTTS_API void whisper_free_params(struct whisper_full_params* params);
STELNETTTS_API void whisper_free_context_params(struct whisper_context_params* params);

// Convert RAW PCM audio to log mel spectrogram.
// The resulting spectrogram is stored inside the default state of the provided whisper context.
// Returns 0 on success
STELNETTTS_API int whisper_pcm_to_mel(struct whisper_context* ctx, const float* samples, int n_samples, int n_threads);

STELNETTTS_API int whisper_pcm_to_mel_with_state(struct whisper_context* ctx, struct whisper_state* state,
                                               const float* samples, int n_samples, int n_threads);

// This can be used to set a custom log mel spectrogram inside the default state of the provided whisper context.
// Use this instead of whisper_pcm_to_mel() if you want to provide your own log mel spectrogram.
// n_mel must be 80
// Returns 0 on success
STELNETTTS_API int whisper_set_mel(struct whisper_context* ctx, const float* data, int n_len, int n_mel);

STELNETTTS_API int whisper_set_mel_with_state(struct whisper_context* ctx, struct whisper_state* state, const float* data,
                                            int n_len, int n_mel);

// Run the Whisper encoder on the log mel spectrogram stored inside the default state in the provided whisper context.
// Make sure to call whisper_pcm_to_mel() or whisper_set_mel() first.
// offset can be used to specify the offset of the first frame in the spectrogram.
// Returns 0 on success
STELNETTTS_API int whisper_encode(struct whisper_context* ctx, int offset, int n_threads);

STELNETTTS_API int whisper_encode_with_state(struct whisper_context* ctx, struct whisper_state* state, int offset,
                                           int n_threads);

// Run the Whisper decoder to obtain the logits and probabilities for the next token.
// Make sure to call whisper_encode() first.
// tokens + n_tokens is the provided context for the decoder.
// n_past is the number of tokens to use from previous decoder calls.
// Returns 0 on success
// TODO: add support for multiple decoders
STELNETTTS_API int whisper_decode(struct whisper_context* ctx, const whisper_token* tokens, int n_tokens, int n_past,
                                int n_threads);

STELNETTTS_API int whisper_decode_with_state(struct whisper_context* ctx, struct whisper_state* state,
                                           const whisper_token* tokens, int n_tokens, int n_past, int n_threads);

// Convert the provided text into tokens.
// The tokens pointer must be large enough to hold the resulting tokens.
// Returns the number of tokens on success, no more than n_max_tokens
// Returns a negative number on failure - the number of tokens that would have been returned
// TODO: not sure if correct
STELNETTTS_API int whisper_tokenize(struct whisper_context* ctx, const char* text, whisper_token* tokens,
                                  int n_max_tokens);

// Return the number of tokens in the provided text
// Equivalent to: -whisper_tokenize(ctx, text, NULL, 0)
int whisper_token_count(struct whisper_context* ctx, const char* text);

// Largest language id (i.e. number of available languages - 1)
STELNETTTS_API int whisper_lang_max_id(void);

// Return the id of the specified language, returns -1 if not found
// Examples:
//   "de" -> 2
//   "german" -> 2
STELNETTTS_API int whisper_lang_id(const char* lang);

// Return the short string of the specified language id (e.g. 2 -> "de"), returns nullptr if not found
STELNETTTS_API const char* whisper_lang_str(int id);

// Return the short string of the specified language name (e.g. 2 -> "german"), returns nullptr if not found
STELNETTTS_API const char* whisper_lang_str_full(int id);

// Use mel data at offset_ms to try and auto-detect the spoken language
// Make sure to call whisper_pcm_to_mel() or whisper_set_mel() first
// Returns the top language id or negative on failure
// If not null, fills the lang_probs array with the probabilities of all languages
// The array must be whisper_lang_max_id() + 1 in size
// ref: https://github.com/openai/whisper/blob/main/whisper/decoding.py#L18-L69
STELNETTTS_API int whisper_lang_auto_detect(struct whisper_context* ctx, int offset_ms, int n_threads, float* lang_probs);

STELNETTTS_API int whisper_lang_auto_detect_with_state(struct whisper_context* ctx, struct whisper_state* state,
                                                     int offset_ms, int n_threads, float* lang_probs);

STELNETTTS_API int whisper_n_len(struct whisper_context* ctx);            // mel length
STELNETTTS_API int whisper_n_len_from_state(struct whisper_state* state); // mel length
STELNETTTS_API int whisper_n_vocab(struct whisper_context* ctx);
STELNETTTS_API int whisper_n_text_ctx(struct whisper_context* ctx);
STELNETTTS_API int whisper_n_audio_ctx(struct whisper_context* ctx);
STELNETTTS_API int whisper_is_multilingual(struct whisper_context* ctx);
// Tiron (#295): 1 if the loaded model has <|speakerN|> tokens.
STELNETTTS_API int whisper_has_speaker_tokens(struct whisper_context* ctx);

STELNETTTS_API int whisper_model_n_vocab(struct whisper_context* ctx);
STELNETTTS_API int whisper_model_n_audio_ctx(struct whisper_context* ctx);
STELNETTTS_API int whisper_model_n_audio_state(struct whisper_context* ctx);
STELNETTTS_API int whisper_model_n_audio_head(struct whisper_context* ctx);
STELNETTTS_API int whisper_model_n_audio_layer(struct whisper_context* ctx);
STELNETTTS_API int whisper_model_n_text_ctx(struct whisper_context* ctx);
STELNETTTS_API int whisper_model_n_text_state(struct whisper_context* ctx);
STELNETTTS_API int whisper_model_n_text_head(struct whisper_context* ctx);
STELNETTTS_API int whisper_model_n_text_layer(struct whisper_context* ctx);
STELNETTTS_API int whisper_model_n_mels(struct whisper_context* ctx);
STELNETTTS_API int whisper_model_ftype(struct whisper_context* ctx);
STELNETTTS_API int whisper_model_type(struct whisper_context* ctx);

// Token logits obtained from the last call to whisper_decode()
// The logits for the last token are stored in the last row
// Rows: n_tokens
// Cols: n_vocab
STELNETTTS_API float* whisper_get_logits(struct whisper_context* ctx);
STELNETTTS_API float* whisper_get_logits_from_state(struct whisper_state* state);

// Token Id -> String. Uses the vocabulary in the provided context
STELNETTTS_API const char* whisper_token_to_str(struct whisper_context* ctx, whisper_token token);
STELNETTTS_API const char* whisper_model_type_readable(struct whisper_context* ctx);


// Special tokens
STELNETTTS_API whisper_token whisper_token_eot(struct whisper_context* ctx);
STELNETTTS_API whisper_token whisper_token_sot(struct whisper_context* ctx);
STELNETTTS_API whisper_token whisper_token_solm(struct whisper_context* ctx);
STELNETTTS_API whisper_token whisper_token_prev(struct whisper_context* ctx);
STELNETTTS_API whisper_token whisper_token_nosp(struct whisper_context* ctx);
STELNETTTS_API whisper_token whisper_token_not(struct whisper_context* ctx);
STELNETTTS_API whisper_token whisper_token_beg(struct whisper_context* ctx);
STELNETTTS_API whisper_token whisper_token_lang(struct whisper_context* ctx, int lang_id);

// Task tokens
STELNETTTS_API whisper_token whisper_token_translate(struct whisper_context* ctx);
STELNETTTS_API whisper_token whisper_token_transcribe(struct whisper_context* ctx);

// Performance information from the default state.
struct whisper_timings {
    float sample_ms;
    float encode_ms;
    float decode_ms;
    float batchd_ms;
    float prompt_ms;
};
STELNETTTS_API struct whisper_timings* whisper_get_timings(struct whisper_context* ctx);
STELNETTTS_API void whisper_print_timings(struct whisper_context* ctx);
STELNETTTS_API void whisper_reset_timings(struct whisper_context* ctx);

// Print system information
STELNETTTS_API const char* whisper_print_system_info(void);

////////////////////////////////////////////////////////////////////////////

// Available sampling strategies
enum whisper_sampling_strategy {
    STELNETTTS_SAMPLING_GREEDY,      // similar to OpenAI's GreedyDecoder
    STELNETTTS_SAMPLING_BEAM_SEARCH, // similar to OpenAI's BeamSearchDecoder
};

// Text segment callback
// Called on every newly generated text segment
// Use the whisper_full_...() functions to obtain the text segments
typedef void (*whisper_new_segment_callback)(struct whisper_context* ctx, struct whisper_state* state, int n_new,
                                             void* user_data);

// Progress callback
typedef void (*whisper_progress_callback)(struct whisper_context* ctx, struct whisper_state* state, int progress,
                                          void* user_data);

// Encoder begin callback
// If not NULL, called before the encoder starts
// If it returns false, the computation is aborted
typedef bool (*whisper_encoder_begin_callback)(struct whisper_context* ctx, struct whisper_state* state,
                                               void* user_data);

// Logits filter callback
// Can be used to modify the logits before sampling
// If not NULL, called after applying temperature to logits
typedef void (*whisper_logits_filter_callback)(struct whisper_context* ctx, struct whisper_state* state,
                                               const whisper_token_data* tokens, int n_tokens, float* logits,
                                               void* user_data);

// Parameters for the whisper_full() function
// If you change the order or add new parameters, make sure to update the default values in stelnettts:
// whisper_full_default_params()
struct whisper_full_params {
    enum whisper_sampling_strategy strategy;

    int n_threads;
    int n_max_text_ctx; // max tokens to use from past text as prompt for the decoder
    int offset_ms;      // start offset in ms
    int duration_ms;    // audio duration to process in ms

    bool translate;
    bool no_context;       // do not use past transcription (if any) as initial prompt for the decoder
    bool no_timestamps;    // do not generate timestamps
    bool single_segment;   // force single segment output (useful for streaming)
    bool print_special;    // print special tokens (e.g. <SOT>, <EOT>, <BEG>, etc.)
    bool print_progress;   // print progress information
    bool print_realtime;   // print results from within stelnettts (avoid it, use callback instead)
    bool print_timestamps; // print timestamps for each text segment when printing realtime

    // [EXPERIMENTAL] token-level timestamps
    bool token_timestamps; // enable token-level timestamps
    float thold_pt;        // timestamp token probability threshold (~0.01)
    float thold_ptsum;     // timestamp token sum probability threshold (~0.01)
    int max_len;           // max segment length in characters
    bool split_on_word;    // split on word rather than on token (when used with max_len)
    int max_tokens;        // max tokens per segment (0 = no limit)

    // [EXPERIMENTAL] speed-up techniques
    // note: these can significantly reduce the quality of the output
    bool debug_mode; // enable debug_mode provides extra info (eg. Dump log_mel)
    int audio_ctx;   // overwrite the audio context size (0 = use default)

    // [EXPERIMENTAL] [TDRZ] tinydiarize
    bool tdrz_enable; // enable tinydiarize speaker turn detection

    // A regular expression that matches tokens to suppress
    const char* suppress_regex;

    // tokens to provide to the whisper decoder as initial prompt
    // these are prepended to any existing text context from a previous call
    // use whisper_tokenize() to convert text to tokens
    // maximum of whisper_n_text_ctx()/2 tokens are used (typically 224)
    const char* initial_prompt;
    bool
        carry_initial_prompt; // if true, always prepend initial_prompt to every decode window (may reduce conditioning on previous text)
    const whisper_token* prompt_tokens;
    int prompt_n_tokens;

    // for auto-detection, set to nullptr, "" or "auto"
    const char* language;
    bool detect_language;

    // common decoding parameters:
    bool
        suppress_blank; // ref: https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/decoding.py#L89
    bool
        suppress_nst; // non-speech tokens, ref: https://github.com/openai/whisper/blob/7858aa9c08d98f75575035ecd6481f462d66ca27/whisper/tokenizer.py#L224-L253

    float temperature; // initial decoding temperature, ref: https://ai.stackexchange.com/a/32478
    float
        max_initial_ts; // ref: https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/decoding.py#L97
    float
        length_penalty; // ref: https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L267

    // fallback parameters
    // ref: https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L274-L278
    float temperature_inc;
    float entropy_thold; // similar to OpenAI's "compression_ratio_threshold"
    float logprob_thold;
    float no_speech_thold;

    struct {
        int best_of; // ref: https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L264
    } greedy;

    struct {
        int beam_size; // ref: https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L265

        float patience; // TODO: not implemented, ref: https://arxiv.org/pdf/2204.05424.pdf
    } beam_search;

    // called for every newly generated text segment
    whisper_new_segment_callback new_segment_callback;
    void* new_segment_callback_user_data;

    // called on each progress update
    whisper_progress_callback progress_callback;
    void* progress_callback_user_data;

    // called each time before the encoder starts
    whisper_encoder_begin_callback encoder_begin_callback;
    void* encoder_begin_callback_user_data;

    // called each time before ggml computation starts
    ggml_abort_callback abort_callback;
    void* abort_callback_user_data;

    // called by each decoder to filter obtained logits
    whisper_logits_filter_callback logits_filter_callback;
    void* logits_filter_callback_user_data;

    const whisper_grammar_element** grammar_rules;
    size_t n_grammar_rules;
    size_t i_start_rule;
    float grammar_penalty;

    // Voice Activity Detection (VAD) params
    bool vad;                   // Enable VAD
    const char* vad_model_path; // Path to VAD model

    whisper_vad_params vad_params;

    // Capture top-N alternative-candidate tokens at each greedy-sampled
    // step. 0 (default) = off. Beam-search siblings are not captured —
    // they're conditional on the beam, not greedy alternatives.
    int alt_n;
};

// NOTE: this function allocates memory, and it is the responsibility of the caller to free the pointer - see whisper_free_context_params & whisper_free_params()
STELNETTTS_API struct whisper_context_params* whisper_context_default_params_by_ref(void);
STELNETTTS_API struct whisper_context_params whisper_context_default_params(void);

STELNETTTS_API struct whisper_full_params* whisper_full_default_params_by_ref(enum whisper_sampling_strategy strategy);
STELNETTTS_API struct whisper_full_params whisper_full_default_params(enum whisper_sampling_strategy strategy);

// Pointer-arg wrappers for FFI bindings (Dart, etc.) — see stelnettts.cpp.
// Use these instead of the by-value variants when the binding cannot
// reliably marshal a large struct across the C ABI on every target.
STELNETTTS_API struct whisper_context* whisper_init_from_file_with_params_by_ref(const char* path_model,
                                                                               struct whisper_context_params* params);
STELNETTTS_API struct whisper_context* whisper_init_from_file_with_params_no_state_by_ref(
    const char* path_model, struct whisper_context_params* params);
STELNETTTS_API int whisper_full_by_ref(struct whisper_context* ctx, struct whisper_full_params* params,
                                     const float* samples, int n_samples);
STELNETTTS_API void stelnettts_params_set_max_tokens(struct whisper_full_params* p, int n);
STELNETTTS_API void stelnettts_params_set_temperature(struct whisper_full_params* p, float t);

// Unified session decode / sampling controls.
// set_temperature: rc=-2 = no backend supports runtime temperature (soft no-op).
// set_tts_seed:    rc=-2 = no TTS backend supports runtime reseeding (soft no-op).
// max_new_tokens <= 0 clears the cap override; frequency_penalty <= 0 disables it.
STELNETTTS_API int stelnettts_session_set_temperature(struct stelnettts_session* s, float temperature, uint64_t seed);
STELNETTTS_API int stelnettts_session_set_tts_seed(struct stelnettts_session* s, uint64_t seed);
STELNETTTS_API int stelnettts_session_set_tts_steps(struct stelnettts_session* s, int steps);
STELNETTTS_API int stelnettts_session_set_tts_cfg_scale(struct stelnettts_session* s, float scale);
STELNETTTS_API int stelnettts_session_set_max_new_tokens(struct stelnettts_session* s, int n);
STELNETTTS_API int stelnettts_session_set_frequency_penalty(struct stelnettts_session* s, float penalty);
STELNETTTS_API int stelnettts_session_set_top_p(struct stelnettts_session* s, float top_p);
STELNETTTS_API int stelnettts_session_set_min_p(struct stelnettts_session* s, float min_p);
STELNETTTS_API int stelnettts_session_set_repetition_penalty(struct stelnettts_session* s, float r);
STELNETTTS_API int stelnettts_session_set_top_k(struct stelnettts_session* s, int top_k);
STELNETTTS_API int stelnettts_session_set_do_sample(struct stelnettts_session* s, int enable);
STELNETTTS_API int stelnettts_session_set_tts_num_candidates(struct stelnettts_session* s, int n);
STELNETTTS_API int stelnettts_session_set_cfg_weight(struct stelnettts_session* s, float cfg_weight);
STELNETTTS_API int stelnettts_session_set_tts_noise_temp(struct stelnettts_session* s, float noise_temp);
STELNETTTS_API int stelnettts_session_set_exaggeration(struct stelnettts_session* s, float exaggeration);
STELNETTTS_API int stelnettts_session_set_max_speech_tokens(struct stelnettts_session* s, int n);
// Issue #360: the floor counterpart to set_max_speech_tokens. UNITS are the
// backend's own AR decode step — NOT samples, NOT milliseconds. Today only the
// MOSS TTS backends consume it, where one unit is an audio-codec frame at
// sampling_rate / downsample_rate (24000 / 1920 = 12.5 Hz on the shipped
// models), i.e. 80 ms per frame, so n = 25 floors the output at ~2 s. It works
// by masking the audio-end token until n frames exist, so it bounds the decode
// rather than padding the result. Other backends return -2.
STELNETTTS_API int stelnettts_session_set_min_speech_tokens(struct stelnettts_session* s, int n);
STELNETTTS_API int stelnettts_session_set_length_scale(struct stelnettts_session* s, float scale);
// G2P dict source: "olaph" (MIT), "open-dict" (CC-BY-SA), or file path.
STELNETTTS_API int stelnettts_session_set_g2p_dict(struct stelnettts_session* s, const char* source);
STELNETTTS_API int stelnettts_session_set_best_of(struct stelnettts_session* s, int n);
STELNETTTS_API int stelnettts_session_set_beam_size(struct stelnettts_session* s, int n);
STELNETTTS_API int stelnettts_session_set_return_logits(struct stelnettts_session* s, int enable);
STELNETTTS_API int stelnettts_session_set_grammar_text(struct stelnettts_session* s, const char* gbnf_text,
                                                   const char* root_rule, float penalty);
STELNETTTS_API int stelnettts_session_set_fallback_thresholds(struct stelnettts_session* s, float entropy_thold,
                                                          float logprob_thold, float no_speech_thold,
                                                          float temperature_inc);
// Apply a named bundle of the four thresholds above: "conservative",
// "balanced" (== the shipped defaults, always a no-op) or "aggressive".
// "strict"/"default"/"loose" are accepted aliases. Mirrors the CLI's
// --sensitivity. Returns 0 on success, -1 on a null/empty argument, -2 for an
// unrecognised name — an unknown preset is REJECTED rather than silently
// treated as balanced, so a typo is visible. Call before transcribing; a later
// stelnettts_session_set_fallback_thresholds() overrides it.
STELNETTTS_API int stelnettts_session_set_sensitivity(struct stelnettts_session* s, const char* preset_name);
STELNETTTS_API int stelnettts_session_set_alt_n(struct stelnettts_session* s, int n);
STELNETTTS_API int stelnettts_session_set_whisper_decode_extras(struct stelnettts_session* s, int suppress_nst,
                                                            const char* suppress_regex, int carry_initial_prompt);
STELNETTTS_API int stelnettts_session_set_ask(struct stelnettts_session* s, const char* prompt);

// TTS synthesis — returns malloc'd float32 PCM at 24 kHz mono.
// Caller frees with stelnettts_pcm_free(). Returns nullptr on failure.
STELNETTTS_API float* stelnettts_session_synthesize(struct stelnettts_session* s, const char* text, int* out_n_samples);
// UNMARKED synthesis (no watermark) — hard-refused (returns nullptr) unless the
// integrator first calls stelnettts_session_accept_marking_responsibility(). Use
// stelnettts_session_synthesize() for the default watermarked output.
STELNETTTS_API float* stelnettts_session_synthesize_raw(struct stelnettts_session* s, const char* text, int* out_n_samples);
// Attest that the integrator accepts AI-content marking/disclosure responsibility
// (EU AI Act Art. 50). REQUIRED to use stelnettts_session_synthesize_raw(); the
// default marked paths do not need it. `attestation` is recorded for audit.
STELNETTTS_API int stelnettts_session_accept_marking_responsibility(struct stelnettts_session* s, const char* attestation);
// Declare whose voice the current PRESET voice is: "real_person" | "synthetic" |
// "unknown". Cloning is not the only way to produce a deep fake: a preset voice
// shipped inside a model can be an identifiable individual (a named donor, a
// corpus speaker such as VCTK's p225), and Art. 3(60) attaches to the audio
// resembling that person, not to which pipeline made it. Setting real_person
// makes the Art. 50(4) reminder fire for a non-cloned voice. It does NOT require
// a consent attestation — whether the donor agreed to the model being trained is
// a licensing matter settled upstream that you cannot attest to.
// Returns 0, -1 on a bad session, -2 on an unrecognised value.
STELNETTTS_API int stelnettts_session_set_speaker_identity(struct stelnettts_session* s, const char* identity);

// Spoken AI-disclosure for voice clones (EU AI Act Art. 50(4)). The watermark
// on synthesize() covers the machine-readable marking duty (Art. 50(2)), but a
// deepfake additionally needs a VISIBLE OR AUDIBLE label, which a watermark is
// not. The CLI and server prepend one automatically; the ABI cannot (neutral-
// voice synthesis is not portable once a clone voice is applied), so it hands
// you the pieces and logs a one-time [MARKING] warning instead of refusing.
//
// disclaimer_text() is a static string — use it for a visible label.
// get_disclaimer_pcm() synthesizes it in the neutral voice and MUST be called
// BEFORE stelnettts_session_set_voice() installs a clone; afterwards it returns
// nullptr, because a disclosure spoken in the cloned voice would make the
// output more deceptive rather than less. Caller frees with stelnettts_pcm_free().
STELNETTTS_API const char* stelnettts_session_disclaimer_text(void);
STELNETTTS_API float* stelnettts_session_get_disclaimer_pcm(struct stelnettts_session* s, int* out_n_samples);

STELNETTTS_API void stelnettts_pcm_free(float* pcm);

// Speech-to-Speech — audio in → audio out via a single model pass.
// Supported on backends with S2S capability (lfm2-audio, mini-omni2, sidon,
// voxcpm2-vae).
// Returns malloc'd float32 PCM; caller frees with stelnettts_pcm_free().
// out_text (optional): if non-null, receives the intermediate transcript
// (malloc'd, caller frees with free()). Returns nullptr on failure or
// if the backend doesn't support S2S.
STELNETTTS_API float* stelnettts_session_speech_to_speech(struct stelnettts_session* s, const float* in_samples,
                                                      int n_in_samples, char** out_text, int* out_n_samples);

// Set hotwords for contextual biasing. Comma-separated list of words or
// phrases. For CTC/TDT backends (parakeet), configures the Aho-Corasick
// trie with the given boost factor. For LLM backends, the hotwords are
// prepended to the ask prompt on the next transcribe call.
// Pass NULL or empty string to clear. Returns 0 on success.
STELNETTTS_API int stelnettts_session_set_hotwords(struct stelnettts_session* s, const char* hotwords, float boost);

// Human-readable error from the last failed synthesize call. Empty string
// when the last call succeeded. Pointer owned by the session.
STELNETTTS_API const char* stelnettts_session_last_synth_error(struct stelnettts_session* s);

// Sample rate the backend expects for input PCM. Use with
// stelnettts_audio_load_at_rate to load audio at the model's native rate,
// avoiding a lossy down-then-up resample. Returns 16000 for Whisper-family
// backends and 0 on error.
STELNETTTS_API int stelnettts_session_input_sample_rate(struct stelnettts_session* s);

// Tell the session what sample rate the next PCM input call's audio is at.
// Backends that normally resample (e.g. 16 kHz → 24 kHz) will skip the
// step when the rate already matches. Defaults to 16000 for back-compat.
STELNETTTS_API int stelnettts_session_set_pcm_sample_rate(struct stelnettts_session* s, int rate);

// #332: Sample rate of the PCM produced by synthesize / synthesize_raw /
// synthesize_streaming / get_disclaimer_pcm / speech_to_speech for this
// session's backend (the "backend-native rate" those calls document).
// Returns 0 for a NULL session or a backend that produces no audio output
// (ASR-only). Mirrors the CLI adapters' tts_sample_rate().
STELNETTTS_API int stelnettts_session_output_sample_rate(struct stelnettts_session* s);

// #332: channel counts for the session's audio input (transcribe / s2s /
// voice input) and audio output (synthesize / s2s). Both are 1 (mono) for
// every current backend — source separation is the stereo exception and has
// its own surface (stelnettts_session_separate*). Getters rather than
// documented constants so a future multi-channel backend is additive.
// Return 0 on a NULL session; output_channels is also 0 when the backend
// produces no audio output.
STELNETTTS_API int stelnettts_session_input_channels(struct stelnettts_session* s);
STELNETTTS_API int stelnettts_session_output_channels(struct stelnettts_session* s);

// CTC vocabulary access. Returns 0 / "" for backends without an exposed CTC
// vocabulary. The token text pointer is model-owned and must not be freed.
STELNETTTS_API int stelnettts_session_n_vocab(struct stelnettts_session* s);
STELNETTTS_API const char* stelnettts_session_token_text(struct stelnettts_session* s, int id);

// Per-frame dense CTC grid captured when stelnettts_session_set_return_logits(s,1)
// was enabled before transcription. Frame-major:
// logits[t * n_logit_vocab + v]. Pointer is result-owned and valid until
// stelnettts_session_result_free().
STELNETTTS_API int stelnettts_session_result_n_logit_frames(struct stelnettts_session_result* r);
STELNETTTS_API int stelnettts_session_result_n_logit_vocab(struct stelnettts_session_result* r);
STELNETTTS_API const float* stelnettts_session_result_logits(struct stelnettts_session_result* r);

// Run the entire model: PCM -> log mel spectrogram -> encoder -> decoder -> text
// Not thread safe for same context
// Uses the specified decoding strategy to obtain the text.
STELNETTTS_API int whisper_full(struct whisper_context* ctx, struct whisper_full_params params, const float* samples,
                              int n_samples);

STELNETTTS_API int whisper_full_with_state(struct whisper_context* ctx, struct whisper_state* state,
                                         struct whisper_full_params params, const float* samples, int n_samples);

// Split the input audio in chunks and process each chunk separately using whisper_full_with_state()
// Result is stored in the default state of the context
// Not thread safe if executed in parallel on the same context.
// It seems this approach can offer some speedup in some cases.
// However, the transcription accuracy can be worse at the beginning and end of each chunk.
STELNETTTS_API int whisper_full_parallel(struct whisper_context* ctx, struct whisper_full_params params,
                                       const float* samples, int n_samples, int n_processors);

// Number of generated text segments
// A segment can be a few words, a sentence, or even a paragraph.
STELNETTTS_API int whisper_full_n_segments(struct whisper_context* ctx);
STELNETTTS_API int whisper_full_n_segments_from_state(struct whisper_state* state);

// Language id associated with the context's default state
STELNETTTS_API int whisper_full_lang_id(struct whisper_context* ctx);

// Language id associated with the provided state
STELNETTTS_API int whisper_full_lang_id_from_state(struct whisper_state* state);

// Get the start and end time of the specified segment
STELNETTTS_API int64_t whisper_full_get_segment_t0(struct whisper_context* ctx, int i_segment);
STELNETTTS_API int64_t whisper_full_get_segment_t0_from_state(struct whisper_state* state, int i_segment);

STELNETTTS_API int64_t whisper_full_get_segment_t1(struct whisper_context* ctx, int i_segment);
STELNETTTS_API int64_t whisper_full_get_segment_t1_from_state(struct whisper_state* state, int i_segment);

// Get whether the next segment is predicted as a speaker turn
STELNETTTS_API bool whisper_full_get_segment_speaker_turn_next(struct whisper_context* ctx, int i_segment);
STELNETTTS_API bool whisper_full_get_segment_speaker_turn_next_from_state(struct whisper_state* state, int i_segment);

// Get the text of the specified segment
STELNETTTS_API const char* whisper_full_get_segment_text(struct whisper_context* ctx, int i_segment);
STELNETTTS_API const char* whisper_full_get_segment_text_from_state(struct whisper_state* state, int i_segment);

// Get number of tokens in the specified segment
STELNETTTS_API int whisper_full_n_tokens(struct whisper_context* ctx, int i_segment);
STELNETTTS_API int whisper_full_n_tokens_from_state(struct whisper_state* state, int i_segment);

// Get the token text of the specified token in the specified segment
STELNETTTS_API const char* whisper_full_get_token_text(struct whisper_context* ctx, int i_segment, int i_token);
STELNETTTS_API const char* whisper_full_get_token_text_from_state(struct whisper_context* ctx,
                                                                struct whisper_state* state, int i_segment,
                                                                int i_token);

STELNETTTS_API whisper_token whisper_full_get_token_id(struct whisper_context* ctx, int i_segment, int i_token);
STELNETTTS_API whisper_token whisper_full_get_token_id_from_state(struct whisper_state* state, int i_segment,
                                                                int i_token);

// Get token data for the specified token in the specified segment
// This contains probabilities, timestamps, etc.
STELNETTTS_API whisper_token_data whisper_full_get_token_data(struct whisper_context* ctx, int i_segment, int i_token);
STELNETTTS_API whisper_token_data whisper_full_get_token_data_from_state(struct whisper_state* state, int i_segment,
                                                                       int i_token);

// Get the probability of the specified token in the specified segment
STELNETTTS_API float whisper_full_get_token_p(struct whisper_context* ctx, int i_segment, int i_token);
STELNETTTS_API float whisper_full_get_token_p_from_state(struct whisper_state* state, int i_segment, int i_token);

// Top-N alternative candidates for a sampled token. Returns 0 / 0 / 0.0f
// when alts are not captured (wparams.alt_n == 0, beam search, or i_alt
// out of range). The chosen token is not present in the alts list.
STELNETTTS_API int whisper_full_get_token_n_alts(struct whisper_context* ctx, int i_segment, int i_token);
STELNETTTS_API int whisper_full_get_token_n_alts_from_state(struct whisper_state* state, int i_segment, int i_token);
STELNETTTS_API whisper_token whisper_full_get_token_alt_id(struct whisper_context* ctx, int i_segment, int i_token,
                                                         int i_alt);
STELNETTTS_API whisper_token whisper_full_get_token_alt_id_from_state(struct whisper_state* state, int i_segment,
                                                                    int i_token, int i_alt);
STELNETTTS_API float whisper_full_get_token_alt_p(struct whisper_context* ctx, int i_segment, int i_token, int i_alt);
STELNETTTS_API float whisper_full_get_token_alt_p_from_state(struct whisper_state* state, int i_segment, int i_token,
                                                           int i_alt);

//
// Voice Activity Detection (VAD)
//

struct whisper_vad_context;

STELNETTTS_API struct whisper_vad_params whisper_vad_default_params(void);

struct whisper_vad_context_params {
    int n_threads; // The number of threads to use for processing.
    bool use_gpu;
    int gpu_device; // CUDA device
};

STELNETTTS_API struct whisper_vad_context_params whisper_vad_default_context_params(void);

STELNETTTS_API struct whisper_vad_context* whisper_vad_init_from_file_with_params(
    const char* path_model, struct whisper_vad_context_params params);
STELNETTTS_API struct whisper_vad_context* whisper_vad_init_with_params(struct whisper_model_loader* loader,
                                                                      struct whisper_vad_context_params params);

STELNETTTS_API bool whisper_vad_detect_speech(struct whisper_vad_context* vctx, const float* samples, int n_samples);

STELNETTTS_API int whisper_vad_n_probs(struct whisper_vad_context* vctx);
STELNETTTS_API float* whisper_vad_probs(struct whisper_vad_context* vctx);

struct whisper_vad_segments;

STELNETTTS_API struct whisper_vad_segments* whisper_vad_segments_from_probs(struct whisper_vad_context* vctx,
                                                                          struct whisper_vad_params params);

STELNETTTS_API struct whisper_vad_segments* whisper_vad_segments_from_samples(struct whisper_vad_context* vctx,
                                                                            struct whisper_vad_params params,
                                                                            const float* samples, int n_samples);

STELNETTTS_API int whisper_vad_segments_n_segments(struct whisper_vad_segments* segments);

STELNETTTS_API float whisper_vad_segments_get_segment_t0(struct whisper_vad_segments* segments, int i_segment);
STELNETTTS_API float whisper_vad_segments_get_segment_t1(struct whisper_vad_segments* segments, int i_segment);

STELNETTTS_API void whisper_vad_free_segments(struct whisper_vad_segments* segments);
STELNETTTS_API void whisper_vad_free(struct whisper_vad_context* ctx);

// StelnetTTS C-ABI VAD helpers.
//
// stelnettts_vad_segments is the legacy Silero/whisper_vad path and returns
// malloc-owned [start_cs, end_cs] float pairs in centiseconds.
STELNETTTS_API int stelnettts_vad_segments(const char* vad_model_path, const float* pcm, int n_samples, int sample_rate,
                                       float threshold, int min_speech_ms, int min_silence_ms, int n_threads,
                                       bool use_gpu, float** out_spans);

// stelnettts_vad_slices routes through StelnetTTS's shared VAD dispatcher and
// can use Silero, FireRedVAD, MarbleNet, or Whisper-VAD-EncDec depending
// on the concrete model path. It returns malloc-owned [start_s, end_s]
// float pairs in seconds. Passing threshold <= 0 leaves per-model default
// threshold behavior intact, including Whisper-VAD-EncDec auto-tuning.
STELNETTTS_API int stelnettts_vad_slices(const char* vad_model_path, const float* pcm, int n_samples, int sample_rate,
                                     float threshold, int min_speech_ms, int min_silence_ms, int speech_pad_ms,
                                     float max_chunk_duration_s, int n_threads, float** out_spans);

STELNETTTS_API void stelnettts_vad_free(float* spans);

// ─── Chunk-boundary LCS dedup ──────────────────────────────────────────
//
// Sub-word LCS over emitted token ids — the algorithm upstream NeMo
// uses in `BatchedFrameASRTDT` for hypothesis stitching between
// overlap-save chunks. Bindings that drive `libstelnettts` chunk-by-chunk
// can call this between adjacent chunks to remove duplicate leading
// tokens that arise when chunk[i-1]'s right-extension and chunk[i]'s
// left-extension transcribe the same audio twice.
//
// Inputs are flat int32 arrays of token ids:
//   prev_tail_tokens : the LAST `n_prev` tokens emitted by chunk i-1
//     (you typically pick this as `delay_seconds * tokens_per_second`
//     so the search is bounded; for parakeet at 80 ms / 1 token-per-
//     frame, `delay_seconds * 12.5` is a safe upper bound).
//   curr_tokens      : the full token-id list emitted by chunk i.
//   min_lcs_length   : LCS lengths below this are ignored (no slice).
//                      Pass 1 to match NeMo's default; raise to 3-4
//                      if your audio has long-silence regions where
//                      blank tokens dominate the boundary.
//
// Returns the number of leading tokens of `curr_tokens` to drop. The
// caller is responsible for applying the slice to its own segment /
// word / text representation.
//
// Returns 0 on invalid input (null pointers, n_* <= 0) or when no LCS
// ≥ min_lcs_length is found.
STELNETTTS_API int stelnettts_lcs_dedup_prefix_count(const int32_t* prev_tail_tokens, int n_prev,
                                                 const int32_t* curr_tokens, int n_curr, int min_lcs_length);

// ─── AI-generated audio watermark ─────────────────────────────────────
//
// stelnettts_session_synthesize() auto-embeds the watermark. Use
// stelnettts_session_synthesize_raw() + stelnettts_watermark_embed() only
// when you need DSP (speed change, mixing) between synthesis and
// watermarking. Two watermark modes:
//
// 1. **Built-in spread-spectrum** (default): frequency-domain pattern,
//    survives re-encoding and moderate compression.
// 2. **AudioSeal neural** (optional): Meta's SEANet-based watermark,
//    stronger robustness. Load via stelnettts_watermark_load_model().
//
// When an AudioSeal model is loaded, embed/detect dispatch to it
// automatically. Otherwise they use the spread-spectrum fallback.
//
// stelnettts_watermark_detect() returns confidence in [0, 1]:
//   > 0.65 — watermark present (AI-generated)
//   < 0.40 — no watermark detected
//
// Input: float32 mono PCM. AudioSeal expects 16 kHz; spread-spectrum
// works at any sample rate.
STELNETTTS_API float stelnettts_watermark_detect(const float* pcm, int n_samples);

// Embed watermark into float32 mono PCM (in-place).
//
// `alpha` controls spread-spectrum strength; ignored when AudioSeal is loaded.
// PASS alpha <= 0 — that selects the band-limited default (~0.05) which is
// what makes the mark reliably DETECTABLE, the property EU AI Act Art. 50(2)
// actually requires. An explicit positive alpha is used verbatim, so passing
// the old 0.005 documented here produces a mark too faint to find again on
// real speech: marking that cannot be detected is not marking. Only pass a
// literal alpha if you are deliberately A/B-ing watermark strength.
STELNETTTS_API void stelnettts_watermark_embed(float* pcm, int n_samples, float alpha);

// C2PA (Content Credentials) signing of an in-memory audio CONTAINER (WAV/MP3
// bytes, not raw PCM). Signs with the user cert/key (PEM file paths) when both
// are non-NULL, else a bundled self-signed default cert (works with no
// filesystem, including the browser). `format` is a C2PA MIME string, e.g.
// "audio/wav" or "audio/mpeg". Returns malloc'd signed bytes (free with
// stelnettts_c2pa_free) and sets *out_len, or NULL when C2PA is unavailable, the
// container can't embed a manifest (AAC/Opus), or signing fails. Available to
// wasm / bindings / server, not just the CLI.
STELNETTTS_API unsigned char* stelnettts_c2pa_sign(const unsigned char* data, size_t len, const char* format,
                                               const char* cert_path, const char* key_path, size_t* out_len);
STELNETTTS_API void stelnettts_c2pa_free(unsigned char* p);

// Wrap float32 mono PCM into a 16-bit WAV carrying the AI-generated provenance
// metadata tag (standard WAV LIST/INFO chunk — interoperable, any tool reads it).
// The zero-cost provenance floor for wasm/bindings that only get raw PCM from
// synthesis. Returns malloc'd WAV bytes (free with stelnettts_c2pa_free), *out_len
// set, or NULL on bad input. Feed to stelnettts_c2pa_sign() to also embed a C2PA
// manifest.
STELNETTTS_API unsigned char* stelnettts_pcm_to_wav(const float* pcm, int n_samples, int sample_rate, size_t* out_len);

// Load an AudioSeal GGUF model for neural watermarking. Call once at
// startup. Returns 0 on success, -1 on failure (falls back to
// spread-spectrum). The model is shared across all subsequent
// embed/detect calls.
STELNETTTS_API int stelnettts_watermark_load_model(const char* gguf_path);

// ─── Atomic progress polling (Dart FFI) ──────────────────────────────
//
// Dart cannot use C function pointers as callbacks; instead it polls
// the module-level atomic via stelnettts_get_progress(). Returns 0-100
// during transcription, -1 when idle.
STELNETTTS_API int stelnettts_get_progress(void);
STELNETTTS_API void stelnettts_reset_progress(void);

// ─── Audio decode ────────────────────────────────────────────────────
//
// Decode any supported audio file (WAV/MP3/FLAC/OGG/Opus/AAC/M4A/WebM/AMR/…)
// to 16 kHz mono float PCM. Returns 0 with a malloc-owned buffer in *out_pcm
// (free with stelnettts_audio_free), *out_samples element count and
// *out_sample_rate (always 16000), or a negative error. No ffmpeg needed for
// the common formats (glint AAC/Opus, miniaudio, AudioToolbox/fdk, libopus, …).
STELNETTTS_API int stelnettts_audio_load(const char* path, float** out_pcm, int* out_samples, int* out_sample_rate);

// Like stelnettts_audio_load but resamples to `target_rate` instead of 16 kHz.
// When the source audio already matches `target_rate`, no resampling occurs —
// avoids the quality-degrading down-then-up path for non-16 kHz backends.
STELNETTTS_API int stelnettts_audio_load_at_rate(const char* path, int target_rate, float** out_pcm, int* out_samples,
                                             int* out_sample_rate);

STELNETTTS_API void stelnettts_audio_free(float* pcm);

// ─── Stereo audio decode ─────────────────────────────────────────────
//
// Like stelnettts_audio_load but returns stereo (2-channel) PCM.
// If the source is mono, both left and right receive the same data
// and *out_channels is 1. Always resamples to 16 kHz.
STELNETTTS_API int stelnettts_audio_load_stereo(const char* path, float** out_left, float** out_right, int* out_samples,
                                            int* out_sample_rate, int* out_channels);

// ─── Parallel transcription ─────────────────────────────────────────
//
// Thin wrapper around whisper_full_parallel with g_progress tracking.
STELNETTTS_API int stelnettts_transcribe_parallel(struct whisper_context* ctx, struct whisper_full_params params,
                                              const float* samples, int n_samples, int n_processors);

// ─── DTW timestamp helpers ──────────────────────────────────────────
//
// stelnettts_ctx_params_set_dtw: configure DTW token-level timestamps
// on a whisper_context_params before context init.
// stelnettts_token_dtw_t: retrieve the DTW timestamp for a given token.
STELNETTTS_API void stelnettts_ctx_params_set_dtw(struct whisper_context_params* p, bool enable, int aheads_preset,
                                              int n_top);
STELNETTTS_API int64_t stelnettts_token_dtw_t(struct whisper_context* ctx, int i_segment, int i_token);

////////////////////////////////////////////////////////////////////////////

// Temporary helpers needed for exposing ggml interface

STELNETTTS_API int whisper_bench_memcpy(int n_threads);
STELNETTTS_API const char* whisper_bench_memcpy_str(int n_threads);
STELNETTTS_API int whisper_bench_ggml_mul_mat(int n_threads);
STELNETTTS_API const char* whisper_bench_ggml_mul_mat_str(int n_threads);

// Control logging output; default behavior is to print to stderr

STELNETTTS_API void whisper_log_set(ggml_log_callback log_callback, void* user_data);

// Get the no_speech probability for the specified segment
STELNETTTS_API float whisper_full_get_segment_no_speech_prob(struct whisper_context* ctx, int i_segment);
STELNETTTS_API float whisper_full_get_segment_no_speech_prob_from_state(struct whisper_state* state, int i_segment);
#ifdef __cplusplus
}
#endif

#endif
