#include "LowCliSymbols.h"

#include "LowCliCommands.h"
#include "LowCliOutput.h"

#include <ctype.h>
#include <filesystem>
#include <fstream>
#include <stdio.h>
#include <string.h>

// PE layout reference: "PE Format" in the Microsoft docs. All values are
// little endian, which is also what we run on, so they are read directly.

namespace Low {
  namespace Cli {
    namespace Symbols {
      namespace fs = std::filesystem;

      // Offset of e_lfanew (file offset of the PE signature) in the DOS
      // header
      static const u64 g_DosPeOffsetField = 0x3C;
      static const u32 g_PeSignature = 0x00004550; // "PE\0\0"
      static const u64 g_CoffHeaderSize = 20;
      static const u16 g_Pe32Magic = 0x10B;
      static const u16 g_Pe32PlusMagic = 0x20B;
      static const u32 g_DebugDirectoryIndex = 6;
      static const u64 g_SectionHeaderSize = 40;
      static const u64 g_DebugEntrySize = 28;
      static const u32 g_DebugTypeCodeView = 2;
      static const u32 g_RsdsSignature = 0x53445352; // "RSDS"
      // "RSDS" + GUID + age, followed by the zero terminated PDB path
      static const u32 g_RsdsHeaderSize = 4 + 16 + 4;
      // Header plus a path, anything bigger is a broken file
      static const u32 g_MaxCodeViewSize = 64 * 1024;

      static bool read_at(std::ifstream &p_File, u64 p_Offset, void *p_Out,
                          size_t p_Size)
      {
        p_File.clear();
        p_File.seekg(static_cast<std::streamoff>(p_Offset));
        p_File.read(static_cast<char *>(p_Out),
                    static_cast<std::streamsize>(p_Size));
        return p_File.good() &&
               p_File.gcount() == static_cast<std::streamsize>(p_Size);
      }

      template <typename T>
      static bool read_value(std::ifstream &p_File, u64 p_Offset,
                             T &p_Out)
      {
        return read_at(p_File, p_Offset, &p_Out, sizeof(T));
      }

      // The debug directory is given as a virtual address, the section
      // that contains it says where that is in the file
      static bool rva_to_file_offset(std::ifstream &p_File,
                                     u64 p_SectionTable, u16 p_SectionCount,
                                     u32 p_Rva, u64 &p_OutOffset)
      {
        for (u16 i = 0; i < p_SectionCount; ++i) {
          const u64 l_Header = p_SectionTable + i * g_SectionHeaderSize;
          u32 l_VirtualSize = 0;
          u32 l_VirtualAddress = 0;
          u32 l_RawSize = 0;
          u32 l_RawPointer = 0;
          if (!read_value(p_File, l_Header + 8, l_VirtualSize) ||
              !read_value(p_File, l_Header + 12, l_VirtualAddress) ||
              !read_value(p_File, l_Header + 16, l_RawSize) ||
              !read_value(p_File, l_Header + 20, l_RawPointer)) {
            return false;
          }

          const u32 l_Size =
              l_VirtualSize > l_RawSize ? l_VirtualSize : l_RawSize;
          if (p_Rva >= l_VirtualAddress &&
              p_Rva < l_VirtualAddress + l_Size) {
            p_OutOffset = static_cast<u64>(p_Rva - l_VirtualAddress) +
                          l_RawPointer;
            return true;
          }
        }
        return false;
      }

      // <GUID><age> exactly like symstore and the debuggers expect it:
      // the GUID fields in uppercase hex without dashes, the age in hex
      // without leading zeros
      static Util::String make_store_key(const u8 *p_Guid, u32 p_Age)
      {
        u32 l_Data1 = 0;
        u16 l_Data2 = 0;
        u16 l_Data3 = 0;
        memcpy(&l_Data1, p_Guid, 4);
        memcpy(&l_Data2, p_Guid + 4, 2);
        memcpy(&l_Data3, p_Guid + 6, 2);

        char l_Buffer[64];
        snprintf(l_Buffer, sizeof(l_Buffer),
                 "%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X%X",
                 l_Data1, l_Data2, l_Data3, p_Guid[8], p_Guid[9],
                 p_Guid[10], p_Guid[11], p_Guid[12], p_Guid[13],
                 p_Guid[14], p_Guid[15], p_Age);
        return l_Buffer;
      }

