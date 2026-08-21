package io.github.ggerganov.whispercpp.params;

/** Available sampling strategies */
public enum WhisperSamplingStrategy {
    /** similar to OpenAI's GreedyDecoder */
    STELNET_ASR_SAMPLING_GREEDY,

    /** similar to OpenAI's BeamSearchDecoder */
    STELNET_ASR_SAMPLING_BEAM_SEARCH
}
