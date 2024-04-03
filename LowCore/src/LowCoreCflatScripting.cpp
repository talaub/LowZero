#include "LowCoreCflatScripting.h"

#include "LowCore.h"
#include "LowCoreUi.h"

#include "LowUtilLogger.h"
#include "LowUtilContainers.h"
#include "LowUtilFileSystem.h"
#include "LowUtilProfiler.h"
#include "LowUtilString.h"

#include "CflatHelper.h"

#include "LowMathVectorUtil.h"

// REGISTER_CFLAT_INCLUDES_BEGIN
#include "LowCoreEntity.h"
#include "LowCoreTransform.h"
#include "LowCoreCamera.h"
#include "LowCoreUiElement.h"
#include "LowCoreUiView.h"
#include "LowCoreUiDisplay.h"
#include "LowCoreUiText.h"
// REGISTER_CFLAT_INCLUDES_END

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

            // get_environment()->load(i_FileWatcher.path.c_str());

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
    CflatRegisterStruct(l_Namespace, Color);
    CflatStructAddMember(l_Namespace, Color, float, x);
    CflatStructAddMember(l_Namespace, Color, float, y);
    CflatStructAddMember(l_Namespace, Color, float, z);
    CflatStructAddMember(l_Namespace, Color, float, w);
    CflatStructAddConstructorParams4(l_Namespace, Color, float, float,
                                     float, float);
    CflatStructAddCopyConstructor(l_Namespace, Color);

    CflatRegisterFunctionReturnParams2(l_Namespace,
                                       Vector4, operator*,
                                       const Color &, const Color &);
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
    CflatStructAddConstructor(l_Namespace, Vector2);
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
    CflatStructAddConstructor(l_Namespace, UVector2);

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
  {
    using namespace Low;
    using namespace Low::Math;
    using namespace Low::Math::VectorUtil;

    Cflat::Namespace *l_Namespace =
        Scripting::get_environment()->requestNamespace(
            "Low::Math::VectorUtil");

    CflatRegisterFunctionReturnParams3(
        l_Namespace, Low::Math::Vector3, lerp, Low::Math::Vector3 &,
        Low::Math::Vector3 &, float);
    CflatRegisterFunctionReturnParams3(
        l_Namespace, Low::Math::Vector2, lerp, Low::Math::Vector2 &,
        Low::Math::Vector2 &, float);
  }
}

#include "LowUtilEnums.h"
static void register_lowutil_enums()
{
  using namespace Low::Core;
  using namespace Low::Util;

  Cflat::Namespace *l_Namespace =
      Scripting::get_environment()->requestNamespace("Low::Util");

  {
    CflatRegisterEnumClass(l_Namespace, KeyboardButton);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, Q);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, W);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, E);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, R);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, T);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, Y);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, U);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, I);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, O);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, P);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, A);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, S);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, D);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, F);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, G);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, H);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, J);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, K);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, L);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, Z);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, X);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, C);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, V);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, B);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, N);
    CflatEnumClassAddValue(l_Namespace, KeyboardButton, M);
  }

  {
    CflatRegisterEnumClass(l_Namespace, MouseButton);
    CflatEnumClassAddValue(l_Namespace, MouseButton, LEFT);
    CflatEnumClassAddValue(l_Namespace, MouseButton, RIGHT);
  }
}

static void register_lowcore()
{
  using namespace Low;
  using namespace Low::Core;

  Cflat::Namespace *l_Namespace =
      Scripting::get_environment()->requestNamespace("Low::Core");

  CflatRegisterFunctionVoidParams1(l_Namespace, void, game_dimensions,
                                   Low::Math::UVector2 &);
}

#include "LowCoreInput.h"
static void register_lowcore_input()
{

  using namespace Low;
  using namespace Low::Core;
  using namespace Low::Core::Input;

  Cflat::Namespace *l_Namespace =
      Scripting::get_environment()->requestNamespace(
          "Low::Core::Input");

  CflatRegisterFunctionReturnParams1(
      l_Namespace, bool, keyboard_button_down, Util::KeyboardButton);
  CflatRegisterFunctionReturnParams1(
      l_Namespace, bool, keyboard_button_up, Util::KeyboardButton);

  CflatRegisterFunctionReturnParams1(
      l_Namespace, bool, mouse_button_down, Util::MouseButton);
  CflatRegisterFunctionReturnParams1(
      l_Namespace, bool, mouse_button_up, Util::MouseButton);
}

