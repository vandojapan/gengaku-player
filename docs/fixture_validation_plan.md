# Fixture Validation Plan

Purpose: validate existing EasyRPG behavior for target-game Maniacs commands that are already SPEC_KNOWN. No implementation changes are part of this plan.

Target source XML: `Games/GengakuSyoujo_WF/*.emu`, `Games/GengakuSyoujo_WF/RPG_RT.edb`

Execution status: fixtures are defined from the target XML. Runtime validation still needs a runnable EasyRPG Player build and reference Maniacs/RPG_RT captures/logs.

## Priority 1: 3007 `Maniac_ShowStringPicture`

Existing implementation:

- Dispatch: `src/game_interpreter.cpp:796`
- Function: `src/game_interpreter.cpp:4568`

Minimal fixtures from target XML:

| fixture | source location | setup commands | command under test | follow-up commands | validation target |
|---|---|---|---|---|---|
| 3007-title-text | `Map0007.emu:map=7:event=0001:page=0001:cmd=15` | `ControlVars 0 1 0 0 19 4 70 2 1`; `ControlVars 0 2 0 0 19 4 71 2 1` | `3007 params=16 1 1 2 100 0 100 100 100 100 0 0 0 94208 65536 10 0 1 0 0 10`, string `- Aoi Level -`, font `ms gothic` | `Loop`; score/key prompt flow | Picture 1 renders text at expected position, size, alpha, layer, and font fallback. |
| 3007-score-text | `Map0001.emu:map=1:event=0001:page=0001:cmd=9` | Set variables 73, 74, 75 to deterministic values. | `3007 params=16 2 1 2 100 0 100 100 100 100 0 0 0 126976 65537 9 0 1 0 0 10`, string contains `\v[75]`, font `ms gothic` | `ControlVars 0 1 0 2 0 32` | Variable escape `\v[75]` renders exactly as Maniacs; picture 2 dimensions and transparency match. |
| 3007-empty-buffer | `RPG_RT.edb:common_event=20:cmd=5` | Set variables 70/71 and ensure picture 999 is free. | `3007 params=16 999 1 2 100 50 100 100 100 100 0 0 0 90112 0 10 0 1 320 240 10`, empty string fields with font `ms gothic` | `3008` on picture 999, then `3021/3025/3026` flow when reference exists | Empty string picture creates a valid 320x240 backing bitmap. This fixture is critical because later image export depends on it. |

EasyRPG validation:

1. Run each fixture in EasyRPG with Maniacs patch mode detected/enabled.
2. Capture screenshot after the command and after any `MovePicture`.
3. Dump picture state: ID, current X/Y, width/height, opacity, layer, string text, and pending-load state.
4. Compare screenshots against Maniacs captures using `tools/compare_images.py`.

Pass criteria:

- No unsupported-command or bad-text warnings.
- Empty string picture remains a real picture with exportable bitmap.
- Pixel differences are explainable only by font rasterization differences; if target-game reproduction requires exact text pixels, font substitution must be resolved before marking pass.

## Priority 2: 3008 `Maniac_GetPictureInfo`

Existing implementation:

- Dispatch: `src/game_interpreter.cpp:798`
- Function: `src/game_interpreter.cpp:4731`

Minimal fixtures from target XML:

| fixture | source location | setup commands | command under test | expected output variables |
|---|---|---|---|---|
| 3008-picture-1-current | `RPG_RT.edb:common_event=3:cmd=1`; also common event 11 | Create/show picture 1 with known bitmap and position. | `3008 params=0 0 0 1 1 2 3 3` | Variables 1,2,3,4 receive X,Y,width,height for picture 1. |
| 3008-picture-999-current | `RPG_RT.edb:common_event=20:cmd=6` | Run `3007-empty-buffer`. | `3008 params=0 0 0 999 1 2 3 4` | Variables 1,2,3,4 receive X,Y,width,height for picture 999. |

EasyRPG validation:

1. After picture creation, run `3008` before and after one frame to catch pending-load behavior.
2. Dump output variables immediately after command completion.
3. Compare variable logs with Maniacs.

Pass criteria:

- Output values match Maniacs for center/current geometry mode.
- If image loading is pending, EasyRPG repeats/yields until dimensions are available, matching command timing closely enough for later logic.

## Priority 3: 3015 `Maniac_RewriteMap`

Existing implementation:

- Dispatch: `src/game_interpreter.cpp:804`
- Function: `src/game_interpreter.cpp:4929`
- Tile helpers: `src/spriteset_map.cpp`

Minimal fixtures from target XML:

