package io.github.ggerganov.whispercpp.chat;

import com.sun.jna.Callback;
import com.sun.jna.IntegerType;
import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;

import java.nio.charset.Charset;

/**
 * Raw JNA declarations for the {@code stelnet_asr_chat_*} C ABI declared in
 * {@code include/stelnet_asr_chat.h} — text in, text out, separate from the ASR
 * session surface and usable on its own.
 *
 * <p>This is the unwrapped edge: every entry point takes and returns the C
 * types verbatim. Application code should use {@link ChatSession}, which owns
 * the string marshalling, the callback lifetimes and the error classification.
 * The interface is public only so tests and advanced callers can reach a symbol
 * the wrapper does not expose.
 *
 * <p>EU AI Act Art. 50(1): a product that puts this in front of a natural
 * person owes them a "you are talking to an AI" notice.
 * {@link ChatSession#aiDisclosureText()} returns the canonical wording.
 */
public interface ChatLib extends Library {

    /** The loaded {@code stelnet_asr} shared library. */
    ChatLib INSTANCE = Native.load("stelnet_asr", ChatLib.class);

    /**
     * The one error code {@code stelnet_asr_chat.h} promises as a contract: a
     * registered abort predicate stopped the run rather than the model
     * faulting. Every other non-zero value is a diagnostic aid — read the
     * message, do not switch on the number.
     */
    int STELNET_ASR_CHAT_ERR_ABORTED = 40;

    /**
     * C {@code size_t}. JNA's {@code NativeLong} is C {@code long}, which is
     * four bytes on 64-bit Windows while {@code size_t} is eight, so it cannot
     * stand in here.
     */
    class SizeT extends IntegerType {
        private static final long serialVersionUID = 1L;

        /** Zero. */
        public SizeT() {
            this(0L);
        }

        /** @param value the unsigned value to carry */
        public SizeT(long value) {
            super(Native.SIZE_T_SIZE, value, true);
        }
    }

    /**
     * {@code stelnet_asr_chat_error}. Left untouched on success; on failure
     * {@code code} is non-zero and {@code message} carries a short
     * NUL-terminated diagnostic.
     */
    @Structure.FieldOrder({ "code", "message" })
    class ChatError extends Structure {
        /** Non-zero on failure. */
        public int code;
        /** NUL-terminated diagnostic, populated on failure. */
        public byte[] message = new byte[256];

        /** @return the diagnostic decoded up to its NUL, empty when unset */
        public String messageString() {
            int n = 0;
            while (n < message.length && message[n] != 0) {
                n++;
            }
            return new String(message, 0, n, Charset.forName("UTF-8"));
        }
    }

    /**
     * {@code stelnet_asr_chat_message}. The two fields are borrowed
     * NUL-terminated UTF-8 pointers the ABI reads during the call.
     */
    @Structure.FieldOrder({ "role", "content" })
    class CMessage extends Structure {
        /** "system", "user", "assistant" or "tool". */
        public Pointer role;
        /** The turn's text. */
        public Pointer content;
    }

    /**
     * {@code stelnet_asr_chat_open_params}. The three {@code bool} fields are
     * declared as {@code byte} because C {@code bool} is one byte while JNA
     * maps a Java {@code boolean} to a four-byte int.
     *
     * <p>{@code embeddings} is present for layout only — the header says
     * "future use; keep false", so {@link ChatOpenParams} exposes no accessor
     * for it and nothing in this binding writes it.
     */
    @Structure.FieldOrder({ "n_threads", "n_threads_batch", "n_ctx", "n_batch", "n_ubatch",
            "n_gpu_layers", "use_mmap", "use_mlock", "embeddings", "chat_template" })
    class COpenParams extends Structure {
        /** Generation threads. */
        public int n_threads;
        /** Batch / prefill threads. */
        public int n_threads_batch;
        /** Context window in tokens; 0 = the model's own. */
        public int n_ctx;
        /** Logical batch size. */
        public int n_batch;
        /** Physical micro-batch size. */
        public int n_ubatch;
        /** Offloaded layers; -1 = all, 0 = CPU only. */
        public int n_gpu_layers;
        /** Map the weights rather than read them. */
        public byte use_mmap;
        /** Lock the weights into RAM. */
        public byte use_mlock;
        /** Future use; keep false. Not exposed by {@link ChatOpenParams}. */
        public byte embeddings;
        /** Template override, or NULL to read it from the GGUF. */
        public Pointer chat_template;
    }

