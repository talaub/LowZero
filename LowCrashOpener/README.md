# LowCrashOpener

Local helper for opening Misteda crash reports from `lowengine-crash://` links.

## Setup

1. Copy `LowCrashOpener.config.ps1.example` to `LowCrashOpener.config.ps1`.
2. Fill in the private admin password and adjust paths if needed.
3. Run:

```powershell
powershell -ExecutionPolicy Bypass -File .\Register-LowCrashProtocol.ps1
```

After that, links like this will download the crash zip and open the dump in Visual Studio:

```text
lowengine-crash://open?id=123
```

Some apps may show custom protocol links as plain text. In that case, paste the link into the Windows Run dialog or run:

```powershell
.\Open-LowCrash.ps1 "lowengine-crash://open?id=123"
```

## Symbols

`low package` stores the PDBs of every packaged build in a symbol store (`<store>\<name>.pdb\<GUID><age>\<name>.pdb`). The opener adds it to `_NT_SYMBOL_PATH` as `srv*<store>` before launching Visual Studio, and the debugger picks the matching PDB for each module of the dump by itself.

The store is `$SymbolRoot` from `LowCrashOpener.config.ps1`, or, if that is empty, the one `low config get symbol_store` returns. Change it with:

```powershell
low config set symbol_store D:\LowSymbols
```

## Cleanup

By default the opener waits for the Visual Studio process it launched to close, then deletes the downloaded zip and extracted crash folder. Set this in `LowCrashOpener.config.ps1` to keep files around:

```powershell
$CleanupAfterVisualStudioCloses = $false
```

## Auto Start Debugging

By default the opener also asks Visual Studio to run `Debug.Start` after opening the dump:

```powershell
$AutoStartVisualStudioDebugging = $true
$VisualStudioAutoDebugCommand = "Debug.Start"
```

If Visual Studio does not pick the native dump action correctly on your machine, turn it off:

```powershell
$AutoStartVisualStudioDebugging = $false
```
