#include "LowCoreCflatScripting.h"

#include "LowCore.h"

#include "LowUtilLogger.h"
#include "LowUtilContainers.h"
#include "LowUtilFileSystem.h"
#include "LowUtilProfiler.h"

#include "CflatHelper.h"

void setup_environment();

namespace Low {
  namespace Core {
    namespace Scripting {
      Util::Map<Util::String, uint64_t> g_SourceTimes;

      Util::Map<Util::String, Cflat::Struct *> g_CflatStructs;

      static void initial_load_directory(
          Util::FileSystem::WatchHandle p_WatchHandle)
      {
        Util::FileSystem::DirectoryWatcher &l_DirectoryWatcher =
            Util::FileSystem::get_directory_watcher(p_WatchHandle);

        for (Util::FileSystem::WatchHandle i_WatchHandle :
             l_DirectoryWatcher.files) {
          Util::FileSystem::FileWatcher &i_FileWatcher =
              Util::FileSystem::get_file_watcher(i_WatchHandle);
          g_SourceTimes[i_FileWatcher.path] =
              i_FileWatcher.modifiedTimestamp;

          if (!get_environment()->load(i_FileWatcher.path.c_str())) {
            LOW_LOG_ERROR << "Failed Cflat compilation of file "
                          << i_FileWatcher.path << LOW_LOG_END;

            LOW_ASSERT(false, get_environment()->getErrorMessage());
          }
        }

        for (Util::FileSystem::WatchHandle i_WatchHandle :
             l_DirectoryWatcher.subdirectories) {
          initial_load_directory(i_WatchHandle);
        }
      }

      void initialize()
      {
        setup_environment();

        initial_load_directory(
            Core::get_filesystem_watchers().scriptDirectory);
      }

      void cleanup()
      {
      }

      Cflat::Environment *get_environment()
      {
        return CflatGlobal::getEnvironment();
      }

      static void
      tick_directory(Util::FileSystem::WatchHandle p_WatchHandle)
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

            if (!get_environment()->load(
                    i_FileWatcher.path.c_str())) {
              LOW_LOG_ERROR << "Failed Cflat compilation of file "
                            << i_FileWatcher.path << LOW_LOG_END;

              LOW_ASSERT(false, get_environment()->getErrorMessage());
            }
            g_SourceTimes[i_FileWatcher.path] =
                i_FileWatcher.modifiedTimestamp;
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
        tick_directory(
            Core::get_filesystem_watchers().scriptDirectory);
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
    CflatRegisterStruct(l_Namespace, Vector4);
    CflatStructAddMember(l_Namespace, Vector4, float, x);
    CflatStructAddMember(l_Namespace, Vector4, float, y);
    CflatStructAddMember(l_Namespace, Vector4, float, z);
    CflatStructAddMember(l_Namespace, Vector4, float, w);
    CflatStructAddConstructorParams4(l_Namespace, Vector4, float,
                                     float, float, float);
    CflatStructAddCopyConstructor(l_Namespace, Vector4);

