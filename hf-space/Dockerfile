FROM ubuntu:24.04
WORKDIR /space

RUN printf 'Acquire::Retries "5";\nAcquire::http::Timeout "30";\nAcquire::https::Timeout "30";\n' \
      > /etc/apt/apt.conf.d/80-retries && \
  apt-get update && \
  apt-get install -y --fix-missing bash curl ffmpeg python3 python3-pip \
      libomp5 libgomp1 && \
  rm -rf /var/lib/apt/lists/* /var/cache/apt/archives/*

# Pre-built binaries from GitHub Releases — avoids the 6+ minute C++
# compilation that exceeds the HF free-tier Docker build timeout.
# v3: tarball with SOVERSION symlinks now includes lib symlinks (libstelnet_asr.so → libstelnet_asr.so.0.7.1 etc.)
ARG BIN_URL=https://github.com/CrispStrobe/StelnetASR/releases/download/hf-space-bin/stelnet_asr-bin-linux-x64.tar.gz?v=4
RUN mkdir -p /opt/stelnet_asr/build/bin && \
  curl -sL "${BIN_URL}" -o /tmp/stelnet_asr-bin.tar.gz && \
  tar xzf /tmp/stelnet_asr-bin.tar.gz -C /opt/stelnet_asr/build/bin/ && \
  rm /tmp/stelnet_asr-bin.tar.gz && \
  find /opt/stelnet_asr/build/bin -name 'lib*.so*' -exec cp -a {} /usr/local/lib/ \; && \
  ldconfig

# Pre-download G2P dicts + kokoro model so TTS works on first call.
RUN mkdir -p /cache/.cache/stelnet_asr && \
  curl -sL "https://huggingface.co/datasets/cstr/g2p-dicts/resolve/main/espeak_en.tsv" \
       -o /cache/.cache/stelnet_asr/espeak_en.tsv && \
  curl -sL "https://raw.githubusercontent.com/cmusphinx/cmudict/refs/heads/master/cmudict.dict" \
       -o /cache/.cache/stelnet_asr/cmudict.dict && \
  curl -sL "https://huggingface.co/cstr/kokoro-82m-GGUF/resolve/main/kokoro-82m-q8_0.gguf" \
       -o /cache/kokoro-82m-q8_0.gguf && \
  curl -sL "https://huggingface.co/cstr/kokoro-voices-GGUF/resolve/main/kokoro-voice-af_heart.gguf" \
       -o /cache/kokoro-voice-af_heart.gguf && \
  ls -lh /cache/ /cache/.cache/stelnet_asr/ || true

# Fetch bundled sample audio from the GitHub repo (optional).
ARG STELNET_ASR_REPO=https://github.com/CrispStrobe/StelnetASR
RUN mkdir -p /space/samples && \
  curl -sL "${STELNET_ASR_REPO}/raw/main/samples/jfk.wav" -o /space/samples/jfk.wav 2>/dev/null || true

COPY requirements.txt /space/requirements.txt
RUN pip3 install --no-cache-dir --break-system-packages -r /space/requirements.txt

COPY app.py start.sh /space/
RUN chmod +x /space/start.sh && \
  id -u 1000 >/dev/null 2>&1 || useradd -m -u 1000 app && \
  mkdir -p /models /cache && \
  chown -R 1000:1000 /space /opt/stelnet_asr /models /cache

ENV PATH=/opt/stelnet_asr/build/bin:$PATH
ENV HOME=/cache
ENV STELNET_ASR_SERVER_URL=http://127.0.0.1:8080
ENV STELNET_ASR_CACHE_DIR=/cache
ENV STELNET_ASR_SAMPLES_DIR=/space/samples
ENV GRADIO_SERVER_NAME=0.0.0.0
ENV GRADIO_SERVER_PORT=7860

USER 1000
EXPOSE 7860
EXPOSE 8080
ENTRYPOINT ["/space/start.sh"]
