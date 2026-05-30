# Reference Logging Plan

Purpose: define the RPG_RT/Maniacs observations required before implementing any SPEC_NEEDS_REFERENCE behavior. This is intentionally not an implementation plan.

Priority order:

1. `3018 Maniac_SetGameOption`
2. `3021 Maniac_GetGameInfo` subcommand `3`
3. `3025 Maniac_EditPicture`
4. `3032 Maniac_Zoom`

## General Logging Format

For each observation run, capture:

- exact command index and source location
- command parameters and command string
- current map ID, event ID, page, and common event stack
- current frame number or timestamp
- variables touched by the command and by the next five commands
- string variables touched by the command and by the next five commands
- picture state for any affected picture IDs
- screenshot before command, immediately after command, and after the next rendered frame
- final PNG output when the flow reaches `3026`

Prefer CSV or JSON logs with one row/object per command boundary. Screenshots and PNG outputs should be named with command ID, source location, and frame number.

## Priority 6: 3018 `Maniac_SetGameOption`

Observed target XML:

- `Map0007.emu:map=7:event=0001:page=0001:cmd=1`
- `params=0 1 60 0`
- Next commands: `MoveEvent 10001 8 1 0 23`, `ChangeMainMenuAccess 0`, `Maniac_GetGameInfo 0 2 70`

Current EasyRPG state:

- Dispatch exists at `src/game_interpreter.cpp:810`.
- `CommandManiacSetGameOption` exists at `src/game_interpreter.cpp:5034`.
- Only operation `2` is handled. Observed operation `1` is currently unsupported.

Reference questions:

| item | observation |
|---|---|
| Parameter meaning | Does parameter 1 select option ID? Does parameter 2 provide the value? Is parameter 0 a value-mode bitfield? |
| Option identity | Does operation `1` change FPS, frame skip, FatalMix, inactive/background behavior, picture limit, wait timing, message/input behavior, or another runtime option? |
| Timing | Does the command take effect immediately on the same frame, next frame, or after map start finishes? |
| Persistence | Does the option persist across map transfer, save/load, or return to title? |
| Target-game effect | Does Map0007 player movement, wait timing, or key input timing differ when this command is removed in Maniacs? |

Required artifacts:

- frame timestamps for 300 frames before/after command
- player/event movement position per frame after the following `MoveEvent`
- wait command duration logs after command
- any visible FPS/frame-skip indicator if available
- screenshots before command and after Map0007 startup settles

Implementation gate:

- Do not implement operation `1` until the option identity and value semantics of `60` are confirmed.

## Priority 7: 3021 `Maniac_GetGameInfo` Subcommand 3

Observed target XML:

- `RPG_RT.edb:common_event=20:cmd=9`
- `params=4369 3 0 1 2 3 4 99999`
- Previous: `ControlVars ...`
- Next: `3025 Maniac_EditPicture params=0 999 0 0 320 240 99999`

Current EasyRPG state:

- Dispatch exists at `src/game_interpreter.cpp:818`.
- `CommandManiacGetGameInfo` exists at `src/game_interpreter.cpp:4199`.
- Subcommands `0`, `1`, and `2` are implemented.
- Subcommand `3` is explicitly marked as pixel-info FIXME at `src/game_interpreter.cpp:4236`.

Reference questions:

| item | observation |
|---|---|
| Source surface | Does pixel info sample from screen framebuffer, map layer, picture, or another render target? |
| Parameter map | For `4369 3 0 1 2 3 4 99999`, identify mode bitfield, x, y, width, height, and output base. |
| Output layout | How many variables are written for a 320x240 area? Are values row-major? |
| Pixel encoding | Determine RGB/RGBA order, alpha/transparency representation, signedness, and any palette/system color handling. |
| Timing | Does sampling include picture 999 created by preceding `3007`, and is it before or after pending renders resolve? |

Required artifacts:

