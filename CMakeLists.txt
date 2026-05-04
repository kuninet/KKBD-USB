cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(kkbd_usb C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(src)

# tests/ は Pico SDK に依存しないホスト側ビルド用なので
# add_subdirectory(tests) はしない。
# テストは別途 `cmake -S tests -B build-tests` で独立ビルドする。
