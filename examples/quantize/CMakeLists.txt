set(TARGET stelnettts-legacy-quantize)
add_executable(${TARGET} quantize.cpp)

include(DefaultTargetOptions)

target_link_libraries(${TARGET} PRIVATE common stelnettts-lib ${CMAKE_THREAD_LIBS_INIT})
install(TARGETS ${TARGET} RUNTIME BUNDLE DESTINATION bin)
