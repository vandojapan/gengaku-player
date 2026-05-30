# Static Review: Validation Harness

Scope reviewed:

- `src/validation_harness.cpp`
- `src/validation_harness.h`
- `src/game_interpreter.cpp` around the command execution hook
- `src/game_interpreter.cpp` around `Maniac_WritePicture` PNG output
- `CMakeLists.txt`
- `docs/validation_harness_usage.md`

This is a static review only. No build or runtime debugging was performed.

## Summary

The harness does not add stub/no-op behavior and does not implement unresolved Maniacs commands. The watched command filter is limited to `3007`, `3008`, `3020`, and `3026`.

With `EASYRPG_VALIDATION_LOG` unset, most harness work is skipped. One low-risk change still happens on every command: `game_interpreter.cpp` copies the current `EventCommand` before calling `ExecuteCommand()`. This should not alter interpreter state, return values, waiting, branching, or stack handling, but it is not literally zero-cost and could allocate for commands with strings/parameters.

## Findings

| severity | area | finding | impact | suggested fix |
|---|---|---|---|---|
| Low | `game_interpreter.cpp` command hook | `const auto command_before_exec = frame->commands[frame->current_command];` copies every command even when validation logging is disabled. | No semantic state change found, but normal runs gain a small allocation/copy risk and overhead. | Gate the copy behind a cheap `ValidationHarness::IsActiveFor(command.code)` helper, or pass the command by const reference and ensure `AfterCommand` only uses copied data stored in `Snapshot`. |
| Low | `validation_harness.cpp` artifact storage | `artifacts` is process-global and never pruned. | Long validation sessions with many `3026` executions can grow memory. Normal runs are unaffected because `RecordArtifactPath` returns unless logging is enabled. | After writing a command record, prune artifacts below the smallest active snapshot index, or clear after each watched command if nested/overlap is not needed. |
| Info | optional screenshots | Screenshot capture happens only after `snapshot.enabled` and `EASYRPG_VALIDATION_SCREENSHOTS` are true. | Disabled path is harmless. Enabled path can add frame-time overhead and creates PNG files, which is expected for validation. | Keep screenshot capture opt-in; current docs already state this. |

## Checklist Review

### 1. Command Hook Return/Wait/Branch Impact

The hook is placed between `index_before_exec` capture and the existing `ExecuteCommand()` call. It stores:

- command index
- Maniacs event/page IDs
- a snapshot object

Then it calls:

```cpp
const bool command_result = ExecuteCommand();
ValidationHarness::AfterCommand(validation_snapshot, command_result);
if (!command_result) {
    break;
}
```

The original control flow of `if (!ExecuteCommand()) break;` is preserved. `command_result` is used exactly for the same break condition, so return value propagation, waits, and event-loop breaking behavior are not intentionally changed.

Important detail: `AfterCommand` runs even when `ExecuteCommand()` returns `false`. That is useful for validation and does not suppress the break. If logging output fails, `AfterCommand` returns without changing interpreter state.

Residual risk: the command is copied before execution even when logging is disabled. This should not affect game semantics, but minimizing this copy would make the disabled harness path cleaner.

### 2. Watched Commands

`IsWatchedCommand` returns true only for:

- `3007`
- `3008`
- `3020`
- `3026`

`3015`, `3018`, `3021`, `3025`, and `3032` are not monitored by the current harness. No unresolved command implementation was added.

### 3. Variable/String/Picture State Side Effects

Variable snapshot:

- Reads `Main_Data::game_variables->GetData()`.
- This is a const data read/copy.

String snapshot:

- Reads `Main_Data::game_strings->GetData()`.
- This is a const data read/copy.

Picture state:

- Uses `GetPicturePtr(id)` only after filtering `id <= 0`.
- Reads `Picture::Exists()`, `IsRequestPending()`, `IsWindowAttached()`, save-picture fields, sprite dimensions, and bitmap dimensions.
- The reviewed functions are read-only: `Exists()` checks `data.name`, `IsRequestPending()` checks `request_id`, and `IsWindowAttached()` checks `data.easyrpg_type`.

No state mutation was found in these reads. The harness also avoids `GetPicture(id)`, which would create entries for missing IDs.

### 4. `3026` PNG Artifact Recording

The save path expression was split:

```cpp
auto output_path = found_file.empty() ? filename : found_file;
auto os = FileFinder::Save().OpenOutputStream(output_path);
```

This is equivalent to the previous inline expression for opening the output stream. `Bitmap::WritePNG(os)` is still called only when the output stream is valid. The added `write_success` only records the return value.

`ValidationHarness::RecordArtifactPath(...)` returns immediately unless `EASYRPG_VALIDATION_LOG` is set. It does not affect whether the PNG is opened, written, or reported as failed.

### 5. JSONL Failure Behavior

`AfterCommand` handles output failure by warning and returning:

- directory creation failure falls through to `OpenOutputStream`
- output stream failure logs `Output::Warning`
- no exception or interpreter break is introduced

Game progression should continue if log writing fails.

### 6. CMake Risk

`src/validation_harness.cpp` and `.h` were added to the main object library source list. This is the expected pattern for this project.

Platform risk appears low because the harness uses facilities already compiled into the player:

- `BaseUi::CaptureScreen`
- `Bitmap::WritePNG`
- `FileFinder::Save`
- `Game_Pictures`
- `Game_Strings`
- `Game_Variables`

`Maniac_WritePicture` already depends on screen capture and PNG writing in `game_interpreter.cpp`, so the harness does not introduce a fundamentally new subsystem dependency.

### 7. Optional Screenshot Disabled Path

`MaybeCaptureScreenshot` is called only from `AfterCommand`, and `AfterCommand` returns immediately unless the snapshot is enabled. A snapshot is enabled only when `EASYRPG_VALIDATION_LOG` is set and the command is watched.

Inside `MaybeCaptureScreenshot`, capture is skipped unless `EASYRPG_VALIDATION_SCREENSHOTS` is also set. Therefore screenshot capture is inert when disabled.

## Recommendation Before Build

The current patch is acceptable for a validation-only branch, with one cleanup recommended before merging into any longer-lived branch: avoid the unconditional `EventCommand` copy in the interpreter loop when validation logging is disabled.

No evidence was found that the harness changes Maniacs command behavior, implements unresolved commands, or masks unsupported commands with stub/no-op behavior.
