if (STELNETTTS_SDL2)
    set(TARGET stelnettts-stream)
    add_executable(${TARGET} stream.cpp)

    include(DefaultTargetOptions)

    target_link_libraries(${TARGET} PRIVATE common common-sdl whisper ${CMAKE_THREAD_LIBS_INIT})

    install(TARGETS ${TARGET} RUNTIME BUNDLE DESTINATION bin)
endif ()
