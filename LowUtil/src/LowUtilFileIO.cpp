#include "LowUtilFileIO.h"

#include "LowUtilAssert.h"

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
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
        default:
          LOW_ASSERT(false, "Unknown file mode");
        }
      }

      File::File(FILE *p_FilePointer) : m_FilePointer(p_FilePointer)
      {
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
    } // namespace FileIO
  }   // namespace Util
} // namespace Low