      bool read_pdb_reference(const Util::String &p_Binary,
                              PdbReference &p_OutReference)
      {
        std::ifstream l_File(to_path(p_Binary), std::ios::binary);
        if (!l_File) {
          return false;
        }

        u32 l_PeOffset = 0;
        u32 l_Signature = 0;
        if (!read_value(l_File, g_DosPeOffsetField, l_PeOffset) ||
            !read_value(l_File, l_PeOffset, l_Signature) ||
            l_Signature != g_PeSignature) {
          return false;
        }

        const u64 l_CoffHeader = static_cast<u64>(l_PeOffset) + 4;
        u16 l_SectionCount = 0;
        u16 l_OptionalHeaderSize = 0;
        if (!read_value(l_File, l_CoffHeader + 2, l_SectionCount) ||
            !read_value(l_File, l_CoffHeader + 16,
                        l_OptionalHeaderSize)) {
          return false;
        }

        // PE32 and PE32+ (64 bit) only differ in where the data
        // directories start
        const u64 l_OptionalHeader = l_CoffHeader + g_CoffHeaderSize;
        u16 l_Magic = 0;
        if (!read_value(l_File, l_OptionalHeader, l_Magic)) {
          return false;
        }
        u64 l_DirectoryCountField = 0;
        u64 l_DataDirectories = 0;
        if (l_Magic == g_Pe32PlusMagic) {
          l_DirectoryCountField = l_OptionalHeader + 108;
          l_DataDirectories = l_OptionalHeader + 112;
        } else if (l_Magic == g_Pe32Magic) {
          l_DirectoryCountField = l_OptionalHeader + 92;
          l_DataDirectories = l_OptionalHeader + 96;
        } else {
          return false;
        }

        u32 l_DirectoryCount = 0;
        if (!read_value(l_File, l_DirectoryCountField,
                        l_DirectoryCount) ||
            l_DirectoryCount <= g_DebugDirectoryIndex) {
          return false;
        }

        // Each data directory is an RVA and a size
        u32 l_DebugRva = 0;
        u32 l_DebugSize = 0;
        const u64 l_DebugDirectory =
            l_DataDirectories + g_DebugDirectoryIndex * 8;
        if (!read_value(l_File, l_DebugDirectory, l_DebugRva) ||
            !read_value(l_File, l_DebugDirectory + 4, l_DebugSize) ||
            l_DebugRva == 0 || l_DebugSize == 0) {
          return false;
        }

        const u64 l_SectionTable = l_OptionalHeader + l_OptionalHeaderSize;
        u64 l_DebugOffset = 0;
        if (!rva_to_file_offset(l_File, l_SectionTable, l_SectionCount,
                                l_DebugRva, l_DebugOffset)) {
          return false;
        }

        // A binary can have several debug entries (e.g. POGO, VC
        // feature), the PDB reference is the CodeView one
        for (u64 i = 0; i < l_DebugSize / g_DebugEntrySize; ++i) {
          const u64 l_Entry = l_DebugOffset + i * g_DebugEntrySize;
          u32 l_Type = 0;
          u32 l_DataSize = 0;
          u32 l_DataPointer = 0;
          if (!read_value(l_File, l_Entry + 12, l_Type) ||
              !read_value(l_File, l_Entry + 16, l_DataSize) ||
              !read_value(l_File, l_Entry + 24, l_DataPointer)) {
            return false;
          }
          if (l_Type != g_DebugTypeCodeView ||
              l_DataSize <= g_RsdsHeaderSize ||
              l_DataSize > g_MaxCodeViewSize) {
            continue;
          }

          Util::List<u8> l_Data(l_DataSize);
          if (!read_at(l_File, l_DataPointer, l_Data.data(),
                       l_DataSize)) {
            return false;
          }

          u32 l_CodeViewSignature = 0;
          memcpy(&l_CodeViewSignature, l_Data.data(), 4);
          if (l_CodeViewSignature != g_RsdsSignature) {
            // Older formats (NB10) are not produced by current linkers
            continue;
          }

          u32 l_Age = 0;
          memcpy(&l_Age, l_Data.data() + 20, 4);

          // The path is zero terminated and UTF-8
          const char *l_PathStart =
              reinterpret_cast<const char *>(l_Data.data()) +
              g_RsdsHeaderSize;
          const size_t l_PathLength =
              strnlen(l_PathStart, l_DataSize - g_RsdsHeaderSize);

          p_OutReference.pdbPath = Util::String(l_PathStart, l_PathLength);
          p_OutReference.storeKey =
              make_store_key(l_Data.data() + 4, l_Age);
          return !p_OutReference.pdbPath.empty();
        }

        return false;
      }

