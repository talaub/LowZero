#include "LowCoreCflatScripting.h"

#include "LowCore.h"

#include "LowUtilLogger.h"
#include "LowUtilContainers.h"
#include "LowUtilFileSystem.h"
#include "LowUtilProfiler.h"

void setup_environment();

namespace Low {
  namespace Core {
    namespace Scripting {
      Util::Map<Util::String, uint64_t> g_SourceTimes;

      static void
      initial_load_directory(Util::FileSystem::WatchHandle p_WatchHandle)
      {
        Util::FileSystem::DirectoryWatcher &l_DirectoryWatcher =
            Util::FileSystem::get_directory_watcher(p_WatchHandle);

        for (Util::FileSystem::WatchHandle i_WatchHandle :
             l_DirectoryWatcher.files) {
          Util::FileSystem::FileWatcher &i_FileWatcher =
              Util::FileSystem::get_file_watcher(i_WatchHandle);
          g_SourceTimes[i_FileWatcher.path] = i_FileWatcher.modifiedTimestamp;

          get_environment()->load(i_FileWatcher.path.c_str());
        }

        for (Util::FileSystem::WatchHandle i_WatchHandle :
             l_DirectoryWatcher.subdirectories) {
          initial_load_directory(i_WatchHandle);
        }
      }

      void initialize()
      {
        setup_environment();

        initial_load_directory(Core::get_filesystem_watchers().scriptDirectory);
      }

      void cleanup()
      {
      }

      Cflat::Environment *get_environment()
      {
        return CflatGlobal::getEnvironment();
      }

      static void tick_directory(Util::FileSystem::WatchHandle p_WatchHandle)
      {
        Util::FileSystem::DirectoryWatcher &l_DirectoryWatcher =
            Util::FileSystem::get_directory_watcher(p_WatchHandle);

        for (Util::FileSystem::WatchHandle i_WatchHandle :
             l_DirectoryWatcher.files) {
          Util::FileSystem::FileWatcher &i_FileWatcher =
              Util::FileSystem::get_file_watcher(i_WatchHandle);

          auto i_Pos = g_SourceTimes.find(i_FileWatcher.path);
          if (i_Pos == g_SourceTimes.end() ||
              i_Pos->second != i_FileWatcher.modifiedTimestamp) {
            get_environment()->load(i_FileWatcher.path.c_str());
            g_SourceTimes[i_FileWatcher.path] = i_FileWatcher.modifiedTimestamp;
          }
        }

        for (Util::FileSystem::WatchHandle i_WatchHandle :
             l_DirectoryWatcher.subdirectories) {
          tick_directory(i_WatchHandle);
        }
      }

      void tick(float p_Delta, Util::EngineState p_State)
      {
        LOW_PROFILE_CPU("Core", "Scripting hotreload");
        tick_directory(Core::get_filesystem_watchers().scriptDirectory);
      }
    } // namespace Scripting
  }   // namespace Core
} // namespace Low

#if defined CFLAT_ENABLED
namespace CflatGlobal {
  Cflat::Environment g_Environment;
  std::mutex g_Mutex;

  Cflat::Environment *getEnvironment()
  {
    return &g_Environment;
  }
  void lockEnvironment()
  {
    g_Mutex.lock();
  }
  void unlockEnvironment()
  {
    g_Mutex.unlock();
  }
  void onError(const char *pErrorMessage)
  {
    LOW_LOG_ERROR << pErrorMessage << LOW_LOG_END;
  }
} // namespace CflatGlobal