| fixture | source location | setup | command under test | validation target |
|---|---|---|---|---|
| 3015-upper-lower-pair | `RPG_RT.edb:common_event=4:cmd=14-15` | Set variables 3 and 4 to values that enter both branches. | `3015 params=272 0 0 0 11 4 1 1 0`; `3015 params=272 0 1 0 11 4 1 1 0` | Lower and upper layer one-tile rewrites at x/y derived from variables. |
| 3015-coin | `RPG_RT.edb:common_event=9:cmd=387`, comments `Coin1`/`Coin2` | Set coin collision variables to trigger branch. | `3015 params=272 0 0 0 1 2 1 1 0` | Coin tile disappears/changes exactly as Maniacs. |
| 3015-goal | `RPG_RT.edb:common_event=9:cmd=398`, comments `goal` | Set goal variables to trigger branch. | `3015 params=272 0 0 0 1 2 1 1 0` | Goal tile change and later branch behavior match Maniacs. |

EasyRPG validation:

1. Before command, dump visible tile IDs at target coordinates and render screenshot.
2. Run command, then dump tile IDs and screenshot on the same frame and next frame.
3. If possible, query collision/passability at rewritten tile.

Pass criteria:

- Rendered tile change matches Maniacs.
- `GetGameInfo` tile info sees the same changed tile where the game relies on it.
- No unobserved replace-range semantics are needed for the target fixture.

## Priority 4: 3020 `Maniac_ControlStrings`

Existing implementation:

- Dispatch: `src/game_interpreter.cpp:812`
- Function: `src/game_interpreter.cpp:5052`
- Storage/helpers: `src/game_strings.*`, `src/game_interpreter_shared.cpp`

Minimal fixtures from target XML:

| fixture | source location | setup | command under test | expected strings |
|---|---|---|---|---|
| 3020-result-text-clear | `RPG_RT.edb:common_event=11:cmd=76-78` | Set branch variable 3 to clear-result path. | `3020 params=0 1 0 0 15 0 0`, string `Let's move on !`; `3020 params=0 2 0 0 17 0 0`, string `illust : lunetuki`; `3020 params=0 3 0 0 13 0 0`, string `Stage Clear!!` | String vars 1,2,3 match exactly. |
| 3020-result-text-final | `RPG_RT.edb:common_event=11:cmd=82-84` | Set variable 75 to deterministic score. | Includes `3020 params=0 3 0 262144 38 0 0`, string `Thank you for Playing!!\nScore : \v[75]` | Escape preservation/expansion behavior matches Maniacs when later consumed by `3007`. |
| 3020-filename | `RPG_RT.edb:common_event=20:cmd=12,15,20,22` | Set variables 10 and 11 to known numeric suffixes. | `3020 params=0 1 0 0 10 0 0`, strings `Result\FF_` or `Result\IL_`; then `3020 params=16 1 0 257 3 0 0` twice | String var 1 becomes the exact output filename base expected by `3026`. |

EasyRPG validation:

1. Dump Maniacs string variables after every command.
2. Dump EasyRPG string variables at the same command indices.
3. Compare byte-for-byte, including backslashes, newlines, and escape sequences.

Pass criteria:

- String values match exactly at each checkpoint.
- Filename produced for `3026` matches Maniacs path/base name before extension handling.

## Priority 5: 3026 `Maniac_WritePicture`

Existing implementation:

- Dispatch: `src/game_interpreter.cpp:814`
- Function: `src/game_interpreter.cpp:5346`
- Screen capture: `src/baseui.cpp`
- PNG output: `Bitmap::WritePNG`

Minimal fixture from target XML:

| fixture | source location | setup | command under test | validation target |
|---|---|---|---|---|
| 3026-result-picture-save | `RPG_RT.edb:common_event=20:cmd=25` | Run `3007-empty-buffer`, `3008-picture-999-current`, `3020-filename`. Full validation also requires reference-defined `3021 subcommand 3` and `3025`. | `3026 params=16 1 999 1` | Save picture 999 to the same PNG path/name and pixel content as Maniacs. |

EasyRPG validation:

1. Run the fixture after string filename setup.
2. Record output path, filename, dimensions, color type/alpha, and file hash.
3. Compare against Maniacs PNG using `tools/compare_images.py`.

Pass criteria:

- Filename and location match Maniacs.
- PNG dimensions match.
- Pixel-for-pixel match after upstream `3021`/`3025` behavior is implemented from reference logs.

## Validation Harness Notes

The fixtures above can be implemented as a small temporary RPG_RT project or injected common events generated from the target XML snippets. Keep the fixture commands minimal and preserve the exact observed parameter arrays. Recommended logging points:

- command index before and after command under test
- variables touched by the fixture
- string variables touched by the fixture
- picture state for IDs 1, 2, 5, 9, 10, 11, 12, 201, 999, and 1000 as relevant
- screenshot/PNG path for each capture

Do not mark a SPEC_KNOWN command as target-validated until the fixture has been run in both Maniacs/RPG_RT and EasyRPG and the artifacts have been compared.