static void register_lowcore_ui()
{
  using namespace Low;
  using namespace Low::Core;
  using namespace Low::Core::UI;

  Cflat::Namespace *l_Namespace =
      Scripting::get_environment()->requestNamespace("Low::Core::UI");

  CflatRegisterFunctionReturn(l_Namespace, Element,
                              get_hovered_element);
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

  Scripting::get_environment()->defineMacro("_LNAME(x)",
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

static void register_lowutil_string()
{
  {
    Cflat::Helper::registerStdString(
        Low::Core::Scripting::get_environment());
  }

  {
    using namespace std;
    Cflat::Namespace *l_Namespace =
        Low::Core::Scripting::get_environment()->requestNamespace(
            "std");
    CflatRegisterFunctionReturnParams1(l_Namespace, string, to_string,
                                       float);
  }

  {
    CflatRegisterClass(Low::Core::Scripting::get_environment(),
                       Low::Util::String);
    CflatClassAddConstructor(Low::Core::Scripting::get_environment(),
                             Low::Util::String);
    CflatClassAddConstructorParams1(
        Low::Core::Scripting::get_environment(), Low::Util::String,
        const char *);
    CflatClassAddMethodReturn(Low::Core::Scripting::get_environment(),
                              Low::Util::String, bool, empty);
    CflatClassAddMethodReturn(Low::Core::Scripting::get_environment(),
                              Low::Util::String, size_t, size);
    CflatClassAddMethodReturn(Low::Core::Scripting::get_environment(),
                              Low::Util::String, size_t, length);
    CflatClassAddMethodReturnParams1(
        Low::Core::Scripting::get_environment(), Low::Util::String,
        Low::Util::String &, assign, const Low::Util::String &);
    CflatClassAddMethodReturnParams1(
        Low::Core::Scripting::get_environment(), Low::Util::String,
        Low::Util::String &, assign, const char *);
    CflatClassAddMethodReturnParams1(
        Low::Core::Scripting::get_environment(), Low::Util::String,
        Low::Util::String &, append, const Low::Util::String &);
    CflatClassAddMethodReturnParams1(
        Low::Core::Scripting::get_environment(), Low::Util::String,
        Low::Util::String &, append, const char *);
    CflatClassAddMethodReturn(Low::Core::Scripting::get_environment(),
                              Low::Util::String, const char *, c_str);
    CflatClassAddMethodReturnParams1(
        Low::Core::Scripting::get_environment(), Low::Util::String,
        char &, at, size_t);

    CflatRegisterFunctionReturnParams2(
        Low::Core::Scripting::get_environment(),
        Low::Util::String, operator+, const Low::Util::String &,
        const Low::Util::String &);
    CflatRegisterFunctionReturnParams2(
        Low::Core::Scripting::get_environment(),
        Low::Util::String, operator+, const Low::Util::String &,
        const char *);
    CflatRegisterFunctionReturnParams2(
        Low::Core::Scripting::get_environment(),
        Low::Util::String, operator+, const Low::Util::String &, int);
  }

  {
    using namespace Low::Util::StringHelper;

    Cflat::Namespace *l_Namespace =
        Low::Core::Scripting::get_environment()->requestNamespace(
            "Low::Util::StringHelper");

    CflatRegisterFunctionVoidParams2(l_Namespace, void, append,
                                     Low::Util::String &, int);
  }

  {
    Low::Core::Scripting::get_environment()->defineMacro(
        "LOW_TO_STRING(x)",
        "Low::Util::String(std::to_string(x).c_str())");
  }
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

#include "LowUtilLogger.h"
static void register_lowutil_logger()
{
  using namespace Low::Core;
  using namespace Low::Util;
  using namespace Low::Util::Log;
  using namespace Low::Util::Log::LogLevel;

  Cflat::Namespace *l_Namespace =
      Scripting::get_environment()->requestNamespace(
          "Low::Util::Log");

  {
    Cflat::Namespace *l_EnumNamespace =
        Scripting::get_environment()->requestNamespace(
            "Low::Util::Log::LogLevel");

    CflatRegisterEnum(l_EnumNamespace, Enum);
    CflatEnumAddValue(l_EnumNamespace, Enum, INFO);
    CflatEnumAddValue(l_EnumNamespace, Enum, DEBUG);
    CflatEnumAddValue(l_EnumNamespace, Enum, WARN);
    CflatEnumAddValue(l_EnumNamespace, Enum, ERROR);
    CflatEnumAddValue(l_EnumNamespace, Enum, PROFILE);
  }

  {
    CflatRegisterEnumClass(l_Namespace, LogLineEnd);
    CflatEnumClassAddValue(l_Namespace, LogLineEnd, LINE_END);
  }

  {
    CflatRegisterStruct(l_Namespace, LogStream);
    CflatClassAddMethodReturnParams1(l_Namespace, LogStream,
                                     LogStream &, operator<<, int);
    CflatClassAddMethodReturnParams1(l_Namespace, LogStream,
                                     LogStream &, operator<<, bool);
    CflatClassAddMethodReturnParams1(l_Namespace, LogStream,
                                     LogStream &, operator<<, size_t);
    CflatClassAddMethodReturnParams1(
        l_Namespace, LogStream, LogStream &, operator<<, uint32_t);
    CflatClassAddMethodReturnParams1(
        l_Namespace, LogStream, LogStream &, operator<<, int32_t);
    CflatClassAddMethodReturnParams1(
        l_Namespace, LogStream, LogStream &, operator<<, uint16_t);
    CflatClassAddMethodReturnParams1(
        l_Namespace, LogStream, LogStream &, operator<<, int16_t);
    CflatClassAddMethodReturnParams1(
        l_Namespace, LogStream, LogStream &, operator<<, uint8_t);
    CflatClassAddMethodReturnParams1(l_Namespace, LogStream,
                                     LogStream &, operator<<, int8_t);
    CflatClassAddMethodReturnParams1(l_Namespace, LogStream,
                                     LogStream &, operator<<, float);
    CflatClassAddMethodReturnParams1(l_Namespace, LogStream,
                                     LogStream &, operator<<,
                                     const char *);
    CflatClassAddMethodReturnParams1(l_Namespace, LogStream,
                                     LogStream &, operator<<,
                                     Low::Util::String);
    /*
    CflatClassAddMethodReturnParams1(
        l_Namespace, LogStream, LogStream &, operator<<, String &);
        */
    CflatClassAddMethodReturnParams1(l_Namespace, LogStream,
                                     LogStream &, operator<<, Name);
    CflatClassAddMethodReturnParams1(l_Namespace, LogStream,
                                     LogStream &, operator<<,
                                     Low::Math::Vector3 &);
    CflatClassAddMethodReturnParams1(
        l_Namespace, LogStream, LogStream &, operator<<, LogLineEnd);
  }

  {
    CflatRegisterFunctionReturnParams3(l_Namespace, LogStream &,
                                       begin_log, uint8_t,
                                       const char *, bool);
  }

  {
    Scripting::get_environment()->defineMacro(
        "LOW_LOG_DEBUG",
        "Low::Util::Log::begin_log("
        "Low::Util::Log::LogLevel::DEBUG, \"scripting\", false)");
    Scripting::get_environment()->defineMacro(
        "LOW_LOG_INFO",
        "Low::Util::Log::begin_log("
        "Low::Util::Log::LogLevel::INFO, \"scripting\", false)");
    Scripting::get_environment()->defineMacro(
        "LOW_LOG_ERROR",
        "Low::Util::Log::begin_log("
        "Low::Util::Log::LogLevel::ERROR, \"scripting\", false)");

    Scripting::get_environment()->defineMacro(
        "LOW_LOG_END", "Low::Util::Log::LogLineEnd::LINE_END");
  }
}

// REGISTER_CFLAT_BEGIN
static void register_lowcore_entity()
{
  using namespace Low;
  using namespace Low::Core;

  Cflat::Namespace *l_Namespace =
      Low::Core::Scripting::get_environment()->requestNamespace(
          "Low::Core");

  Cflat::Struct *type =
      Scripting::g_CflatStructs["Low::Core::Entity"];

  {
    Cflat::Namespace *l_UtilNamespace =
        Low::Core::Scripting::get_environment()->requestNamespace(
            "Low::Util");

    CflatRegisterSTLVectorCustom(
        Low::Core::Scripting::get_environment(), Low::Util::List,
        Low::Core::Entity);
  }

  CflatStructAddConstructorParams1(
      Low::Core::Scripting::get_environment(), Low::Core::Entity,
      uint64_t);
  CflatStructAddStaticMember(Low::Core::Scripting::get_environment(),
                             Low::Core::Entity, uint16_t, TYPE_ID);
  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::Entity, bool, is_alive);
  CflatStructAddMethodVoid(Low::Core::Scripting::get_environment(),
                           Low::Core::Entity, void, destroy);
  CflatStructAddStaticMethodReturn(
      Low::Core::Scripting::get_environment(), Low::Core::Entity,
      uint32_t, get_capacity);
  CflatStructAddStaticMethodReturnParams1(
      Low::Core::Scripting::get_environment(), Low::Core::Entity,
      Low::Core::Entity, find_by_index, uint32_t);
  CflatStructAddStaticMethodReturnParams1(
      Low::Core::Scripting::get_environment(), Low::Core::Entity,
      Low::Core::Entity, find_by_name, Low::Util::Name);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::Entity, Low::Util::Name,
                             get_name);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(), Low::Core::Entity,
      void, set_name, Low::Util::Name);

  CflatStructAddMethodReturnParams1(
      Low::Core::Scripting::get_environment(), Low::Core::Entity,
      uint64_t, get_component, uint16_t);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(), Low::Core::Entity,
      void, add_component, Low::Util::Handle);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(), Low::Core::Entity,
      void, remove_component, uint16_t);
  CflatStructAddMethodReturnParams1(
      Low::Core::Scripting::get_environment(), Low::Core::Entity,
      bool, has_component, uint16_t);
  CflatStructAddMethodReturn(
      Low::Core::Scripting::get_environment(), Low::Core::Entity,
      Low::Core::Component::Transform, get_transform);
}