#include "LowMath.h"
static void register_math()
{
  using namespace Low::Core;
  using namespace Low::Math;

  Cflat::Namespace *l_Namespace =
      Scripting::get_environment()->requestNamespace("Low::Math");

  {
    CflatRegisterStruct(l_Namespace, Vector3);
    CflatStructAddMember(l_Namespace, Vector3, float, x);
    CflatStructAddMember(l_Namespace, Vector3, float, y);
    CflatStructAddMember(l_Namespace, Vector3, float, z);
    CflatStructAddConstructorParams3(l_Namespace, Vector3, float, float, float);
    CflatStructAddCopyConstructor(l_Namespace, Vector3);

    CflatRegisterFunctionReturnParams2(l_Namespace, Vector3, operator+,
                                       const Vector3 &, const Vector3 &);
    CflatRegisterFunctionReturnParams2(l_Namespace, Vector3, operator-,
                                       const Vector3 &, const Vector3 &);
    CflatRegisterFunctionReturnParams2(l_Namespace, Vector3, operator*,
                                       const Vector3 &, float);
  }
  {
    CflatRegisterStruct(l_Namespace, Vector2);
    CflatStructAddMember(l_Namespace, Vector2, float, x);
    CflatStructAddMember(l_Namespace, Vector2, float, y);
    CflatStructAddConstructorParams2(l_Namespace, Vector2, float, float);
    CflatStructAddCopyConstructor(l_Namespace, Vector2);

    CflatRegisterFunctionReturnParams2(l_Namespace, Vector2, operator+,
                                       const Vector2 &, const Vector2 &);
    CflatRegisterFunctionReturnParams2(l_Namespace, Vector2, operator-,
                                       const Vector2 &, const Vector2 &);
    CflatRegisterFunctionReturnParams2(l_Namespace, Vector2, operator*,
                                       const Vector2 &, float);
  }
  {
    CflatRegisterStruct(l_Namespace, Quaternion);
    CflatStructAddMember(l_Namespace, Quaternion, float, x);
    CflatStructAddMember(l_Namespace, Quaternion, float, y);
    CflatStructAddMember(l_Namespace, Quaternion, float, z);
    CflatStructAddMember(l_Namespace, Quaternion, float, w);
    CflatStructAddConstructorParams4(l_Namespace, Quaternion, float, float,
                                     float, float);
    CflatStructAddCopyConstructor(l_Namespace, Quaternion);

    CflatRegisterFunctionReturnParams2(l_Namespace, Quaternion, operator*,
                                       const Quaternion &, const Quaternion &);
    CflatRegisterFunctionReturnParams2(l_Namespace, Vector3, operator*,
                                       const Quaternion &, const Vector3 &);
  }
}

#include "LowUtilName.h"
static void register_lowutil_name()
{
  using namespace Low::Core;
  using namespace Low::Util;

  Cflat::Namespace *l_Namespace =
      Scripting::get_environment()->requestNamespace("Low::Util");

  CflatRegisterStruct(l_Namespace, Name);

  CflatStructAddConstructor(l_Namespace, Name);
  CflatStructAddCopyConstructor(l_Namespace, Name);
  CflatStructAddConstructorParams1(l_Namespace, Name, const char *);
  CflatStructAddConstructorParams1(l_Namespace, Name, const Name &);
  CflatStructAddConstructorParams1(l_Namespace, Name, uint32_t);
  CflatStructAddMethodReturn(l_Namespace, Name, char *, c_str);
  CflatStructAddMember(l_Namespace, Name, uint32_t, m_Index);

  CflatStructAddMethodReturnParams1(l_Namespace, Name, bool, operator==,
                                    const Name &);
  CflatStructAddMethodReturnParams1(l_Namespace, Name, bool, operator!=,
                                    const Name &);
  CflatStructAddMethodReturnParams1(l_Namespace, Name, bool, operator<,
                                    const Name &);
  CflatStructAddMethodReturnParams1(l_Namespace, Name, bool, operator>,
                                    const Name &);
  CflatStructAddMethodReturnParams1(l_Namespace, Name, Name &, operator=,
                                    const Name);
}

#include "LowUtilHandle.h"
static void register_lowutil_handle()
{
  using namespace Low::Core;
  using namespace Low::Util;

  Cflat::Namespace *l_Namespace =
      Scripting::get_environment()->requestNamespace("Low::Util");

  CflatRegisterStruct(l_Namespace, Handle);

  CflatStructAddConstructor(l_Namespace, Handle);
  CflatStructAddCopyConstructor(l_Namespace, Handle);
  CflatStructAddConstructorParams1(l_Namespace, Handle, uint64_t);
  CflatStructAddMethodReturn(l_Namespace, Handle, uint64_t, get_id);
  CflatStructAddMethodReturn(l_Namespace, Handle, uint32_t, get_index);
  CflatStructAddMethodReturn(l_Namespace, Handle, uint16_t, get_generation);
  CflatStructAddMethodReturn(l_Namespace, Handle, uint16_t, get_type);
  CflatStructAddMethodReturnParams1(l_Namespace, Handle, bool, operator==,
                                    const Handle &);
  CflatStructAddMethodReturnParams1(l_Namespace, Handle, bool, operator!=,
                                    const Handle &);
  CflatStructAddMethodReturnParams1(l_Namespace, Handle, bool, operator<,
                                    const Handle &);
}

