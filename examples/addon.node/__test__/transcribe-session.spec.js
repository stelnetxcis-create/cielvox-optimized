// transcribe-session.spec.js — exercises the backend-agnostic transcribeSession()
// entry point (stelnet_asr_session C-ABI), distinct from the whisper-only whisper().
//
// Model-gated: set STELNET_ASR_TEST_MODEL (+ optional STELNET_ASR_TEST_BACKEND) to a
// GGUF, or drop models/ggml-base.en.bin in place. Skips cleanly otherwise so CI
// without a model stays green.
const { join } = require('path');
const fs = require('fs');
const { promisify } = require('util');
const { transcribeSession } = require('../../../build/Release/addon.node');

const run = promisify(transcribeSession);

const MODEL = process.env.STELNET_ASR_TEST_MODEL || join(__dirname, '../../../models/ggml-base.en.bin');
const BACKEND = process.env.STELNET_ASR_TEST_BACKEND || 'whisper';
const SAMPLE = join(__dirname, '../../../samples/jfk.wav');
const haveModel = fs.existsSync(MODEL) && fs.existsSync(SAMPLE);

const maybe = haveModel ? describe : describe.skip;

maybe('transcribeSession (stelnet_asr_session C-ABI)', () => {
  test('transcribes through the session API and returns segments', async () => {
    const r = await run({
      model: MODEL,
      backend: BACKEND,
      language: 'en',
      punctuation: true,
      no_prints: true,
      fname_inp: SAMPLE,
    });
    expect(typeof r).toBe('object');
    expect(Array.isArray(r.transcription)).toBe(true);
    expect(r.transcription.length).toBeGreaterThan(0);
    const text = r.transcription.map((s) => s[2]).join(' ').toLowerCase();
    expect(text).toContain('country');
  }, 60000);
});