static void register_lowcore_transform()
{
  using namespace Low;
  using namespace Low::Core;
  using namespace ::Low::Core::Component;

  Cflat::Namespace *l_Namespace =
      Low::Core::Scripting::get_environment()->requestNamespace(
          "Low::Core::Component");

  Cflat::Struct *type =
      Scripting::g_CflatStructs["Low::Core::Component::Transform"];

  {
    Cflat::Namespace *l_UtilNamespace =
        Low::Core::Scripting::get_environment()->requestNamespace(
            "Low::Util");

    CflatRegisterSTLVectorCustom(
        Low::Core::Scripting::get_environment(), Low::Util::List,
        Low::Core::Component::Transform);
  }

  CflatStructAddConstructorParams1(
      Low::Core::Scripting::get_environment(),
      Low::Core::Component::Transform, uint64_t);
  CflatStructAddStaticMember(Low::Core::Scripting::get_environment(),
                             Low::Core::Component::Transform,
                             uint16_t, TYPE_ID);
  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::Component::Transform, bool,
                             is_alive);
  CflatStructAddMethodVoid(Low::Core::Scripting::get_environment(),
                           Low::Core::Component::Transform, void,
                           destroy);
  CflatStructAddStaticMethodReturn(
      Low::Core::Scripting::get_environment(),
      Low::Core::Component::Transform, uint32_t, get_capacity);
  CflatStructAddStaticMethodReturnParams1(
      Low::Core::Scripting::get_environment(),
      Low::Core::Component::Transform,
      Low::Core::Component::Transform, find_by_index, uint32_t);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::Component::Transform,
                             Low::Math::Vector3 &, position);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(),
      Low::Core::Component::Transform, void, position,
      Low::Math::Vector3 &);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::Component::Transform,
                             Low::Math::Quaternion &, rotation);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(),
      Low::Core::Component::Transform, void, rotation,
      Low::Math::Quaternion &);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::Component::Transform,
                             Low::Math::Vector3 &, scale);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(),
      Low::Core::Component::Transform, void, scale,
      Low::Math::Vector3 &);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::Component::Transform,
                             uint64_t, get_parent);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(),
      Low::Core::Component::Transform, void, set_parent, uint64_t);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::Component::Transform,
                             Low::Util::List<uint64_t> &,
                             get_children);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::Component::Transform,
                             Low::Math::Vector3 &,
                             get_world_position);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::Component::Transform,
                             Low::Math::Quaternion &,
                             get_world_rotation);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::Component::Transform,
                             Low::Math::Vector3 &, get_world_scale);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::Component::Transform,
                             Low::Core::Entity, get_entity);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(),
      Low::Core::Component::Transform, void, set_entity,
      Low::Core::Entity);
}

