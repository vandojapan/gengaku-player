# Windows Minimal Build

Purpose: build only a local Windows x64 Debug EasyRPG Player for target-game Maniacs validation. This does not change Player behavior and does not add any stub/no-op Maniacs handling.

## Relevant CMake Files

| file | role |
|---|---|
| `CMakeLists.txt` | Main build graph and feature options. `PLAYER_TARGET_PLATFORM` selects SDL3/SDL2/SDL1/libretro or console ports. |
| `CMakePresets.json` | Generated preset list. Do not edit directly. |
| `builds/cmake/CMakePresetsBase.json` | Base presets. `win-base` supplies the Windows vcpkg toolchain. |
| `builds/cmake/CMakePresetsUser.json` | User override hook. Use this or command-line `-D...` values for local paths/options. |
| `builds/cmake/CMakePresets.json.template` | Source template for regenerating `CMakePresets.json`. Not needed for this local build. |

The generated preset closest to the goal is:

```text
windows-x64-vs2022-sdl3-debug
```

It inherits:

- `build-sdl3`: sets `PLAYER_TARGET_PLATFORM=SDL3`
- `windows-x64-vs2022-parent`: uses generator `Visual Studio 17 2022`, architecture `x64`, and `VCPKG_TARGET_TRIPLET=x64-windows-static`
- `type-debug`: sets `CMAKE_BUILD_TYPE=Debug`

For a Visual Studio multi-config generator, still build with `--config Debug`.

This workspace also provides a short local user preset:

```text
gengaku-win64-debug
```

It inherits from `windows-x64-vs2022-sdl3-debug`, sets the local build directory to `build/gengaku-win64-debug`, and folds the validation-oriented dependency options into one preset.

## Recommended Configure

Use the simplified local preset:

```powershell
$env:VCPKG_ROOT = "C:\path\to\vcpkg"
cmake --preset gengaku-win64-debug
cmake --build --preset gengaku-win64-debug
```

If you prefer the generated preset directly, the equivalent standalone vcpkg command is:

```powershell
cmake --preset windows-x64-vs2022-sdl3-debug `
  -DCMAKE_TOOLCHAIN_FILE=C:\path\to\vcpkg\scripts\buildsystems\vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows-static `
  -DPLAYER_ENABLE_TESTS=OFF `
  -DPLAYER_ENABLE_BENCHMARKS=OFF `
  -DPLAYER_TARGET_PLATFORM=SDL3
cmake --build --preset windows-x64-vs2022-sdl3-debug --config Debug
```

Expected executable target on Windows is `Player.exe`; `CMakeLists.txt` sets the Visual Studio startup project to the SDL executable target for Windows SDL builds.

## Minimal Target Selection

Use:

```text
PLAYER_TARGET_PLATFORM=SDL3
```

This avoids the libretro path and all console-port executable paths:

- `libretro` is only selected when `PLAYER_TARGET_PLATFORM=libretro`.
- 3DS/Switch/Wii/Wii U/Vita are selected by their toolchains/platform variables or generated console presets.
- Android-specific library output is selected only under Android configuration.
- Emscripten-specific JS/WASM output is selected only when `CMAKE_SYSTEM_NAME=Emscripten`.

For this Windows local pass, do not use these presets:

- `*-libretro-*`
- `android-*`
- `emscripten-*`
- `3ds-*`
- `switch-*`
- `wii-*`
- `wiiu-*`
- `psvita-*`

No source change is needed to disable them. CMake configures only the selected preset/build tree.

## Dependencies

Required for Windows SDL3 Player:

| dependency | why |
|---|---|
| SDL3 | UI/input/audio backend when `PLAYER_TARGET_PLATFORM=SDL3` |
| liblcf | RPG Maker 2000/2003 data model and readers |
| libpng | image loading/output, needed for picture/PNG validation |
| fmt | formatting |
| pixman | bitmap/rendering operations |

Recommended to keep for Maniacs visual validation:

| option | recommendation | reason |
|---|---|---|
| `PLAYER_WITH_FREETYPE` | `ON` | Text rendering matters for `3007 Maniac_ShowStringPicture`. |
| `PLAYER_WITH_HARFBUZZ` | `ON` or default | Safer for font/text behavior; may be disabled only if dependency setup is painful and target text still renders acceptably. |

Optional dependencies that can usually be disabled for command behavior validation:

| option | suggested value | effect |
|---|---|---|
| `PLAYER_WITH_LHASA` | `OFF` | Disables LZH archive support. OK if running an unpacked game directory. |
| `PLAYER_WITH_NLOHMANN_JSON` | `OFF` | Disables EasyRPG JSON command support. Not needed for the listed SPEC_KNOWN commands. |
| `PLAYER_ENABLE_TESTS` | `OFF` | Avoids test target setup. |
| `PLAYER_ENABLE_BENCHMARKS` | `OFF` | Avoids benchmark target setup. |
| `PLAYER_ENABLE_INSTRUMENTATION` | `OFF` | Default; keep off. |
| `PLAYER_WITH_BAEKMUK` | `OFF` | Removes built-in Korean font. |
| `PLAYER_WITH_WQY` | `OFF` | Removes built-in Chinese font. |

Audio minimization options:

| option | conservative | most minimal |
|---|---|---|
| `PLAYER_AUDIO_BACKEND` | `SDL3` | `OFF` |
| `PLAYER_WITH_MPG123` | `OFF` unless MP3 playback is needed | `OFF` |
| `PLAYER_WITH_LIBSNDFILE` | `OFF` unless WAV edge cases need libsndfile | `OFF` |
| `PLAYER_WITH_OGGVORBIS` | `OFF` unless Ogg playback is needed | `OFF` |
| `PLAYER_WITH_OPUS` | `OFF` unless Opus playback is needed | `OFF` |
| `PLAYER_WITH_WILDMIDI` | `OFF` unless MIDI comparison is needed | `OFF` |
| `PLAYER_WITH_FLUIDSYNTH` | `OFF` unless MIDI comparison is needed | `OFF` |
| `PLAYER_WITH_FLUIDLITE` | `OFF` unless MIDI comparison is needed | `OFF` |
| `PLAYER_WITH_XMP` | `OFF` unless module music is needed | `OFF` |
| `PLAYER_ENABLE_DRWAV` | `ON` if WAV playback is useful without libsndfile | `OFF` |
| `PLAYER_ENABLE_FMMIDI` | `ON` if MIDI fallback is useful | `OFF` |
| `PLAYER_AUDIO_RESAMPLER` | `OFF` | `OFF` |

For Maniacs picture/string/PNG validation, audio can be disabled if the fixture does not depend on timing caused by audio playback. For whole-game local playtesting, keep `PLAYER_AUDIO_BACKEND=SDL3` and selectively disable external codecs only after confirming assets are not needed.

## Suggested Minimal Visual-Validation Configure

The `gengaku-win64-debug` preset already applies this profile. It keeps SDL3, text rendering, PNG output, and the executable, while dropping tests, archives, JSON, extra audio codecs, and bundled CJK fallback fonts.

```powershell
$env:VCPKG_ROOT = "C:\path\to\vcpkg"
cmake --preset gengaku-win64-debug
cmake --build --preset gengaku-win64-debug
```

## Ultra-Minimal No-Audio Configure

Use this only for isolated command fixtures where audio is irrelevant:

```powershell
cmake --preset windows-x64-vs2022-sdl3-debug `
  -DCMAKE_TOOLCHAIN_FILE=C:\path\to\vcpkg\scripts\buildsystems\vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows-static `
  -DPLAYER_TARGET_PLATFORM=SDL3 `
  -DPLAYER_AUDIO_BACKEND=OFF `
  -DPLAYER_ENABLE_TESTS=OFF `
  -DPLAYER_ENABLE_BENCHMARKS=OFF `
  -DPLAYER_WITH_LHASA=OFF `
  -DPLAYER_WITH_NLOHMANN_JSON=OFF `
  -DPLAYER_WITH_FREETYPE=ON `
  -DPLAYER_WITH_HARFBUZZ=ON `
  -DPLAYER_WITH_BAEKMUK=OFF `
  -DPLAYER_WITH_WQY=OFF
```

Do not use this for final whole-game behavior checks unless you have confirmed audio absence cannot affect the observed flow.

## Visual Studio Debug Notes

After generating the Visual Studio solution:

- Startup project should be `easyrpg-player_exe`/`Player` from the SDL executable branch.
- Configuration: `Debug`
- Platform: `x64`
- Command arguments: path to the target game directory.
- Environment for validation logs:

```text
EASYRPG_VALIDATION_LOG=1
```

Optional screenshot capture:

```text
EASYRPG_VALIDATION_SCREENSHOTS=1
```

## What Not To Change

- Do not edit Maniacs command implementation for this build step.
- Do not add stubs/no-ops for unresolved `3018`, `3021`, `3025`, or `3032`.
- Do not edit `CMakePresets.json` directly; it is generated.
- Do not remove platform code from the repository. Select the Windows SDL3 preset instead.