      static bool is_binary(const fs::path &p_File)
      {
        Util::String l_Extension = from_path(p_File.extension());
        for (char &i_Char : l_Extension) {
          i_Char = static_cast<char>(
              tolower(static_cast<unsigned char>(i_Char)));
        }
        return l_Extension == ".dll" || l_Extension == ".exe";
      }

      // Copied next to the target first and then renamed, so a
      // cancelled copy never leaves a truncated PDB that looks complete
      // to the next package
      static bool copy_into_store(const fs::path &p_Source,
                                  const fs::path &p_Target,
                                  std::error_code &p_OutError)
      {
        fs::create_directories(p_Target.parent_path(), p_OutError);
        if (p_OutError) {
          return false;
        }

        fs::path l_TempTarget = p_Target;
        l_TempTarget += ".tmp";
        fs::copy_file(p_Source, l_TempTarget,
                      fs::copy_options::overwrite_existing, p_OutError);
        if (p_OutError) {
          return false;
        }

        fs::rename(l_TempTarget, p_Target, p_OutError);
        return !p_OutError;
      }

      bool store(const Util::List<Util::String> &p_BinaryDirectories,
                 const Util::String &p_Store, StoreResult &p_OutResult)
      {
        // Collected first so the progress bar knows the total
        Util::List<fs::path> l_Binaries;
        for (const Util::String &i_Directory : p_BinaryDirectories) {
          std::error_code l_Error;
          for (const fs::directory_entry &i_Entry :
               fs::directory_iterator(to_path(i_Directory), l_Error)) {
            if (i_Entry.is_regular_file(l_Error) &&
                is_binary(i_Entry.path())) {
              l_Binaries.push_back(i_Entry.path());
            }
          }
        }

        const fs::path l_Store = to_path(p_Store);
        const char *l_TaskId = "symbols";
        Output::task_begin(l_TaskId, "Storing symbols",
                           static_cast<int64_t>(l_Binaries.size()));

        for (size_t i = 0; i < l_Binaries.size(); ++i) {
          const fs::path &i_Binary = l_Binaries[i];
          const Util::String i_BinaryName = from_path(i_Binary.filename());
          Output::task_update(l_TaskId, static_cast<int64_t>(i),
                              i_BinaryName);

          PdbReference i_Reference;
          if (!read_pdb_reference(from_path(i_Binary), i_Reference)) {
            Output::debug(i_BinaryName + " has no debug info");
            continue;
          }

          // The linker writes the absolute path of the PDB it created,
          // which is exactly the PDB belonging to this binary
          const fs::path i_Pdb = to_path(i_Reference.pdbPath);
          std::error_code i_Error;
          if (!fs::is_regular_file(i_Pdb, i_Error)) {
            p_OutResult.missing.push_back(i_BinaryName + ": " +
                                          i_Reference.pdbPath);
            continue;
          }

          const fs::path i_Name = i_Pdb.filename();
          const fs::path i_Target =
              l_Store / i_Name / to_path(i_Reference.storeKey) / i_Name;
          if (fs::exists(i_Target, i_Error)) {
            ++p_OutResult.existing;
            continue;
          }

          if (!copy_into_store(i_Pdb, i_Target, i_Error)) {
            Output::task_end(l_TaskId, TaskStatus::FAILED);
            Output::error("Could not copy a PDB into the symbol store",
                          {from_path(i_Pdb), from_path(i_Target),
                           i_Error.message().c_str()},
                          "check that the symbol store is writable "
                          "(low config get symbol_store)");
            return false;
          }

          ++p_OutResult.added;
          // The error_code overload returns -1 on failure
          const uintmax_t i_Size = fs::file_size(i_Target, i_Error);
          if (!i_Error) {
            p_OutResult.addedBytes += i_Size;
          }
        }

        Output::task_update(l_TaskId,
                            static_cast<int64_t>(l_Binaries.size()));
        Output::task_end(l_TaskId, TaskStatus::OK);
        return true;
      }
    } // namespace Symbols
  } // namespace Cli
} // namespace Low