static void register_lowcore_camera()
{
  using namespace Low;
  using namespace Low::Core;
  using namespace ::Low::Core::Component;

  Cflat::Namespace *l_Namespace =
      Low::Core::Scripting::get_environment()->requestNamespace(
          "Low::Core::Component");

  Cflat::Struct *type =
      Scripting::g_CflatStructs["Low::Core::Component::Camera"];

  {
    Cflat::Namespace *l_UtilNamespace =
        Low::Core::Scripting::get_environment()->requestNamespace(
            "Low::Util");

    CflatRegisterSTLVectorCustom(
        Low::Core::Scripting::get_environment(), Low::Util::List,
        Low::Core::Component::Camera);
  }

  CflatStructAddConstructorParams1(
      Low::Core::Scripting::get_environment(),
      Low::Core::Component::Camera, uint64_t);
  CflatStructAddStaticMember(Low::Core::Scripting::get_environment(),
                             Low::Core::Component::Camera, uint16_t,
                             TYPE_ID);
  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::Component::Camera, bool,
                             is_alive);
  CflatStructAddMethodVoid(Low::Core::Scripting::get_environment(),
                           Low::Core::Component::Camera, void,
                           destroy);
  CflatStructAddStaticMethodReturn(
      Low::Core::Scripting::get_environment(),
      Low::Core::Component::Camera, uint32_t, get_capacity);
  CflatStructAddStaticMethodReturnParams1(
      Low::Core::Scripting::get_environment(),
      Low::Core::Component::Camera, Low::Core::Component::Camera,
      find_by_index, uint32_t);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::Component::Camera, bool,
                             is_active);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::Component::Camera, float,
                             get_fov);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(),
      Low::Core::Component::Camera, void, set_fov, float);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::Component::Camera,
                             Low::Core::Entity, get_entity);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(),
      Low::Core::Component::Camera, void, set_entity,
      Low::Core::Entity);
}

