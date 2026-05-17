# LowEngine Debug Visualizer Extension

This is the Visual Studio Concord/Natvis extension path for LowEngine-specific
debugger formatting.

The plain `LowDebug.natvis` file handles simple field-only views and EASTL
containers. Anything that needs debugger-side logic should live here as a VSIX
debugger engine extension implementing `IDkmIntrinsicFunctionEvaluator140`.

## Current State

The repo contains:

- `LowEngineDebug.natvis`: Natvis intrinsic declarations for `Low::Util::Name`
  and `Low::Util::Handle`.
- `LowEngineIntrinsicEvaluator.cs`: the Concord evaluator skeleton.
- `LowEngineIntrinsicEvaluator.vsdconfigxml`: Concord component registration
  source.
- `source.extension.vsixmanifest`: VSIX asset declaration.

Build this project with Visual Studio 2026 or MSBuild from the VS 2026
installation. The project imports the local VS 2026 VSSDK targets and compiles
`*.vsdconfigxml` into `*.vsdconfig` as part of the build.

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" `
  ".\LowEngineDebugVisualizerExtension.csproj" `
  /t:Restore,Build,CreateVsixContainer `
  /p:Configuration=Debug `
  /m:1
```

The generated VSIX is written to `LowEngineDebugVisualizerExtension.vsix`.

## Intrinsic IDs

- `1001`: `low_name_debug_string(Low::Util::Name*)`
- `1101`: `low_handle_debug_string(void*)`
- `1102`: `low_handle_type_string(void*)`
- `1103`: `low_handle_liveness_string(void*)`
- `1104`: `low_handle_name_string(void*)`

Both intrinsics return a pseudo `char const *` with debugger-owned bytes.
