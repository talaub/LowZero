#include "LowCoreScriptingEngine.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilHandle.h"
#include "LowUtilFileIO.h"
#include "LowUtilString.h"

#include "LowCoreMonoUtils.h"
#include "LowCoreMonoLog.inl"
#include "LowCoreMonoHandleHelper.inl"
#include "LowCoreMonoDebugGeometry.inl"
#include "LowCoreMonoTypeMethods.inl"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>

namespace Low {
  namespace Core {
    namespace ScriptingEngine {
      Mono::Context g_Context;

      static inline void initialize_handle_helper_functions()
      {
        mono_add_internal_call(
            "Low.Internal.HandleHelper::GetLivingInstancesCount",
            &Mono::handle_get_living_instances_count);

        mono_add_internal_call("Low.Internal.HandleHelper::GetLivingInstance",
                               &Mono::handle_get_living_instance);

        mono_add_internal_call("Low.Internal.HandleHelper::GetUlongValue",
                               &Mono::handle_get_ulong_value);
        mono_add_internal_call("Low.Internal.HandleHelper::SetUlongValue",
                               &Mono::handle_set_ulong_value);
        mono_add_internal_call("Low.Internal.HandleHelper::GetFloatValue",
                               &Mono::handle_get_generic_value);
        mono_add_internal_call("Low.Internal.HandleHelper::SetFloatValue",
                               &Mono::handle_set_float_value);
        mono_add_internal_call("Low.Internal.HandleHelper::SetVector3Value",
                               &Mono::handle_set_vector3_value);
        mono_add_internal_call("Low.Internal.HandleHelper::GetVector3Value",
                               &Mono::handle_get_vector3_value);
        mono_add_internal_call("Low.Internal.HandleHelper::SetQuaternionValue",
                               &Mono::handle_set_quat_value);
        mono_add_internal_call("Low.Internal.HandleHelper::GetQuaternionValue",
                               &Mono::handle_get_quat_value);
        mono_add_internal_call("Low.Internal.HandleHelper::SetNameValue",
                               &Mono::handle_set_name_value);
        mono_add_internal_call("Low.Internal.HandleHelper::GetNameValue",
                               &Mono::handle_get_name_value);
      }

      static inline void initialize_type_functions()
      {
        mono_add_internal_call("Low.Entity::GetComponent",
                               &Mono::entity_get_component);
      }

      static inline void initialize_log_functions()
      {
        mono_add_internal_call("Low.Log::Debug", &Mono::log_debug);
        mono_add_internal_call("Low.Log::Info", &Mono::log_info);
        mono_add_internal_call("Low.Log::Warning", &Mono::log_warn);
        mono_add_internal_call("Low.Log::Error", &Mono::log_error);
      }

      static inline void initialize_debug_functions()
      {
        mono_add_internal_call("Low.Debug::DrawSphere", &Mono::dg_draw_sphere);
      }

      static void initialize_functions()
      {
        initialize_log_functions();
        initialize_debug_functions();
        initialize_handle_helper_functions();
        initialize_type_functions();
      }

      static void initialize_type_class(Util::String p_TypeName,
                                        Util::String &p_ModuleName,
                                        uint16_t p_TypeId,
                                        Util::Yaml::Node &p_Node,
                                        Util::String &p_NamespaceString)
      {
        MonoClass *l_Class;
        Util::String l_LowString = "Low";
        if (Util::StringHelper::begins_with(p_ModuleName, l_LowString)) {
          l_Class = Mono::get_low_class(LOW_NAME(p_NamespaceString.c_str()),
                                        LOW_NAME(p_TypeName.c_str()));
        }

        if (!l_Class) {
          return;
        }

        Mono::register_type_class(p_TypeId, l_Class);

        MonoClassField *typeField =
            mono_class_get_field_from_name(l_Class, "type");

        /*
              MonoMethod *l_SetTypeMethod =
                  mono_class_get_method_from_name(l_Class, "SetType", 1);

              LOW_ASSERT(l_SetTypeMethod, "Found settype method");
        */

        MonoVTable *l_VTable =
            mono_class_vtable(Mono::get_context().domain, l_Class);

        mono_field_static_set_value(l_VTable, typeField, &p_TypeId);

        /*
        uint16_t l_TypeId = p_TypeId;

        void *l_Params[] = {&l_TypeId};

        MonoObject *exception = nullptr;
        mono_runtime_invoke(l_SetTypeMethod, nullptr, l_Params, &exception);
  */
      }

