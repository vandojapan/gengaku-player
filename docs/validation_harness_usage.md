# Validation Harness Usage

This harness is temporary instrumentation for validating existing EasyRPG behavior against target-game fixtures. It does not implement or stub unresolved Maniacs commands.

## Scope

Watched commands:

| command_id | command |
|---|---|
| 3007 | `Maniac_ShowStringPicture` |
| 3008 | `Maniac_GetPictureInfo` |
| 3020 | `Maniac_ControlStrings` |
| 3026 | `Maniac_WritePicture` |

`3015` is intentionally left for the later tile-rewrite validation pass. `3018`, `3021`, `3025`, and `3032` are not implemented by this harness.

## Enabling

Run EasyRPG Player with:

```powershell
$env:EASYRPG_VALIDATION_LOG = "1"
.\Player.exe <path-to-fixture-or-target-game>
```

Logs are written under the game save filesystem:

```text
logs/validation/commands.jsonl
```

To also capture a screen PNG after each watched command:

```powershell
$env:EASYRPG_VALIDATION_LOG = "1"
$env:EASYRPG_VALIDATION_SCREENSHOTS = "1"
.\Player.exe <path-to-fixture-or-target-game>
```

Screenshots are written to:

```text
logs/validation/screens/
```

## JSONL Fields

Each line in `commands.jsonl` is one watched command execution.

| field | meaning |
|---|---|
| `sequence` | validation log sequence number |
| `command_id` | event command ID |
| `command_index0` / `command_index1` | zero-based and one-based command index in the active event frame |
| `map_id` | current map ID |
| `event_id` | Maniacs event/common-event ID from the active interpreter frame |
| `page_id` | Maniacs page ID when available |
| `execute_result` | return value from the command dispatcher |
| `parameters` | raw event command parameters |
| `command_string` | raw command string payload |
| `variables_touched` | variable IDs whose values changed during the command |
| `string_variables_touched` | string variable IDs changed during the command |
| `picture_state` | selected picture state after command execution |
| `screenshot_path` | captured screenshot path when screenshots are enabled |
| `artifacts` | command-produced artifacts recorded by instrumentation, currently `3026` PNG output paths |

## Fixture Workflow

1. Build EasyRPG Player with this instrumentation.
2. Run the minimal fixture for `3007`, `3008`, `3020`, or `3026` with `EASYRPG_VALIDATION_LOG=1`.
3. Save the corresponding RPG_RT/Maniacs reference logs and PNGs separately.
4. Compare EasyRPG PNG outputs or screenshots with `tools/compare_images.py`.
5. Mark the command as target-validated only when variable/string logs, picture state, and image artifacts match the reference behavior.

## Notes

- Normal gameplay is unaffected unless `EASYRPG_VALIDATION_LOG` is set.
- Commands that yield and repeat may produce multiple log lines. This is useful for validating pending-load behavior.
- `3026` records the EasyRPG output PNG path in `artifacts`; reference PNG paths must be supplied from the RPG_RT/Maniacs capture flow.
