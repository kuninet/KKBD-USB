cmake_minimum_required(VERSION 3.13)

# CMake 4.x 互換性対応：
# Pico SDK 1.5.1 同梱の TinyUSB pioasm / elf2uf2 が古い cmake_minimum_required を
# 使用しており、CMake 4.0+ で「Compatibility with CMake < 3.5 has been removed」エラーになる。
# 環境変数として設定することで ExternalProject_Add の子CMake呼び出しに伝播させる。
# Pico SDK 2.x にアップグレードすれば不要（Phase 1 PR では SDK 1.5.1 のまま運用）。
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
    set(ENV{CMAKE_POLICY_VERSION_MINIMUM} 3.5)
endif()

include(pico_sdk_import.cmake)

project(kkbd_usb C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(src)

# tests/ は Pico SDK に依存しないホスト側ビルド用なので
# add_subdirectory(tests) はしない。
# テストは別途 `cmake -S tests -B build-tests` で独立ビルドする。
