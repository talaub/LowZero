#pragma once

#include "LowMath.h"
#include "LowUtilContainers.h"

// Copies PDBs into a symbol store with the symbol server layout
//   <store>/<name>.pdb/<GUID><age>/<name>.pdb
// which Visual Studio and WinDbg read with srv*<store>. A debugger finds
// the PDB of a crash dump's module by the GUID and age the linker wrote
// into the binary, so builds never overwrite each other and PDBs that
// did not change between packages are only stored once.

namespace Low {
  namespace Cli {
    namespace Symbols {
      struct PdbReference
      {
        // As written by the linker, usually absolute in the build tree
        Util::String pdbPath;
        // <GUID><age> folder name of the store layout
        Util::String storeKey;
      };

      // False if the file is not a PE binary or was linked without
      // debug info (no CodeView RSDS entry)
      bool read_pdb_reference(const Util::String &p_Binary,
                              PdbReference &p_OutReference);

      struct StoreResult
      {
        u32 added = 0;
        u32 existing = 0;
        u64 addedBytes = 0;
        // Binaries whose PDB could not be found
        Util::List<Util::String> missing;
      };

      // Stores the PDBs of all .dll and .exe files directly inside
      // p_BinaryDirectories. Reports errors itself.
      bool store(const Util::List<Util::String> &p_BinaryDirectories,
                 const Util::String &p_Store, StoreResult &p_OutResult);
    } // namespace Symbols
  } // namespace Cli
} // namespace Low
