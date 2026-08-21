set(TARGET cielvox2-asr-test-trace)
add_executable(${TARGET} main.cpp)
include(DefaultTargetOptions)
target_include_directories(${TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(${TARGET} PRIVATE
    cielvox2_asr
    stelnettts-diff-lib
    Threads::Threads)
