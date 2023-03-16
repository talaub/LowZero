#pragma once

#include "LowUtilApi.h"
#include "LowUtilContainers.h"

#include <stdint.h>
#include <stdio.h>

namespace Low {
  namespace Util {
    namespace FileIO {
      namespace FileMode {
        enum Enum
        {
          READ_BYTES
        };
      }

      struct File;

      LOW_EXPORT File open(const char *p_Path, uint8_t p_Mode);
      LOW_EXPORT void close(File &p_File);
      LOW_EXPORT uint32_t read_sync(File &p_File, char *p_Buffer);
      LOW_EXPORT uint32_t size_sync(File &p_File);
      LOW_EXPORT uint64_t modified_sync(const char *p_Path);

      struct LOW_EXPORT File
      {
        friend void close(File &);
        friend File open(const char *, uint8_t);
        friend uint32_t read_sync(File &p_File, char *p_Buffer);
        friend uint32_t size_sync(File &p_File);

      private:
        FILE *m_FilePointer;
        File(FILE *);
      };

      LOW_EXPORT void list_directory(const char *p_Path,
                                     List<String> &p_ContentPaths);

    } // namespace FileIO
  }   // namespace Util
} // namespace Low
