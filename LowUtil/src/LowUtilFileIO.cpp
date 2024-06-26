#include "LowUtilFileIO.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"

#include <filesystem>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdio>

#ifndef WIN32
#include <unistd.h>
#endif

#ifdef WIN32
#define stat _stat
#endif

namespace Low {
  namespace Util {
    namespace FileIO {
      static const char *get_mode(uint8_t p_Mode)
      {
        switch (p_Mode) {
        case FileMode::READ_BYTES:
          return "rb";
        case FileMode::APPEND:
          return "a";
        case FileMode::WRITE:
          return "w";
        default:
          LOW_ASSERT(false, "Unknown file mode");
        }
      }

      File::File(FILE *p_FilePointer) : m_FilePointer(p_FilePointer)
      {
      }

      File::File() : m_FilePointer(nullptr)
      {
      }

      void File::open(const char *p_Path, uint8_t p_Mode)
      {
        LOW_ASSERT(!m_FilePointer, "Cannot open file that is already opened");
        m_FilePointer = FileIO::open(p_Path, p_Mode).m_FilePointer;
      }

      bool file_exists_sync(const char *p_Path)
      {
        struct stat l_Result;
        return stat(p_Path, &l_Result) == 0;
      }

      uint64_t modified_sync(const char *p_Path)
      {
        struct stat l_Result;
        LOW_ASSERT(stat(p_Path, &l_Result) == 0, "Could not find file");

        return l_Result.st_mtime;
      }

      uint32_t read_sync(File &p_File, char *p_Buffer)
      {
        size_t l_Size;
        size_t l_Result;

        l_Size = size_sync(p_File);
        l_Result = fread(p_Buffer, 1, l_Size, p_File.m_FilePointer);
        if (l_Result != l_Size) {
          return 1;
        }

        p_Buffer[l_Result] = '\0';

        return 0;
      }

      void write_sync(FILE *p_File, const char *p_Content, uint32_t p_Length)
      {
        std::fwrite(p_Content, 1, p_Length, p_File);
      }

      void write_sync(File &p_File, const char *p_Content, uint32_t p_Length)
      {
        write_sync(p_File.m_FilePointer, p_Content, p_Length);
      }

      void write_sync(File &p_File, String p_Content)
      {
        write_sync(p_File.m_FilePointer, p_Content.c_str(), p_Content.size());
      }

      void delete_sync(const char *p_Path)
      {
        std::remove(p_Path);
      }

      uint32_t size_sync(File &p_File)
      {
        uint32_t l_Size;

        rewind(p_File.m_FilePointer);
        fseek(p_File.m_FilePointer, 0, SEEK_END);
        l_Size = ftell(p_File.m_FilePointer);

        rewind(p_File.m_FilePointer);

        return l_Size;
      }

      File open(const char *p_FilePath, uint8_t p_Mode)
      {
        return File(fopen(p_FilePath, get_mode(p_Mode)));
      }

      void close(File &p_File)
      {
        fclose(p_File.m_FilePointer);
      }

      void list_directory(const char *p_Path, List<String> &p_ContentPaths)
      {
        for (const auto &i_Entry :
             std::filesystem::directory_iterator(p_Path)) {
          p_ContentPaths.push_back(String(i_Entry.path().string().c_str()));
        }
      }

      bool is_directory(const char *p_Path)
      {
        return std::filesystem::is_directory(p_Path);
      }

      void move_sync(const char *p_Source, const char *p_Target)
      {
        std::filesystem::rename(p_Source, p_Target);
      }
    } // namespace FileIO
  }   // namespace Util
} // namespace Low