static void register_lowcore_element()
{
  using namespace Low;
  using namespace Low::Core;
  using namespace ::Low::Core::UI;

  Cflat::Namespace *l_Namespace =
      Low::Core::Scripting::get_environment()->requestNamespace(
          "Low::Core::UI");

  Cflat::Struct *type =
      Scripting::g_CflatStructs["Low::Core::UI::Element"];

  {
    Cflat::Namespace *l_UtilNamespace =
        Low::Core::Scripting::get_environment()->requestNamespace(
            "Low::Util");

    CflatRegisterSTLVectorCustom(
        Low::Core::Scripting::get_environment(), Low::Util::List,
        Low::Core::UI::Element);
  }

  CflatStructAddConstructorParams1(
      Low::Core::Scripting::get_environment(), Low::Core::UI::Element,
      uint64_t);
  CflatStructAddStaticMember(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::Element, uint16_t,
                             TYPE_ID);
  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::Element, bool, is_alive);
  CflatStructAddMethodVoid(Low::Core::Scripting::get_environment(),
                           Low::Core::UI::Element, void, destroy);
  CflatStructAddStaticMethodReturn(
      Low::Core::Scripting::get_environment(), Low::Core::UI::Element,
      uint32_t, get_capacity);
  CflatStructAddStaticMethodReturnParams1(
      Low::Core::Scripting::get_environment(), Low::Core::UI::Element,
      Low::Core::UI::Element, find_by_index, uint32_t);
  CflatStructAddStaticMethodReturnParams1(
      Low::Core::Scripting::get_environment(), Low::Core::UI::Element,
      Low::Core::UI::Element, find_by_name, Low::Util::Name);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::Element,
                             Low::Core::UI::View, get_view);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(), Low::Core::UI::Element,
      void, set_view, Low::Core::UI::View);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::Element, bool,
                             is_click_passthrough);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(), Low::Core::UI::Element,
      void, set_click_passthrough, bool);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::Element, Low::Util::Name,
                             get_name);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(), Low::Core::UI::Element,
      void, set_name, Low::Util::Name);

  CflatStructAddMethodReturnParams1(
      Low::Core::Scripting::get_environment(), Low::Core::UI::Element,
      uint64_t, get_component, uint16_t);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(), Low::Core::UI::Element,
      void, add_component, Low::Util::Handle);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(), Low::Core::UI::Element,
      void, remove_component, uint16_t);
  CflatStructAddMethodReturnParams1(
      Low::Core::Scripting::get_environment(), Low::Core::UI::Element,
      bool, has_component, uint16_t);
  CflatStructAddMethodReturn(
      Low::Core::Scripting::get_environment(), Low::Core::UI::Element,
      Low::Core::UI::Component::Display, get_display);
}