      static void initialize_type_classes()
      {
        Util::String l_TypesPath = LOW_DATA_PATH;
        l_TypesPath += "/_internal/type_configs";

        Util::String l_TypeIdsPath = l_TypesPath + "/typeids.yaml";
        Util::Yaml::Node l_TypeIdsNode =
            Util::Yaml::load_file(l_TypeIdsPath.c_str());

        Util::List<Util::String> l_FilePaths;

        Util::FileIO::list_directory(l_TypesPath.c_str(), l_FilePaths);
        Util::String l_Ending = ".types.yaml";

        for (Util::String &i_Path : l_FilePaths) {
          if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
            Util::Yaml::Node i_Node = Util::Yaml::load_file(i_Path.c_str());

            Util::String l_ModuleName = LOW_YAML_AS_STRING(i_Node["module"]);
            Util::Yaml::Node &i_ScriptingNamespaceNode = i_Node["namespace"];
            if (i_Node["scripting_namespace"]) {
              i_ScriptingNamespaceNode = i_Node["scripting_namespace"];
            }
            Util::String l_NamespaceString = "";

            for (uint32_t i = 0u; i < i_ScriptingNamespaceNode.size(); ++i) {
              if (i) {
                l_NamespaceString += ".";
              }
              l_NamespaceString +=
                  LOW_YAML_AS_STRING(i_ScriptingNamespaceNode[i]);
            }

            for (auto it = i_Node["types"].begin(); it != i_Node["types"].end();
                 ++it) {
              Util::String i_TypeName = LOW_YAML_AS_STRING(it->first);
              if (it->second["scripting_expose"] &&
                  l_TypeIdsNode[l_ModuleName.c_str()][i_TypeName.c_str()]) {
                initialize_type_class(
                    i_TypeName, l_ModuleName,
                    l_TypeIdsNode[l_ModuleName.c_str()][i_TypeName.c_str()]
                        .as<int>(),
                    it->second, l_NamespaceString);
              }
            }
          }
        }
      }

      void initialize()
      {
        Util::String l_AssembliesPath = LOW_DATA_PATH;
        l_AssembliesPath += "/../mono/lib/4.5";
        mono_set_assemblies_path(l_AssembliesPath.c_str());

        MonoDomain *l_RootDomain = mono_jit_init("LowScriptRuntime");

        LOW_ASSERT(l_RootDomain, "Could not initialize mono domain");

        MonoDomain *l_AppDomain =
            mono_domain_create_appdomain("LowAppDomain", nullptr);
        g_Context.domain = l_AppDomain;
        mono_domain_set(l_AppDomain, true);

        MonoAssembly *l_LowAssembly = 0;
        {
          Util::String l_DllPath = LOW_DATA_PATH;
          l_DllPath += "/../lowscriptingapi.dll";
          Util::String l_DllPathString = l_DllPath.c_str();
          // LOW_LOG_DEBUG << l_DllPath << LOW_LOG_END;
          l_LowAssembly = Mono::load_assembly(l_DllPathString);
          LOW_ASSERT(l_LowAssembly, "Failed to load LowScripting API assembly");
        }

        g_Context.low_assembly = l_LowAssembly;

        Util::String l_DllPath = LOW_DATA_PATH;
        l_DllPath += "/scripts/bin/Debug/net6.0/scripts.dll";

        g_Context.game_assembly = Mono::load_assembly(l_DllPath);

        g_Context.low_image = mono_assembly_get_image(g_Context.low_assembly);

        Mono::set_context(g_Context);

        initialize_type_classes();
        initialize_functions();
      }

      void cleanup()
      {
      }
    } // namespace ScriptingEngine
  }   // namespace Core
} // namespace Low
