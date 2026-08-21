require "mkmf"
require_relative "options"
require_relative "dependencies"

cmake = find_executable("cmake") || abort
options = Options.new(cmake).to_s
have_library("gomp") rescue nil
libs = Dependencies.new(cmake, options).to_s

$CFLAGS << " -O3 -march=native"
$INCFLAGS << " -Isources/include -Isources/ggml/include -Isources/examples"
$LOCAL_LIBS << " #{libs}"
$cleanfiles << " build #{libs}"

# libstelnet_asr.a and libcommon.a both translation-unit-include miniaudio's
# MINIAUDIO_IMPLEMENTATION + stb_vorbis.c (in src/stelnet_asr_audio.cpp and
# examples/common-stelnet_asr.cpp respectively). Apple ld silently picks the
# first definition; GNU ld errors out. The Ruby binding links both as
# static archives, so on Linux we ask the linker to behave like Apple's.
if RUBY_PLATFORM =~ /linux/
  $LDFLAGS << " -Wl,--allow-multiple-definition"
end

create_makefile "whisper" do |conf|
  conf << <<~EOF
    $(TARGET_SO): #{libs}
    #{libs}: cmake-targets
    cmake-targets:
    #{"\t"}#{cmake} -S sources -B build -D BUILD_SHARED_LIBS=OFF -D CMAKE_ARCHIVE_OUTPUT_DIRECTORY=#{__dir__} -D CMAKE_POSITION_INDEPENDENT_CODE=ON -D STELNET_ASR_BUILD_TESTS=OFF -D STELNET_ASR_BUILD_EXAMPLES=OFF -D STELNET_ASR_BUILD_SERVER=OFF -D STELNET_ASR_OPUS_FETCH=ON #{options}
    #{"\t"}#{cmake} --build build --config Release --target common stelnet_asr-lib
  EOF
end