- controlled scene with solid red/green/blue/transparent regions
- variable dump beginning at 99999 after command
- screenshot at sampling frame
- repeat with picture overlays and without picture overlays
- repeat immediately before `3025`

Implementation gate:

- Do not implement pixel-info until variable layout and color packing are confirmed.

## Priority 8: 3025 `Maniac_EditPicture`

Observed target XML:

- `RPG_RT.edb:common_event=20:cmd=10`
- `params=0 999 0 0 320 240 99999`
- Previous: `3021 Maniac_GetGameInfo params=4369 3 0 1 2 3 4 99999`
- Next: branch on variable 89 and filename setup, eventually `3026`.

Current EasyRPG state:

- No `case Cmd::Maniac_EditPicture` found.
- No `CommandManiacEditPicture` found.

Reference questions:

| item | observation |
|---|---|
| Target | Does parameter 1 mean picture ID 999? Can it be variable/indirect? |
| Rectangle | Confirm x=0, y=0, width=320, height=240 for observed params. |
| Source | Confirm source variable base 99999 and relation to `3021` pixel-info output. |
| Mutation model | Does it mutate picture bitmap in-place, sprite source, window/string picture backing bitmap, or cached picture resource? |
| Pixel behavior | Confirm color packing, alpha, clipping, transparent color, and whether dynamic effects are baked. |
| Export interaction | Confirm that following `3026` writes the edited bitmap, not the original string-picture bitmap. |

Required artifacts:

- before/after capture of picture 999
- output PNG after `3026`
- variable dump from `3021`
- exact file hash and dimensions of Maniacs PNG
- pixel comparison against controlled expected pattern

Implementation gate:

- Do not add dispatch or bitmap mutation code until the mutation model and pixel format are confirmed.

## Priority 9: 3032 `Maniac_Zoom`

Observed target XML:

- `Map0001.emu:cmd=5 params=17 1 2 100 0 7 0`, after `ShowPicture Logo`
- `RPG_RT.edb:common_event=10:cmd=53 params=17 1 2 200 1 8 0`
- `RPG_RT.edb:common_event=11:cmd=12 params=17 1 2 200 31 7 0`, after `3008 GetPictureInfo`

Current EasyRPG state:

- No `case Cmd::Maniac_Zoom` found.
- No `CommandManiacZoom` found.
- Existing UI/window zoom, sprite zoom, and transition zoom code must not be reused unless reference behavior proves a match.

Reference questions:

| item | observation |
|---|---|
| Target domain | Does it zoom the screen, map camera, viewport, picture, or render output? |
| Parameter map | Identify mode bitfield, target ID, x/y anchor, zoom percent, duration, easing, wait flag, and reset behavior for each observed pattern. |
| Timing | Does the command wait for zoom completion or start an async effect? |
| Logical effects | Does zoom affect input coordinates, collision, `GetPictureInfo`, screen position variables, or only rendering? |
| Anchor | Are variables 7/8 used as coordinates, outputs, or mode selectors? |

Required artifacts:

- frame-by-frame screenshots before/during/after each observed zoom
- command timing logs
- camera position variables and screen position variables before/after
- picture state before/after
- replay around Logo, common event 10, and common event 11 contexts

Implementation gate:

- Do not implement `3032` until target domain and timing semantics are confirmed.

## Artifact Naming

Suggested directory layout:

```text
reference_artifacts/
  maniacs/
    3018_map0007_start/
    3021_3025_3026_image_flow/
    3032_zoom/
  easyrpg/
    3007_show_string_picture/
    3008_get_picture_info/
    3015_rewrite_map/
    3020_control_strings/
    3026_write_picture/
```

Suggested file names:

- `commands.csv`
- `variables_before.csv`
- `variables_after.csv`
- `strings_before.csv`
- `strings_after.csv`
- `pictures_before.csv`
- `pictures_after.csv`
- `screen_before.png`
- `screen_after_frame000.png`
- `screen_after_frameNNN.png`
- `output.png`

Use `tools/compare_images.py` for pixel comparison.
