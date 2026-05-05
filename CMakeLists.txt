cmake_minimum_required(VERSION 3.13)

# CMake 4.x 互換性に関する注意：
# Pico SDK 1.5.1 同梱の TinyUSB pioasm / elf2uf2 が古い cmake_minimum_required を
# 使用しており、CMake 4.0+ で「Compatibility with CMake < 3.5 has been removed」エラーになる。
# これらは ExternalProject_Add 経由で「ビルド時」に再 configure されるため、
# CMakeLists.txt 内の set(ENV{}) では伝播しない。
# 回避策：シェル側で `export CMAKE_POLICY_VERSION_MINIMUM=3.5` を設定するか、
# 同等処理を行う `scripts/build.sh` を使用する（README 参照）。
# Pico SDK 2.x にアップグレードすれば不要（別Issueで検討）。
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0" AND
   NOT DEFINED ENV{CMAKE_POLICY_VERSION_MINIMUM})
    message(WARNING
        "CMake ${CMAKE_VERSION} detected but CMAKE_POLICY_VERSION_MINIMUM env var is not set. "
        "Pico SDK 1.5.1 sub-builds (pioasm/elf2uf2) will likely fail. "
        "Run with: export CMAKE_POLICY_VERSION_MINIMUM=3.5  "
        "(or use scripts/build.sh)")
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
