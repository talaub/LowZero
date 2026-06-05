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

      enum class FileEventType
      {
        Added,
        Modified,
        Removed
      };

      enum class AssetFileRole
      {
        Primary,
        Source,
        Manifest,
        Derived,
        Alias
      };

      enum class AssetHealthState
      {
        Unknown,
        Ok,
        MissingPrimaryFile,
        MissingSourceFile,
        MissingManifest,
        MissingDerivedFile,
        RequiresReimport,
        Stale,
        Broken,
        Deleting
      };

      typedef Low::Util::Handle (*Creator)(
          const Low::Util::Name, const Low::Util::String p_Path);
      typedef Low::Util::Handle (*Initializer)(
          const Low::Util::String);
      typedef Low::Util::String (*Importer)(const Low::Util::String);

      typedef void (*Loader)(Low::Util::Handle);
      typedef void (*Saver)(Low::Util::Handle);
      typedef bool (*SimpleCheck)(Low::Util::Handle);
      typedef void (*SimpleCall)(Low::Util::Handle);

      struct AssetFile
      {
        String path;
        AssetFileRole role = AssetFileRole::Primary;
        bool generated = false;
        bool deleteWithAsset = true;
      };

      struct AssetBundle
      {
        u16 typeId = 0u;
        Handle primaryHandle = Handle::DEAD;
        u64 uniqueId = 0ull;
        Util::Name name;
        List<Handle> handles;
        List<AssetFile> files;
      };

      struct AssetHealth
      {
        AssetHealthState state = AssetHealthState::Unknown;
        String message;
        List<String> missingFiles;
        List<String> staleFiles;

        bool is_ok() const
        {
          return state == AssetHealthState::Ok;
        }
      };

      typedef bool (*BundleByHandle)(Low::Util::Handle,
                                     AssetBundle &);
      typedef bool (*BundleByPath)(const Low::Util::String,
                                   AssetBundle &);
      typedef AssetHealth (*BundleDiagnoser)(const AssetBundle &);
      typedef void (*BundleAction)(const AssetBundle &,
                                   const AssetHealth &);
      typedef void (*BundleFileEventHandler)(const AssetBundle &,
                                             const AssetFile &,
                                             const FileEventType);

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

      // Authoring data describes loose source files, generated files,
      // import/reimport behavior, and health checks. It is meant for
      // editors, cookers, validation tools, and other non-final-build
      // workflows, while runtime asset registration stays in
      // TypeRegistrator.
      struct AuthoringTypeRegistrator
      {
        Name name;
        u16 typeId = 0u;
        Importer importer = nullptr;
        List<ImportDirectory> importDirectories;
        List<String> rawSuffixes;
        bool importOnStartup = false;
        BundleByHandle describeFromHandle = nullptr;
        BundleByPath describeFromPath = nullptr;
        BundleDiagnoser diagnose = nullptr;
        BundleAction repair = nullptr;
        BundleAction deleteAsset = nullptr;
        BundleFileEventHandler fileEvent = nullptr;
      };

      struct TypeRegistrator
      {
        Name name;
        u16 typeId;
        Initializer initializer = nullptr;
        Creator creator;
        List<Directory> initializeDirectories;
        bool autoInitialize;
        bool initializeOnStartup;
        bool creatable;
        List<String> assetSuffixes;
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
        add_asset_suffix(const String p_Suffix)
        {
          m_Registrator.assetSuffixes.push_back(p_Suffix);

          return *this;
        }

        const TypeRegistrator &build() const
        {
          return m_Registrator;
        }
      };

      struct AuthoringTypeRegistratorBuilder
      {
      private:
        AuthoringTypeRegistrator m_Registrator;

      public:
        AuthoringTypeRegistratorBuilder(
            const Name p_Name,
            const TypeIdentifier p_TypeIdentifier)
        {
          m_Registrator.name = p_Name;
          m_Registrator.typeId = Handle::type_id(p_TypeIdentifier);
        }

        AuthoringTypeRegistratorBuilder(const Name p_Name,
                                        const u16 p_TypeId)
        {
          m_Registrator.name = p_Name;
          m_Registrator.typeId = p_TypeId;
        }

        AuthoringTypeRegistratorBuilder &
        importer(const Importer p_Importer)
        {
          m_Registrator.importer = p_Importer;
          return *this;
        }

        AuthoringTypeRegistratorBuilder &
        import_on_startup(const bool p_Value)
        {
          m_Registrator.importOnStartup = p_Value;
          return *this;
        }

        AuthoringTypeRegistratorBuilder &
        add_import_directory(const String p_Path,
                             const bool p_Recursive = false,
                             const bool p_Autoscan = true)
        {
          m_Registrator.importDirectories.push_back(
              {p_Path, p_Autoscan, p_Recursive});
          return *this;
        }

        AuthoringTypeRegistratorBuilder &
        add_raw_suffix(const String p_Suffix)
        {
          m_Registrator.rawSuffixes.push_back(p_Suffix);
          return *this;
        }

        AuthoringTypeRegistratorBuilder &
        describe_from_handle(const BundleByHandle p_Callback)
        {
          m_Registrator.describeFromHandle = p_Callback;
          return *this;
        }

        AuthoringTypeRegistratorBuilder &
        describe_from_path(const BundleByPath p_Callback)
        {
          m_Registrator.describeFromPath = p_Callback;
          return *this;
        }

        AuthoringTypeRegistratorBuilder &
        diagnose(const BundleDiagnoser p_Callback)
        {
          m_Registrator.diagnose = p_Callback;
          return *this;
        }

        AuthoringTypeRegistratorBuilder &
        repair(const BundleAction p_Callback)
        {
          m_Registrator.repair = p_Callback;
          return *this;
        }

        AuthoringTypeRegistratorBuilder &
        delete_asset(const BundleAction p_Callback)
        {
          m_Registrator.deleteAsset = p_Callback;
          return *this;
        }

        AuthoringTypeRegistratorBuilder &
        file_event(const BundleFileEventHandler p_Callback)
        {
          m_Registrator.fileEvent = p_Callback;
          return *this;
        }

        const AuthoringTypeRegistrator &build() const
        {
          return m_Registrator;
        }
      };

      void LOW_EXPORT
      register_asset_type(const TypeRegistrator &p_Registrator);
      void LOW_EXPORT register_asset_authoring_type(
          const AuthoringTypeRegistrator &p_Registrator);

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

      bool LOW_EXPORT _find_authoring_type(
          const u16 p_TypeId,
          AuthoringTypeRegistrator &p_OutRegistrator);

      bool LOW_EXPORT _describe_bundle_from_handle(
          Util::Handle p_Handle, AssetBundle &p_OutBundle);
      bool LOW_EXPORT _describe_bundle_from_path(
          const Util::String p_Path, AssetBundle &p_OutBundle);
      AssetHealth LOW_EXPORT
      diagnose_bundle(const AssetBundle &p_Bundle);
      void LOW_EXPORT repair_bundle(const AssetBundle &p_Bundle,
                                    const AssetHealth &p_Health);
      void LOW_EXPORT delete_bundle(const AssetBundle &p_Bundle,
                                    const AssetHealth &p_Health);

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
