package io.github.ggerganov.whispercpp.chat;

/**
 * A {@code stelnet_asr_chat_*} entry point failed. {@link #code()} carries what
 * the C side reported.
 *
 * <p>Only {@link ChatAbortedException} may be identified by its code: the
 * header promises {@code STELNET_ASR_CHAT_ERR_ABORTED} as a stable contract and
 * says every other value is a diagnostic aid. Read {@link #getMessage()} for
 * those, do not switch on the number.
 */
public class ChatException extends RuntimeException {

    private static final long serialVersionUID = 1L;

    private final int code;

    /**
     * @param message the C diagnostic, or a fallback when C supplied none
     * @param code    the reported error code
     */
    public ChatException(String message, int code) {
        super(message);
        this.code = code;
    }

    /** @return the error code the C side reported; 0 when it reported none */
    public int code() {
        return code;
    }
}
