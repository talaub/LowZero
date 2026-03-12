#pragma once

#include "LowUtilApi.h"

#include "LowUtilHandle.h"

namespace Low {
  namespace Util {

    namespace AssetManager {

      typedef Low::Util::Handle (*Initializer)(
          const Low::Util::String);
      typedef Low::Util::String (*Importer)(const Low::Util::String);
      typedef void (*Deleter)(const Low::Util::String);

      struct ImportDirectory
      {
        String path;
        bool autoscan;
        bool recursive;
      };

      struct Directory
      {
        String path;
        bool recursive;
      };

      struct TypeRegistrator
      {
        Name name;
        Initializer initializer;
        Importer importer;
        Deleter rawDeleter;
        Deleter deleter;
        List<ImportDirectory> importDirectories;
        List<Directory> initializeDirectories;
        bool autoInitialize;
        bool initializeOnStartup;
        List<String> assetSuffixes;
        List<String> rawSuffixes;
      };

      struct TypeRegistratorBuilder
      {
      private:
        TypeRegistrator m_Registrator;

      public:
        TypeRegistratorBuilder(const Name p_Name)
        {
          m_Registrator.name = p_Name;
        }

        TypeRegistratorBuilder &
        initializer(const Initializer p_Initializer)
        {
          m_Registrator.initializer = p_Initializer;
          return *this;
        }

        TypeRegistratorBuilder &importer(const Importer p_Importer)
        {
          m_Registrator.importer = p_Importer;
          return *this;
        }

        TypeRegistratorBuilder &raw_deleter(const Deleter p_Deleter)
        {
          m_Registrator.rawDeleter = p_Deleter;
          return *this;
        }

        TypeRegistratorBuilder &deleter(const Deleter p_Deleter)
        {
          m_Registrator.deleter = p_Deleter;
          return *this;
        }

        TypeRegistratorBuilder &auto_initialize(const bool p_Value)
        {
          m_Registrator.autoInitialize = p_Value;
          return *this;
        }

        TypeRegistratorBuilder &
        initialize_on_startup(const bool p_Value)
        {
          m_Registrator.initializeOnStartup = p_Value;
          return *this;
        }

        TypeRegistratorBuilder &
        add_initialize_directory(const String p_Path,
                                 const bool p_Recursive = false)
        {
          m_Registrator.initializeDirectories.push_back(
              {p_Path, p_Recursive});

          return *this;
        }

        TypeRegistratorBuilder &
        add_import_direcotry(const String p_Path,
                             const bool p_Recursive = false,
                             const bool p_Autoscan = true)
        {
          m_Registrator.importDirectories.push_back(
              {p_Path, p_Autoscan, p_Recursive});

          return *this;
        }

        TypeRegistratorBuilder &
        add_asset_suffix(const String p_Suffix)
        {
          m_Registrator.assetSuffixes.push_back(p_Suffix);

          return *this;
        }

        TypeRegistratorBuilder &add_raw_suffix(const String p_Suffix)
        {
          m_Registrator.rawSuffixes.push_back(p_Suffix);

          return *this;
        }

        const TypeRegistrator &build() const
        {
          return m_Registrator;
        }
      };

      void LOW_EXPORT
      register_asset_type(const TypeRegistrator &p_Registrator);

      void initialize();
      void tick(const float p_Delta);
      void cleanup();
    }; // namespace AssetManager

  } // namespace Util
} // namespace Low
