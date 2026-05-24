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
          READ_BYTES,
          APPEND,
          APPEND_BYTES,
          WRITE,
          WRITE_BYTES
        };
      }

      struct File;

      LOW_EXPORT File open(const char *p_Path, uint8_t p_Mode);
      LOW_EXPORT void close(File &p_File);
      LOW_EXPORT uint32_t read_sync(File &p_File, char *p_Buffer);
      LOW_EXPORT uint32_t size_sync(File &p_File);
      LOW_EXPORT uint64_t modified_sync(const char *p_Path);
      LOW_EXPORT bool file_exists_sync(const char *p_Path);
      LOW_EXPORT void write_sync(File &p_File, const char *p_Content,
                                 uint32_t p_Length);
      LOW_EXPORT void write_sync(File &p_File, String p_Content);
      LOW_EXPORT bool write_bytes(File &p_File, const void *p_Data,
                                  uint32_t p_Length);

      LOW_EXPORT void move_sync(const char *p_Source, const char *p_Target);

      LOW_EXPORT void delete_sync(const char *p_Path);

      struct LOW_EXPORT File
      {
        friend void close(File &);
        friend File open(const char *, uint8_t);
        friend uint32_t read_sync(File &p_File, char *p_Buffer);
        friend uint32_t size_sync(File &p_File);
        friend void write_sync(File &p_File, const char *p_Content,
                               uint32_t p_Length);
        friend void write_sync(File &p_File, String p_Content);
        friend bool write_bytes(File &p_File, const void *p_Data,
                                uint32_t p_Length);

        File();
        void open(const char *p_Path, uint8_t p_Mode);
        bool is_open() const
        {
          return m_FilePointer != nullptr;
        }

      private:
        FILE *m_FilePointer;
        File(FILE *);
      };

      LOW_EXPORT void list_directory(const char *p_Path,
                                     List<String> &p_ContentPaths);

      LOW_EXPORT bool is_directory(const char *p_Path);

      template <typename T>
      bool write_value(File &p_File, const T &p_Value)
      {
        return write_bytes(p_File, &p_Value,
                           static_cast<uint32_t>(sizeof(T)));
      }

      template <typename T>
      bool write_array(File &p_File, const T *p_Data,
                       const uint32_t p_Count)
      {
        if (p_Count == 0u) {
          return true;
        }

        return write_bytes(
            p_File, p_Data,
            static_cast<uint32_t>(sizeof(T) * p_Count));
      }

      template <typename T>
      bool write_array(File &p_File, const List<T> &p_Data)
      {
        return write_array(p_File, p_Data.data(),
                           static_cast<uint32_t>(p_Data.size()));
      }

    } // namespace FileIO
  }   // namespace Util
} // namespace Low