    CflatRegisterFunctionReturnParams2(
        l_Namespace, Vector4, operator*, const Vector4 &,
        const Vector4 &);
  }
  {
    CflatRegisterStruct(l_Namespace, Vector3);
    CflatStructAddMember(l_Namespace, Vector3, float, x);
    CflatStructAddMember(l_Namespace, Vector3, float, y);
    CflatStructAddMember(l_Namespace, Vector3, float, z);
    CflatStructAddConstructorParams3(l_Namespace, Vector3, float,
                                     float, float);
    CflatStructAddCopyConstructor(l_Namespace, Vector3);

    CflatRegisterFunctionReturnParams2(
        l_Namespace, Vector3, operator+, const Vector3 &,
        const Vector3 &);
    CflatRegisterFunctionReturnParams2(
        l_Namespace, Vector3, operator-, const Vector3 &,
        const Vector3 &);
    CflatRegisterFunctionReturnParams2(
        l_Namespace, Vector3, operator*, const Vector3 &, float);
  }
  {
    CflatRegisterStruct(l_Namespace, Vector2);
    CflatStructAddMember(l_Namespace, Vector2, float, x);
    CflatStructAddMember(l_Namespace, Vector2, float, y);
    CflatStructAddConstructorParams2(l_Namespace, Vector2, float,
                                     float);
    CflatStructAddCopyConstructor(l_Namespace, Vector2);

    CflatRegisterFunctionReturnParams2(
        l_Namespace, Vector2, operator+, const Vector2 &,
        const Vector2 &);
    CflatRegisterFunctionReturnParams2(
        l_Namespace, Vector2, operator-, const Vector2 &,
        const Vector2 &);
    CflatRegisterFunctionReturnParams2(
        l_Namespace, Vector2, operator*, const Vector2 &, float);
  }
  {
    CflatRegisterStruct(l_Namespace, UVector2);
    CflatStructAddMember(l_Namespace, UVector2, uint32_t, x);
    CflatStructAddMember(l_Namespace, UVector2, uint32_t, y);
    CflatStructAddConstructorParams2(l_Namespace, UVector2, uint32_t,
                                     uint32_t);
    CflatStructAddCopyConstructor(l_Namespace, UVector2);

    CflatRegisterFunctionReturnParams2(
        l_Namespace, UVector2, operator+, const UVector2 &,
        const UVector2 &);
    CflatRegisterFunctionReturnParams2(
        l_Namespace, UVector2, operator-, const UVector2 &,
        const UVector2 &);
    CflatRegisterFunctionReturnParams2(
        l_Namespace, UVector2, operator*, const UVector2 &, uint32_t);
  }
  {
    CflatRegisterStruct(l_Namespace, Quaternion);
    CflatStructAddMember(l_Namespace, Quaternion, float, x);
    CflatStructAddMember(l_Namespace, Quaternion, float, y);
    CflatStructAddMember(l_Namespace, Quaternion, float, z);
    CflatStructAddMember(l_Namespace, Quaternion, float, w);
    CflatStructAddConstructorParams4(l_Namespace, Quaternion, float,
                                     float, float, float);
    CflatStructAddCopyConstructor(l_Namespace, Quaternion);

    CflatRegisterFunctionReturnParams2(
        l_Namespace, Quaternion, operator*, const Quaternion &,
        const Quaternion &);
    CflatRegisterFunctionReturnParams2(
        l_Namespace, Vector3, operator*, const Quaternion &,
        const Vector3 &);
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

  CflatStructAddMethodReturnParams1(l_Namespace, Name,
                                    bool, operator==, const Name &);
  CflatStructAddMethodReturnParams1(l_Namespace, Name,
                                    bool, operator!=, const Name &);
  CflatStructAddMethodReturnParams1(l_Namespace, Name,
                                    bool, operator<, const Name &);
  CflatStructAddMethodReturnParams1(l_Namespace, Name,
                                    bool, operator>, const Name &);
  CflatStructAddMethodReturnParams1(l_Namespace, Name,
                                    Name &, operator=, const Name);

  Scripting::get_environment()->defineMacro("N(x)",
                                            "Low::Util::Name(#x)");
}

#include "LowUtilContainers.h"
static void register_lowutil_containers()
{
  using namespace Low;
  using namespace Low::Core;
  using namespace Low::Util;

  Cflat::Namespace *l_Namespace =
      Scripting::get_environment()->requestNamespace("Low::Util");

  CflatRegisterSTLVectorCustom(l_Namespace, List, bool);
  CflatRegisterSTLVectorCustom(l_Namespace, List, int);
  CflatRegisterSTLVectorCustom(l_Namespace, List, float);

  CflatRegisterSTLVectorCustom(l_Namespace, List, uint64_t);
  CflatRegisterSTLVectorCustom(l_Namespace, List, uint32_t);

  // CflatRegisterSTLVectorCustom(l_Namespace, List, String);
  CflatRegisterSTLVectorCustom(l_Namespace, List, Name);

  CflatRegisterSTLVectorCustom(l_Namespace, List, Math::Vector3);
  CflatRegisterSTLVectorCustom(l_Namespace, List, Math::Vector2);
  CflatRegisterSTLVectorCustom(l_Namespace, List, Math::Vector4);
  CflatRegisterSTLVectorCustom(l_Namespace, List, Math::UVector2);

  CflatRegisterSTLVectorCustom(l_Namespace, List, Handle);
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
  CflatStructAddMethodReturn(l_Namespace, Handle, uint32_t,
                             get_index);
  CflatStructAddMethodReturn(l_Namespace, Handle, uint16_t,
                             get_generation);
  CflatStructAddMethodReturn(l_Namespace, Handle, uint16_t, get_type);
  CflatStructAddMethodReturnParams1(l_Namespace, Handle,
                                    bool, operator==, const Handle &);
  CflatStructAddMethodReturnParams1(l_Namespace, Handle,
                                    bool, operator!=, const Handle &);
  CflatStructAddMethodReturnParams1(l_Namespace, Handle,
                                    bool, operator<, const Handle &);
}

// REGISTER_CFLAT_BEGIN
#include "LowCoreEntity.h"
#include "LowCoreTransform.h"
#include "LowCoreUiElement.h"
#include "LowCoreUiDisplay.h"

static void register_lowcore_entity()
{
  using namespace Low;
  using namespace Low::Core;

  Cflat::Namespace *l_Namespace =
      Scripting::get_environment()->requestNamespace("Low::Core");

  Cflat::Struct *type =
      Low::Core::Scripting::g_CflatStructs["Low::Core::Entity"];

  {
    Cflat::Namespace *l_UtilNamespace =
        Scripting::get_environment()->requestNamespace("Low::Util");

    CflatRegisterSTLVectorCustom(l_UtilNamespace, Low::Util::List,
                                 Low::Core::Entity);
  }

  CflatStructAddMethodReturn(l_Namespace, Entity, bool, is_alive);
  CflatStructAddStaticMethodReturn(l_Namespace, Entity, uint32_t,
                                   get_capacity);
  CflatStructAddStaticMethodReturnParams1(l_Namespace, Entity,
                                          Low::Core::Entity,
                                          find_by_index, uint32_t);
  CflatStructAddStaticMethodReturnParams1(
      l_Namespace, Entity, Low::Core::Entity, find_by_name,
      Low::Util::Name);

  CflatStructAddMethodReturn(l_Namespace, Entity, Low::Util::Name,
                             get_name);
  CflatStructAddMethodVoidParams1(l_Namespace, Entity, void, set_name,
                                  Low::Util::Name);

  CflatStructAddMethodReturnParams1(l_Namespace, Entity, uint64_t,
                                    get_component, uint16_t);
  CflatStructAddMethodVoidParams1(l_Namespace, Entity, void,
                                  add_component, Util::Handle);
  CflatStructAddMethodVoidParams1(l_Namespace, Entity, void,
                                  remove_component, uint16_t);
  CflatStructAddMethodReturnParams1(l_Namespace, Entity, bool,
                                    has_component, uint16_t);
  CflatStructAddMethodReturn(l_Namespace, Entity,
                             Component::Transform, get_transform);
}

static void register_lowcore_transform()
{
  using namespace Low;
  using namespace Low::Core;
  using namespace Low::Core::Component;

  Cflat::Namespace *l_Namespace =
      Scripting::get_environment()->requestNamespace(
          "Low::Core::Component");

  Cflat::Struct *type = Low::Core::Scripting::g_CflatStructs
      ["Low::Core::Component::Transform"];

  {
    Cflat::Namespace *l_UtilNamespace =
        Scripting::get_environment()->requestNamespace("Low::Util");

    CflatRegisterSTLVectorCustom(l_UtilNamespace, Low::Util::List,
                                 Low::Core::Component::Transform);
  }

  CflatStructAddMethodReturn(l_Namespace, Transform, bool, is_alive);
  CflatStructAddStaticMethodReturn(l_Namespace, Transform, uint32_t,
                                   get_capacity);
  CflatStructAddStaticMethodReturnParams1(
      l_Namespace, Transform, Low::Core::Component::Transform,
      find_by_index, uint32_t);

  CflatStructAddMethodReturn(l_Namespace, Transform, Math::Vector3 &,
                             position);
  CflatStructAddMethodVoidParams1(l_Namespace, Transform, void,
                                  position, Math::Vector3 &);

  CflatStructAddMethodReturn(l_Namespace, Transform,
                             Math::Quaternion &, rotation);
  CflatStructAddMethodVoidParams1(l_Namespace, Transform, void,
                                  rotation, Math::Quaternion &);

  CflatStructAddMethodReturn(l_Namespace, Transform, Math::Vector3 &,
                             scale);
  CflatStructAddMethodVoidParams1(l_Namespace, Transform, void, scale,
                                  Math::Vector3 &);

  CflatStructAddMethodReturn(l_Namespace, Transform, uint64_t,
                             get_parent);
  CflatStructAddMethodVoidParams1(l_Namespace, Transform, void,
                                  set_parent, uint64_t);

  CflatStructAddMethodReturn(l_Namespace, Transform,
                             Util::List<uint64_t> &, get_children);

  CflatStructAddMethodReturn(l_Namespace, Transform, Math::Vector3 &,
                             get_world_position);

  CflatStructAddMethodReturn(l_Namespace, Transform,
                             Math::Quaternion &, get_world_rotation);

  CflatStructAddMethodReturn(l_Namespace, Transform, Math::Vector3 &,
                             get_world_scale);

  CflatStructAddMethodReturn(l_Namespace, Transform,
                             Low::Core::Entity, get_entity);
  CflatStructAddMethodVoidParams1(l_Namespace, Transform, void,
                                  set_entity, Low::Core::Entity);
}

static void register_lowcore_element()
{
  using namespace Low;
  using namespace Low::Core;
  using namespace Low::Core::UI;

  Cflat::Namespace *l_Namespace =
      Scripting::get_environment()->requestNamespace("Low::Core::UI");

  Cflat::Struct *type =
      Low::Core::Scripting::g_CflatStructs["Low::Core::UI::Element"];

  {
    Cflat::Namespace *l_UtilNamespace =
        Scripting::get_environment()->requestNamespace("Low::Util");

    CflatRegisterSTLVectorCustom(l_UtilNamespace, Low::Util::List,
                                 Low::Core::UI::Element);
  }

  CflatStructAddMethodReturn(l_Namespace, Element, bool, is_alive);
  CflatStructAddStaticMethodReturn(l_Namespace, Element, uint32_t,
                                   get_capacity);
  CflatStructAddStaticMethodReturnParams1(l_Namespace, Element,
                                          Low::Core::UI::Element,
                                          find_by_index, uint32_t);
  CflatStructAddStaticMethodReturnParams1(
      l_Namespace, Element, Low::Core::UI::Element, find_by_name,
      Low::Util::Name);

  CflatStructAddMethodReturn(l_Namespace, Element, Low::Util::Name,
                             get_name);
  CflatStructAddMethodVoidParams1(l_Namespace, Element, void,
                                  set_name, Low::Util::Name);

  CflatStructAddMethodReturnParams1(l_Namespace, Element, uint64_t,
                                    get_component, uint16_t);
  CflatStructAddMethodVoidParams1(l_Namespace, Element, void,
                                  add_component, Util::Handle);
  CflatStructAddMethodVoidParams1(l_Namespace, Element, void,
                                  remove_component, uint16_t);
  CflatStructAddMethodReturnParams1(l_Namespace, Element, bool,
                                    has_component, uint16_t);
  CflatStructAddMethodReturn(l_Namespace, Element,
                             UI::Component::Display, get_display);
}

static void register_lowcore_display()
{
  using namespace Low;
  using namespace Low::Core;
  using namespace Low::Core::UI::Component;

  Cflat::Namespace *l_Namespace =
      Scripting::get_environment()->requestNamespace(
          "Low::Core::UI::Component");

  Cflat::Struct *type = Low::Core::Scripting::g_CflatStructs
      ["Low::Core::UI::Component::Display"];

  {
    Cflat::Namespace *l_UtilNamespace =
        Scripting::get_environment()->requestNamespace("Low::Util");

    CflatRegisterSTLVectorCustom(l_UtilNamespace, Low::Util::List,
                                 Low::Core::UI::Component::Display);
  }

  CflatStructAddMethodReturn(l_Namespace, Display, bool, is_alive);
  CflatStructAddStaticMethodReturn(l_Namespace, Display, uint32_t,
                                   get_capacity);
  CflatStructAddStaticMethodReturnParams1(
      l_Namespace, Display, Low::Core::UI::Component::Display,
      find_by_index, uint32_t);

  CflatStructAddMethodReturn(l_Namespace, Display, Math::Vector3 &,
                             position);
  CflatStructAddMethodVoidParams1(l_Namespace, Display, void,
                                  position, Math::Vector3 &);

  CflatStructAddMethodReturn(l_Namespace, Display, Math::Quaternion &,
                             rotation);
  CflatStructAddMethodVoidParams1(l_Namespace, Display, void,
                                  rotation, Math::Quaternion &);

  CflatStructAddMethodReturn(l_Namespace, Display, Math::Vector3 &,
                             scale);
  CflatStructAddMethodVoidParams1(l_Namespace, Display, void, scale,
                                  Math::Vector3 &);

  CflatStructAddMethodReturn(l_Namespace, Display, uint64_t,
                             get_parent);
  CflatStructAddMethodVoidParams1(l_Namespace, Display, void,
                                  set_parent, uint64_t);

  CflatStructAddMethodReturn(l_Namespace, Display,
                             Util::List<uint64_t> &, get_children);

  CflatStructAddMethodReturn(l_Namespace, Display, Math::Vector3 &,
                             get_world_position);

  CflatStructAddMethodReturn(l_Namespace, Display, Math::Quaternion &,
                             get_world_rotation);

  CflatStructAddMethodReturn(l_Namespace, Display, Math::Vector3 &,
                             get_world_scale);

  CflatStructAddMethodReturn(l_Namespace, Display,
                             Low::Core::UI::Element, get_element);
  CflatStructAddMethodVoidParams1(l_Namespace, Display, void,
                                  set_element,
                                  Low::Core::UI::Element);
}

static void preregister_types()
{
  using namespace Low::Core;

  {
    using namespace Low::Core;
    Cflat::Namespace *l_Namespace =
        Scripting::get_environment()->requestNamespace("Low::Core");

    CflatRegisterStruct(l_Namespace, Entity);
    CflatStructAddBaseType(Scripting::get_environment(),
                           Low::Core::Entity, Low::Util::Handle);

    Scripting::g_CflatStructs["Low::Core::Entity"] = type;
  }

  {
    using namespace Low::Core::Component;
    Cflat::Namespace *l_Namespace =
        Scripting::get_environment()->requestNamespace(
            "Low::Core::Component");

    CflatRegisterStruct(l_Namespace, Transform);
    CflatStructAddBaseType(Scripting::get_environment(),
                           Low::Core::Component::Transform,
                           Low::Util::Handle);

    Scripting::g_CflatStructs["Low::Core::Component::Transform"] =
        type;
  }

  {
    using namespace Low::Core::UI;
    Cflat::Namespace *l_Namespace =
        Scripting::get_environment()->requestNamespace(
            "Low::Core::UI");

    CflatRegisterStruct(l_Namespace, Element);
    CflatStructAddBaseType(Scripting::get_environment(),
                           Low::Core::UI::Element, Low::Util::Handle);

    Scripting::g_CflatStructs["Low::Core::UI::Element"] = type;
  }

  {
    using namespace Low::Core::UI::Component;
    Cflat::Namespace *l_Namespace =
        Scripting::get_environment()->requestNamespace(
            "Low::Core::UI::Component");

    CflatRegisterStruct(l_Namespace, Display);
    CflatStructAddBaseType(Scripting::get_environment(),
                           Low::Core::UI::Component::Display,
                           Low::Util::Handle);

    Scripting::g_CflatStructs["Low::Core::UI::Component::Display"] =
        type;
  }
}
static void register_types()
{
  preregister_types();

  register_lowcore_entity();
  register_lowcore_transform();
  register_lowcore_element();
  register_lowcore_display();
}
// REGISTER_CFLAT_END

static void setup_environment()
{
  register_math();
  register_lowutil_name();
  register_lowutil_handle();
  register_lowutil_containers();
  register_types();
}
#endif