static void register_lowcore_view()
{
  using namespace Low;
  using namespace Low::Core;
  using namespace ::Low::Core::UI;

  Cflat::Namespace *l_Namespace =
      Low::Core::Scripting::get_environment()->requestNamespace(
          "Low::Core::UI");

  Cflat::Struct *type =
      Scripting::g_CflatStructs["Low::Core::UI::View"];

  {
    Cflat::Namespace *l_UtilNamespace =
        Low::Core::Scripting::get_environment()->requestNamespace(
            "Low::Util");

    CflatRegisterSTLVectorCustom(
        Low::Core::Scripting::get_environment(), Low::Util::List,
        Low::Core::UI::View);
  }

  CflatStructAddConstructorParams1(
      Low::Core::Scripting::get_environment(), Low::Core::UI::View,
      uint64_t);
  CflatStructAddStaticMember(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::View, uint16_t, TYPE_ID);
  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::View, bool, is_alive);
  CflatStructAddMethodVoid(Low::Core::Scripting::get_environment(),
                           Low::Core::UI::View, void, destroy);
  CflatStructAddStaticMethodReturn(
      Low::Core::Scripting::get_environment(), Low::Core::UI::View,
      uint32_t, get_capacity);
  CflatStructAddStaticMethodReturnParams1(
      Low::Core::Scripting::get_environment(), Low::Core::UI::View,
      Low::Core::UI::View, find_by_index, uint32_t);
  CflatStructAddStaticMethodReturnParams1(
      Low::Core::Scripting::get_environment(), Low::Core::UI::View,
      Low::Core::UI::View, find_by_name, Low::Util::Name);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::View,
                             Low::Math::Vector2 &, pixel_position);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(), Low::Core::UI::View,
      void, pixel_position, Low::Math::Vector2 &);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::View, float, rotation);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(), Low::Core::UI::View,
      void, rotation, float);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::View, float,
                             scale_multiplier);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(), Low::Core::UI::View,
      void, scale_multiplier, float);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::View, uint32_t,
                             layer_offset);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(), Low::Core::UI::View,
      void, layer_offset, uint32_t);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::View, Low::Util::Name,
                             get_name);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(), Low::Core::UI::View,
      void, set_name, Low::Util::Name);

  CflatStructAddMethodReturnParams1(
      Low::Core::Scripting::get_environment(), Low::Core::UI::View,
      Low::Core::UI::View, spawn_instance, Low::Util::Name);
  CflatStructAddMethodReturnParams1(
      Low::Core::Scripting::get_environment(), Low::Core::UI::View,
      Low::Core::UI::Element, find_element_by_name, Low::Util::Name);
}