    /** {@code stelnet_asr_chat_generate_params}. */
    @Structure.FieldOrder({ "max_tokens", "temperature", "top_k", "top_p", "min_p",
            "repeat_penalty", "repeat_last_n", "seed", "stop", "n_stop", "prefill_only" })
    class CGenerateParams extends Structure {
        /** Hard cap on tokens generated; any non-positive value takes the ABI default of 256. */
        public int max_tokens;
        /** Sampling temperature; 0.0 = greedy. */
        public float temperature;
        /** Top-k cutoff; 0 = disabled. */
        public int top_k;
        /** Nucleus cutoff; 1.0 = disabled. */
        public float top_p;
        /** Minimum-probability cutoff; 0.0 = disabled. */
        public float min_p;
        /** Repetition penalty; 1.0 = disabled. */
        public float repeat_penalty;
        /** Repetition window; -1 = ctx size, 0 = disabled. */
        public int repeat_last_n;
        /** Sampler seed, unsigned; 0xFFFFFFFF draws a random one. */
        public int seed;
        /** Borrowed array of borrowed NUL-terminated stop sequences, or NULL. */
        public Pointer stop;
        /** Length of {@link #stop}. */
        public SizeT n_stop;
        /** Prefill the prompt with assistant generation suppressed. */
        public byte prefill_only;
    }

    /**
     * {@code stelnet_asr_chat_on_token}. Fired once per detokenised chunk; the
     * pointer is valid only for the length of the callback.
     *
     * <p>The chunk is the raw byte output of the detokenizer, so a byte-fallback
     * token arrives as a single byte and a character can be split across two to
     * four calls. {@link Utf8ChunkAssembler} is what reassembles it.
     */
    interface OnTokenCallback extends Callback {
        /**
         * @param utf8Chunk NUL-terminated chunk, valid only during this call
         * @param user      the word handed to the stream entry point
         */
        void invoke(Pointer utf8Chunk, Pointer user);
    }

    /**
     * {@code stelnet_asr_chat_abort_callback}. Return 1 to LET THE GENERATION
     * CONTINUE and 0 to abort it — the header's polarity, forwarded unchanged.
     *
     * <p>Declared as {@code byte} rather than {@code boolean} to match the
     * one-byte C {@code bool} return exactly.
     */
    interface AbortCallback extends Callback {
        /**
         * @param user the word handed to {@code stelnet_asr_chat_set_abort_callback}
         * @return 1 to continue, 0 to abort
         */
        byte invoke(Pointer user);
    }

    /** Fill {@code out} with the ABI's own open defaults. */
    void stelnet_asr_chat_open_params_default(COpenParams out);

    /** Fill {@code out} with the ABI's own generate defaults. */
    void stelnet_asr_chat_generate_params_default(CGenerateParams out);

    /**
     * Open a session over a GGUF chat model; NULL on failure.
     *
     * <p>The path is a {@code Pointer}, not a {@code String}: JNA marshals a
     * String through its configured native encoding, which on the Java 8
     * target this binding supports is a legacy code page on Windows. The C
     * side reads the path as UTF-8, so it is marshalled with
     * {@link ChatNative#cstring} like every other string here.
     */
    Pointer stelnet_asr_chat_open(Pointer modelPath, COpenParams params, ChatError err);

    /** Free the session and its KV cache. */
    void stelnet_asr_chat_close(Pointer session);

    /** Clear the KV cache so the next generate re-prefills from scratch. */
    int stelnet_asr_chat_reset(Pointer session, ChatError err);

    /** One-shot generate; returns a malloc'd UTF-8 string, or NULL on failure. */
    Pointer stelnet_asr_chat_generate(Pointer session, CMessage[] messages, SizeT nMessages,
            CGenerateParams params, ChatError err);

    /** Streaming generate; 0 on clean completion. */
    int stelnet_asr_chat_generate_stream(Pointer session, CMessage[] messages, SizeT nMessages,
            CGenerateParams params, OnTokenCallback onToken, Pointer user, ChatError err);

    /** Register (or, with a NULL callback, clear) the abort predicate. */
    void stelnet_asr_chat_set_abort_callback(Pointer session, AbortCallback cb, Pointer user);

    /** Name of the chat template the session resolved against. */
    String stelnet_asr_chat_template_name(Pointer session);

    /** The context window in tokens. */
    int stelnet_asr_chat_n_ctx(Pointer session);

    /** Prompt length in tokens for {@code messages}; negative on failure. */
    int stelnet_asr_chat_count_tokens(Pointer session, CMessage[] messages, SizeT nMessages,
            ChatError err);

    /**
     * Pre-flight working-set estimate in bytes; 0 with {@code err} filled on
     * failure. Deliberately conservative — see
     * {@link ChatSession#memoryEstimate(String, ChatOpenParams)}. The path is
     * a {@code Pointer} for the reason given on {@link #stelnet_asr_chat_open}.
     */
    SizeT stelnet_asr_chat_memory_estimate(Pointer modelPath, COpenParams params, ChatError err);

    /** Free a string returned by {@link #stelnet_asr_chat_generate}. */
    void stelnet_asr_chat_string_free(Pointer s);

    /** The canonical "you are talking to an AI" wording (EU AI Act Art. 50(1)). */
    String stelnet_asr_chat_ai_disclosure_text();
}
