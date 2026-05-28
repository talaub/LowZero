#pragma once

#include "LowUtilApi.h"

#include "LowUtilHandle.h"

namespace Low {
  namespace Util {

    namespace AssetManager {

      enum class LoadPriority
      {
        Low,
        Medium,
        High
      };

      typedef Low::Util::Handle (*Creator)(
          const Low::Util::Name, const Low::Util::String p_Path);
      typedef Low::Util::Handle (*Initializer)(
          const Low::Util::String);
      typedef Low::Util::String (*Importer)(const Low::Util::String);
      typedef void (*Deleter)(const Low::Util::String);

      typedef void (*Loader)(Low::Util::Handle);
      typedef void (*Saver)(Low::Util::Handle);
      typedef bool (*SimpleCheck)(Low::Util::Handle);
      typedef void (*SimpleCall)(Low::Util::Handle);

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

      struct StoreSettings
      {
        Util::Name pathPropertyName;
      };

      struct TypeRegistrator
      {
        Name name;
        u16 typeId;
        Initializer initializer = nullptr;
        Creator creator;
        Importer importer;
        Deleter rawDeleter;
        Deleter deleter;
        List<ImportDirectory> importDirectories;
        List<Directory> initializeDirectories;
        bool autoInitialize;
        bool initializeOnStartup;
        bool importOnStartup;
        bool creatable;
        List<String> assetSuffixes;
        List<String> rawSuffixes;
        Loader loader;
        Saver saver;
        bool supportsLoading;
        bool supportsSaving;
        SimpleCheck isLoadable;
        StoreSettings storeSettings;
        SimpleCall postRegister;
      };

      struct TypeRegistratorBuilder
      {
      private:
        TypeRegistrator m_Registrator;

      public:
        TypeRegistratorBuilder(const Name p_Name,
                               const TypeIdentifier p_TypeIdentifier)
        {
          m_Registrator.name = p_Name;
          m_Registrator.supportsLoading = true;
          m_Registrator.supportsSaving = true;
          m_Registrator.typeId = Handle::type_id(p_TypeIdentifier);
          m_Registrator.loader = nullptr;
          m_Registrator.creatable = false;
          m_Registrator.autoInitialize = false;
          m_Registrator.initializeOnStartup = false;
          m_Registrator.importOnStartup = false;
          m_Registrator.postRegister = nullptr;
        }

        TypeRegistratorBuilder &
        initializer(const Initializer p_Initializer)
        {
          m_Registrator.initializer = p_Initializer;
          return *this;
        }

        TypeRegistratorBuilder &creator(const Creator p_Creator)
        {
          m_Registrator.creator = p_Creator;
          return *this;
        }

        TypeRegistratorBuilder &post_register(const SimpleCall p_Call)
        {
          m_Registrator.postRegister = p_Call;
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

        TypeRegistratorBuilder &saver(const Saver p_Saver)
        {
          m_Registrator.saver = p_Saver;
          return *this;
        }

        TypeRegistratorBuilder &loader(const Loader p_Loader)
        {
          m_Registrator.loader = p_Loader;
          return *this;
        }

        TypeRegistratorBuilder &is_loadable(const SimpleCheck p_Check)
        {
          m_Registrator.isLoadable = p_Check;
          return *this;
        }

        TypeRegistratorBuilder &creatable(const bool p_Value = true)
        {
          m_Registrator.creatable = p_Value;
          return *this;
        }

        TypeRegistratorBuilder &auto_initialize(const bool p_Value)
        {
          m_Registrator.autoInitialize = p_Value;
          return *this;
        }

        TypeRegistratorBuilder &supports_loading(const bool p_Value)
        {
          m_Registrator.supportsLoading = p_Value;
          return *this;
        }

        TypeRegistratorBuilder &supports_saving(const bool p_Value)
        {
          m_Registrator.supportsSaving = p_Value;
          return *this;
        }

        TypeRegistratorBuilder &no_saving()
        {
          return supports_saving(false);
        }

        TypeRegistratorBuilder &
        load_path_property_name(const Name p_Propertyname)
        {
          m_Registrator.storeSettings.pathPropertyName =
              p_Propertyname;
          return *this;
        }

        TypeRegistratorBuilder &import_on_startup(const bool p_Value)
        {
          m_Registrator.importOnStartup = p_Value;
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
        add_import_directory(const String p_Path,
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

      void LOW_EXPORT _load(Util::Handle p_Handle,
                            const LoadPriority p_Priority);

      void LOW_EXPORT _save(Util::Handle p_Handle);

      Util::Handle LOW_EXPORT _create(const u16 p_TypeId,
                                      const Name p_Name,
                                      const String p_Path);

      template <typename T>
      void load(T p_Handle,
                const LoadPriority p_Priority = LoadPriority::Medium)
      {
        _load(p_Handle.get_id(), p_Priority);
      }

      template <typename T> void save(T p_Handle)
      {
        _save(p_Handle.get_id());
      }

      template <typename T>
      T create(const Low::Util::Name p_Name,
               const Low::Util::String p_Path)
      {
        return _create(T::type_id(), p_Name, p_Path).get_id();
      }

      Handle LOW_EXPORT _find_by_path(const String p_Path);
      template <typename T> T find_by_path(const String p_Path)
      {
        return _find_by_path(p_Path).get_id();
      }

      void LOW_EXPORT _register(Util::Handle p_Handle,
                                Util::String p_Path,
                                Util::Name p_Name,
                                const u64 p_UniqueId);
      void LOW_EXPORT _register(Util::Handle p_Handle,
                                Util::String p_Path);
      void LOW_EXPORT _register_alias(Util::Handle p_Handle,
                                      Util::String p_Path);

      template <typename T>
      void register_asset(T p_Handle, const Util::String p_Path)
      {
        _register(p_Handle.get_id(), p_Path);
      }

      template <typename T>
      void register_alias(T p_Handle, const Util::String p_Path)
      {
        _register_alias(p_Handle.get_id(), p_Path);
      }

    } // namespace AssetManager

  } // namespace Util
} // namespace Low