static void register_lowcore_display()
{
  using namespace Low;
  using namespace Low::Core;
  using namespace ::Low::Core::UI::Component;

  Cflat::Namespace *l_Namespace =
      Low::Core::Scripting::get_environment()->requestNamespace(
          "Low::Core::UI::Component");

  Cflat::Struct *type =
      Scripting::g_CflatStructs["Low::Core::UI::Component::Display"];

  {
    Cflat::Namespace *l_UtilNamespace =
        Low::Core::Scripting::get_environment()->requestNamespace(
            "Low::Util");

    CflatRegisterSTLVectorCustom(
        Low::Core::Scripting::get_environment(), Low::Util::List,
        Low::Core::UI::Component::Display);
  }

  CflatStructAddConstructorParams1(
      Low::Core::Scripting::get_environment(),
      Low::Core::UI::Component::Display, uint64_t);
  CflatStructAddStaticMember(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::Component::Display,
                             uint16_t, TYPE_ID);
  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::Component::Display, bool,
                             is_alive);
  CflatStructAddMethodVoid(Low::Core::Scripting::get_environment(),
                           Low::Core::UI::Component::Display, void,
                           destroy);
  CflatStructAddStaticMethodReturn(
      Low::Core::Scripting::get_environment(),
      Low::Core::UI::Component::Display, uint32_t, get_capacity);
  CflatStructAddStaticMethodReturnParams1(
      Low::Core::Scripting::get_environment(),
      Low::Core::UI::Component::Display,
      Low::Core::UI::Component::Display, find_by_index, uint32_t);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::Component::Display,
                             Low::Math::Vector2 &, pixel_position);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(),
      Low::Core::UI::Component::Display, void, pixel_position,
      Low::Math::Vector2 &);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::Component::Display, float,
                             rotation);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(),
      Low::Core::UI::Component::Display, void, rotation, float);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::Component::Display,
                             Low::Math::Vector2 &, pixel_scale);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(),
      Low::Core::UI::Component::Display, void, pixel_scale,
      Low::Math::Vector2 &);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::Component::Display,
                             uint32_t, layer);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(),
      Low::Core::UI::Component::Display, void, layer, uint32_t);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::Component::Display,
                             uint64_t, get_parent);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(),
      Low::Core::UI::Component::Display, void, set_parent, uint64_t);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::Component::Display,
                             Low::Util::List<uint64_t> &,
                             get_children);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::Component::Display,
                             Low::Math::Vector2 &,
                             get_absolute_pixel_position);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::Component::Display, float,
                             get_absolute_rotation);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::Component::Display,
                             Low::Math::Vector2 &,
                             get_absolute_pixel_scale);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::Component::Display,
                             uint32_t, get_absolute_layer);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::Component::Display,
                             Low::Core::UI::Element, get_element);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(),
      Low::Core::UI::Component::Display, void, set_element,
      Low::Core::UI::Element);
}

static void register_lowcore_text()
{
  using namespace Low;
  using namespace Low::Core;
  using namespace ::Low::Core::UI::Component;

  Cflat::Namespace *l_Namespace =
      Low::Core::Scripting::get_environment()->requestNamespace(
          "Low::Core::UI::Component");

  Cflat::Struct *type =
      Scripting::g_CflatStructs["Low::Core::UI::Component::Text"];

  {
    Cflat::Namespace *l_UtilNamespace =
        Low::Core::Scripting::get_environment()->requestNamespace(
            "Low::Util");

    CflatRegisterSTLVectorCustom(
        Low::Core::Scripting::get_environment(), Low::Util::List,
        Low::Core::UI::Component::Text);
  }

  CflatStructAddConstructorParams1(
      Low::Core::Scripting::get_environment(),
      Low::Core::UI::Component::Text, uint64_t);
  CflatStructAddStaticMember(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::Component::Text, uint16_t,
                             TYPE_ID);
  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::Component::Text, bool,
                             is_alive);
  CflatStructAddMethodVoid(Low::Core::Scripting::get_environment(),
                           Low::Core::UI::Component::Text, void,
                           destroy);
  CflatStructAddStaticMethodReturn(
      Low::Core::Scripting::get_environment(),
      Low::Core::UI::Component::Text, uint32_t, get_capacity);
  CflatStructAddStaticMethodReturnParams1(
      Low::Core::Scripting::get_environment(),
      Low::Core::UI::Component::Text, Low::Core::UI::Component::Text,
      find_by_index, uint32_t);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::Component::Text,
                             Low::Util::String &, get_text);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(),
      Low::Core::UI::Component::Text, void, set_text,
      Low::Util::String &);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::Component::Text,
                             Low::Math::Color &, get_color);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(),
      Low::Core::UI::Component::Text, void, set_color,
      Low::Math::Color &);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::Component::Text, float,
                             get_size);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(),
      Low::Core::UI::Component::Text, void, set_size, float);

  CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(),
                             Low::Core::UI::Component::Text,
                             Low::Core::UI::Element, get_element);
  CflatStructAddMethodVoidParams1(
      Low::Core::Scripting::get_environment(),
      Low::Core::UI::Component::Text, void, set_element,
      Low::Core::UI::Element);
}

