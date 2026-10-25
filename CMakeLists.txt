cmake_minimum_required(VERSION 3.21)
project(trippy C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

set(TRIPPY_VERSION "dev" CACHE STRING "Version string shown on the title screen")

# Release builds (CI) download SDL2 + its satellite libs and link them
# statically, so the .app / .exe run without Homebrew or DLLs. Local builds
# keep using the system (Homebrew / pkg-config) libraries by default.
option(TRIPPY_VENDOR_SDL "Download and statically link SDL2, SDL2_ttf, SDL2_image, SDL2_mixer" OFF)

if(TRIPPY_VENDOR_SDL)
  include(FetchContent)
  # The freetype bundled in SDL2_ttf still declares cmake_minimum_required
  # < 3.5, which CMake 4 (GitHub runners) refuses without this.
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  set(SDL_TEST OFF CACHE BOOL "" FORCE)
  # Satellites: only built-in decoders (stb PNG, stb_vorbis, minimp3, WAV),
  # freetype from the SDL2_ttf release tarball.
  foreach(opt SDL2TTF_SAMPLES SDL2TTF_INSTALL SDL2TTF_HARFBUZZ
              SDL2IMAGE_SAMPLES SDL2IMAGE_INSTALL SDL2IMAGE_VENDORED SDL2IMAGE_AVIF
              SDL2IMAGE_JXL SDL2IMAGE_TIF SDL2IMAGE_WEBP SDL2IMAGE_BACKEND_IMAGEIO
              SDL2IMAGE_BACKEND_WIC SDL2IMAGE_DEPS_SHARED
              SDL2MIXER_SAMPLES SDL2MIXER_INSTALL SDL2MIXER_VENDORED SDL2MIXER_OPUS
              SDL2MIXER_FLAC SDL2MIXER_MOD SDL2MIXER_MIDI SDL2MIXER_WAVPACK
              SDL2MIXER_GME SDL2MIXER_DEPS_SHARED)
    set(${opt} OFF CACHE BOOL "" FORCE)
  endforeach()
  set(SDL2TTF_VENDORED ON CACHE BOOL "" FORCE)
  set(SDL2IMAGE_BACKEND_STB ON CACHE BOOL "" FORCE)

  FetchContent_Declare(SDL2
    URL https://github.com/libsdl-org/SDL/releases/download/release-2.30.9/SDL2-2.30.9.tar.gz)
  FetchContent_Declare(SDL2_ttf
    URL https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.22.0/SDL2_ttf-2.22.0.tar.gz)
  FetchContent_Declare(SDL2_image
    URL https://github.com/libsdl-org/SDL_image/releases/download/release-2.8.2/SDL2_image-2.8.2.tar.gz)
  FetchContent_Declare(SDL2_mixer
    URL https://github.com/libsdl-org/SDL_mixer/releases/download/release-2.8.0/SDL2_mixer-2.8.0.tar.gz)
  FetchContent_MakeAvailable(SDL2)
  FetchContent_MakeAvailable(SDL2_ttf SDL2_image SDL2_mixer)

  set(TRIPPY_SDL_LIBS SDL2::SDL2-static SDL2_ttf::SDL2_ttf-static
      SDL2_image::SDL2_image-static SDL2_mixer::SDL2_mixer-static)
  if(WIN32)
    list(PREPEND TRIPPY_SDL_LIBS SDL2::SDL2main)
  endif()
else()
  # Homebrew on Apple Silicon keeps everything under /opt/homebrew.
  if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH /opt/homebrew)
  endif()

  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
  pkg_check_modules(SDL2_TTF REQUIRED IMPORTED_TARGET SDL2_ttf)
  pkg_check_modules(SDL2_IMAGE REQUIRED IMPORTED_TARGET SDL2_image)
  pkg_check_modules(SDL2_MIXER REQUIRED IMPORTED_TARGET SDL2_mixer)
  set(TRIPPY_SDL_LIBS PkgConfig::SDL2 PkgConfig::SDL2_TTF
      PkgConfig::SDL2_IMAGE PkgConfig::SDL2_MIXER)
endif()

add_executable(trippy
  src/main.cpp
  src/Game.cpp
  src/Renderer.cpp
  src/Input.cpp
  src/Room.cpp
  src/RoomLoader.cpp
  src/Player.cpp
  src/Entity.cpp
  src/NPC.cpp
  src/UFO.cpp
  src/TripFX.cpp
  src/Dialogue.cpp
  src/Inventory.cpp
  src/SaveGame.cpp
  src/Audio.cpp
  src/ChiptuneSynth.cpp
  src/Assets.cpp
)

target_link_libraries(trippy PRIVATE ${TRIPPY_SDL_LIBS})
target_compile_definitions(trippy PRIVATE TRIPPY_VERSION="${TRIPPY_VERSION}" _USE_MATH_DEFINES)

if(MSVC)
  target_compile_options(trippy PRIVATE /W3 /utf-8)
  set_target_properties(trippy PROPERTIES WIN32_EXECUTABLE TRUE)
else()
  target_compile_options(trippy PRIVATE -Wall -Wextra)
endif()
