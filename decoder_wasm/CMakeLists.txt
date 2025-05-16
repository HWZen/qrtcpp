cmake_minimum_required(VERSION 3.18...3.28)
project(decoder_wasm)

set(CMAKE_CXX_STANDARD 20)

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".js")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -sMODULARIZE -sEXPORTED_RUNTIME_METHODS=ccall")
endif()

add_library(decoder_wasm SHARED decoder_wasm.cpp)

target_include_directories(decoder_wasm PRIVATE ./)
target_link_libraries(decoder_wasm PRIVATE
    libwirehare.a
)