static void preregister_types()
{
  using namespace Low::Core;

  {
    using namespace Low::Core;
    Cflat::Namespace *l_Namespace =
        Low::Core::Scripting::get_environment()->requestNamespace(
            "Low::Core");

    CflatRegisterStruct(l_Namespace, Entity);
    CflatStructAddBaseType(Low::Core::Scripting::get_environment(),
                           Low::Core::Entity, Low::Util::Handle);

    Scripting::g_CflatStructs["Low::Core::Entity"] = type;
  }

  {
    using namespace Low::Core::Component;
    Cflat::Namespace *l_Namespace =
        Low::Core::Scripting::get_environment()->requestNamespace(
            "Low::Core::Component");

    CflatRegisterStruct(l_Namespace, Transform);
    CflatStructAddBaseType(Low::Core::Scripting::get_environment(),
                           Low::Core::Component::Transform,
                           Low::Util::Handle);

    Scripting::g_CflatStructs["Low::Core::Component::Transform"] =
        type;
  }

  {
    using namespace Low::Core::Component;
    Cflat::Namespace *l_Namespace =
        Low::Core::Scripting::get_environment()->requestNamespace(
            "Low::Core::Component");

    CflatRegisterStruct(l_Namespace, Camera);
    CflatStructAddBaseType(Low::Core::Scripting::get_environment(),
                           Low::Core::Component::Camera,
                           Low::Util::Handle);

    Scripting::g_CflatStructs["Low::Core::Component::Camera"] = type;
  }

  {
    using namespace Low::Core::UI;
    Cflat::Namespace *l_Namespace =
        Low::Core::Scripting::get_environment()->requestNamespace(
            "Low::Core::UI");

    CflatRegisterStruct(l_Namespace, Element);
    CflatStructAddBaseType(Low::Core::Scripting::get_environment(),
                           Low::Core::UI::Element, Low::Util::Handle);

    Scripting::g_CflatStructs["Low::Core::UI::Element"] = type;
  }

  {
    using namespace Low::Core::UI;
    Cflat::Namespace *l_Namespace =
        Low::Core::Scripting::get_environment()->requestNamespace(
            "Low::Core::UI");

    CflatRegisterStruct(l_Namespace, View);
    CflatStructAddBaseType(Low::Core::Scripting::get_environment(),
                           Low::Core::UI::View, Low::Util::Handle);

    Scripting::g_CflatStructs["Low::Core::UI::View"] = type;
  }

  {
    using namespace Low::Core::UI::Component;
    Cflat::Namespace *l_Namespace =
        Low::Core::Scripting::get_environment()->requestNamespace(
            "Low::Core::UI::Component");

    CflatRegisterStruct(l_Namespace, Display);
    CflatStructAddBaseType(Low::Core::Scripting::get_environment(),
                           Low::Core::UI::Component::Display,
                           Low::Util::Handle);

    Scripting::g_CflatStructs["Low::Core::UI::Component::Display"] =
        type;
  }

  {
    using namespace Low::Core::UI::Component;
    Cflat::Namespace *l_Namespace =
        Low::Core::Scripting::get_environment()->requestNamespace(
            "Low::Core::UI::Component");

    CflatRegisterStruct(l_Namespace, Text);
    CflatStructAddBaseType(Low::Core::Scripting::get_environment(),
                           Low::Core::UI::Component::Text,
                           Low::Util::Handle);

    Scripting::g_CflatStructs["Low::Core::UI::Component::Text"] =
        type;
  }
}
static void register_types()
{
  register_lowcore_entity();
  register_lowcore_transform();
  register_lowcore_camera();
  register_lowcore_element();
  register_lowcore_view();
  register_lowcore_display();
  register_lowcore_text();
}
// REGISTER_CFLAT_END

static void setup_environment()
{
  register_math();
  register_lowutil_enums();
  register_lowutil_name();
  register_lowutil_handle();
  register_lowutil_string();
  register_lowutil_logger();
  register_lowutil_containers();
  preregister_types();
  register_types();
  register_lowcore();
  register_lowcore_input();
  register_lowcore_ui();
}
#endif