// REGISTER_CFLAT_BEGIN
#include "LowCoreEntity.h"
static void register_lowcore_entity()
{
  using namespace Low::Core;

  Cflat::Namespace *l_Namespace =
      Scripting::get_environment()->requestNamespace("Low::Core");

  CflatRegisterStruct(l_Namespace, Entity);
  CflatStructAddBaseType(Scripting::get_environment(), Low::Core::Entity,
                         Low::Util::Handle);
  CflatStructAddMethodReturn(l_Namespace, Entity, bool, is_alive);
  CflatStructAddStaticMethodReturn(l_Namespace, Entity, uint32_t, get_capacity);
  CflatStructAddStaticMethodReturnParams1(
      l_Namespace, Entity, Low::Core::Entity, find_by_index, uint32_t);
  CflatStructAddStaticMethodReturnParams1(
      l_Namespace, Entity, Low::Core::Entity, find_by_name, Low::Util::Name);

  /*
  CflatStructAddMethodReturn(
      l_Namespace, Entity, Util::Map<uint16_t, Util::Handle> &, get_components);

  CflatStructAddMethodReturn(l_Namespace, Entity, Region, get_region);
  CflatStructAddMethodVoidParams1(l_Namespace, Entity, set_region, Region);
  */

  /*
  CflatStructAddMethodReturn(l_Namespace, Entity, Low::Util::UniqueId,
                             get_unique_id);
                             */

  CflatStructAddMethodReturn(l_Namespace, Entity, Low::Util::Name, get_name);
  CflatStructAddMethodVoidParams1(l_Namespace, Entity, void, set_name,
                                  Low::Util::Name);
}

#include "LowCoreTransform.h"
static void register_lowcore_transform()
{
  using namespace Low;
  using namespace Low::Core;
  using namespace Low::Core::Component;

  Cflat::Namespace *l_Namespace =
      Scripting::get_environment()->requestNamespace("Low::Core::Component");

  CflatRegisterStruct(l_Namespace, Transform);
  CflatStructAddBaseType(Scripting::get_environment(),
                         Low::Core::Component::Transform, Low::Util::Handle);
  CflatStructAddMethodReturn(l_Namespace, Transform, bool, is_alive);
  CflatStructAddStaticMethodReturn(l_Namespace, Transform, uint32_t,
                                   get_capacity);
  CflatStructAddStaticMethodReturnParams1(l_Namespace, Transform,
                                          Low::Core::Component::Transform,
                                          find_by_index, uint32_t);

  CflatStructAddMethodReturn(l_Namespace, Transform, Math::Vector3 &, position);
  CflatStructAddMethodVoidParams1(l_Namespace, Transform, void, position,
                                  Math::Vector3 &);

  CflatStructAddMethodReturn(l_Namespace, Transform, Math::Quaternion &,
                             rotation);
  CflatStructAddMethodVoidParams1(l_Namespace, Transform, void, rotation,
                                  Math::Quaternion &);

  CflatStructAddMethodReturn(l_Namespace, Transform, Math::Vector3 &, scale);
  CflatStructAddMethodVoidParams1(l_Namespace, Transform, void, scale,
                                  Math::Vector3 &);

  CflatStructAddMethodReturn(l_Namespace, Transform, uint64_t, get_parent);
  CflatStructAddMethodVoidParams1(l_Namespace, Transform, void, set_parent,
                                  uint64_t);

  CflatStructAddMethodReturn(l_Namespace, Transform, uint64_t, get_parent_uid);

  /*
  CflatStructAddMethodReturn(l_Namespace, Transform, Util::List<uint64_t> &,
                             get_children);
                             */

  CflatStructAddMethodReturn(l_Namespace, Transform, Math::Vector3 &,
                             get_world_position);

  CflatStructAddMethodReturn(l_Namespace, Transform, Math::Quaternion &,
                             get_world_rotation);

  CflatStructAddMethodReturn(l_Namespace, Transform, Math::Vector3 &,
                             get_world_scale);

  /*
  CflatStructAddMethodReturn(l_Namespace, Transform, Math::Matrix4x4 &,
                             get_world_matrix);
                             */

  CflatStructAddMethodReturn(l_Namespace, Transform, bool, is_world_updated);
  CflatStructAddMethodVoidParams1(l_Namespace, Transform, void,
                                  set_world_updated, bool);

  CflatStructAddMethodReturn(l_Namespace, Transform, Low::Core::Entity,
                             get_entity);
  CflatStructAddMethodVoidParams1(l_Namespace, Transform, void, set_entity,
                                  Low::Core::Entity);

  /*
  CflatStructAddMethodReturn(l_Namespace, Transform, Low::Util::UniqueId,
                             get_unique_id);
                             */

  CflatStructAddMethodReturn(l_Namespace, Transform, bool, is_dirty);
  CflatStructAddMethodVoidParams1(l_Namespace, Transform, void, set_dirty,
                                  bool);

  CflatStructAddMethodReturn(l_Namespace, Transform, bool, is_world_dirty);
  CflatStructAddMethodVoidParams1(l_Namespace, Transform, void, set_world_dirty,
                                  bool);
}

static void register_types()
{
  register_lowcore_entity();
  register_lowcore_transform();
}
// REGISTER_CFLAT_END

static void setup_environment()
{
  register_math();
  register_lowutil_name();
  register_lowutil_handle();
  register_types();
}
#endif
