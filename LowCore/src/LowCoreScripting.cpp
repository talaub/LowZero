#include "LowCoreScripting.h"

#include "LowCoreEventManager.h"
#include "LowCoreEntity.h"
#include "LowCoreGameplaySystem.h"
#include "LowCoreScriptAsset.h"
#include "LowCoreScriptModule.h"
#include "LowCoreScriptClass.h"
#include "LowCoreUiController.h"
#include "LowCoreUiElement.h"
#include "LowUtilAssert.h"
#include "LowUtilFileIO.h"
#include "LowUtilHandle.h"
#include "LowUtilLogger.h"
#include "angelscript.h"

#include <angelscript.h>
#include <imgui.h>
#include <scriptbuilder/scriptbuilder.h>

#include <cctype>
#include <iostream>
#include <string>

namespace Low {
  namespace Core {
    void expose_types(asIScriptEngine *p_Engine);
    void register_types(asIScriptEngine *p_Engine);
    namespace Scripting {
      static asIScriptEngine *g_Engine = nullptr;
      static asIScriptContext *g_TickContext = nullptr;

      static bool g_Initialized = false;

      void expose(asIScriptEngine *p_Engine);
      void register_interfaces(asIScriptEngine *p_Engine);

      static bool load_script_to_file(const char *p_Path,
                                      Util::String &p_Source)
      {
        LOW_ASSERT_ERROR_RETURN_FALSE(
            Util::FileIO::file_exists_sync(p_Path),
            "Script file does not exist");

        Low::Util::FileIO::File l_File = Low::Util::FileIO::open(
            p_Path, Low::Util::FileIO::FileMode::READ_BYTES);

        const u32 l_Size = Low::Util::FileIO::size_sync(l_File);
        LOW_ASSERT_ERROR_RETURN_FALSE(l_Size > 0,
                                      "Script file is empty");

        p_Source.resize(l_Size);
        Low::Util::FileIO::read_sync(l_File, p_Source.data());

        Low::Util::FileIO::close(l_File);

        return true;
      }

      static bool has_metadata(CScriptBuilder &p_Builder,
                               asIScriptFunction *p_Function,
                               const char *p_Metadata)
      {
        auto l_Metadata = p_Builder.GetMetadataForFunc(p_Function);

        for (const auto &i_Metadata : l_Metadata) {
          if (i_Metadata == p_Metadata) {
            return true;
          }
        }

        return false;
      }

      static Util::String
      trim_metadata_value(const Util::String &p_Value)
      {
        size_t l_Begin = 0;
        size_t l_End = p_Value.size();

        while (l_Begin < l_End &&
               std::isspace(
                   static_cast<unsigned char>(p_Value[l_Begin]))) {
          ++l_Begin;
        }
        while (l_End > l_Begin &&
               std::isspace(
                   static_cast<unsigned char>(p_Value[l_End - 1]))) {
          --l_End;
        }

        return p_Value.substr(l_Begin, l_End - l_Begin);
      }

      static bool
      parse_find_by_name_metadata(const Util::String &p_Metadata,
                                  Low::Util::Name &p_Name)
      {
        Util::String l_Metadata = trim_metadata_value(p_Metadata);
        const Util::String l_Prefix = "FindByName";

        if (l_Metadata.rfind(l_Prefix, 0) != 0) {
          return false;
        }

        const size_t l_Open = l_Metadata.find('(');
        const size_t l_Close = l_Metadata.rfind(')');
        if (l_Open == Util::String::npos ||
            l_Close == Util::String::npos || l_Close <= l_Open) {
          return false;
        }

        Util::String l_Value = trim_metadata_value(
            l_Metadata.substr(l_Open + 1, l_Close - l_Open - 1));

        if (l_Value.size() >= 2 &&
            ((l_Value.front() == '"' && l_Value.back() == '"') ||
             (l_Value.front() == '\'' && l_Value.back() == '\''))) {
          l_Value = l_Value.substr(1, l_Value.size() - 2);
        }

        if (l_Value.empty()) {
          return false;
        }

        p_Name = Low::Util::Name(l_Value.c_str());
        return true;
      }

      enum class MemberFieldFillType
      {
        FindByName
      };

      struct MemberFieldFillMetadata
      {
        MemberFieldFillType type;
        u32 propertyIndex;
        Low::Util::Name value;
      };

      static Low::Util::Map<u64,
                            Low::Util::List<MemberFieldFillMetadata>>
          g_MemberFieldFillMetadata;

      static void clear_member_field_metadata(ScriptClass p_Class)
      {
        g_MemberFieldFillMetadata.erase(p_Class.get_id());
      }

      static void add_find_by_name_metadata(ScriptClass p_Class,
                                            u32 p_PropertyIndex,
                                            Low::Util::Name p_Name)
      {
        MemberFieldFillMetadata l_Metadata;
        l_Metadata.type = MemberFieldFillType::FindByName;
        l_Metadata.propertyIndex = p_PropertyIndex;
        l_Metadata.value = p_Name;

        g_MemberFieldFillMetadata[p_Class.get_id()].push_back(
            l_Metadata);
      }

      static std::string
      get_member_type_namespace(asITypeInfo *p_PropertyType)
      {
        const char *l_TypeNamespace = p_PropertyType->GetNamespace();
        std::string l_Namespace =
            l_TypeNamespace ? l_TypeNamespace : "";
        if (!l_Namespace.empty()) {
          l_Namespace += "::";
        }
        l_Namespace += p_PropertyType->GetName();
        return l_Namespace;
      }

      static bool assign_member_field_handle(
          asIScriptObject *p_Object, asITypeInfo *p_Type,
          u32 p_PropertyIndex, Low::Util::Handle p_Handle)
      {
        void *l_PropertyAddress =
            p_Object->GetAddressOfProperty(p_PropertyIndex);
        if (!l_PropertyAddress) {
          return false;
        }

        *(Low::Util::Handle *)l_PropertyAddress = p_Handle;
        return true;
      }

      static bool
      get_member_field_low_type_id(asITypeInfo *p_PropertyType,
                                   u16 &p_TypeId)
      {
        asIScriptEngine *l_Engine = p_PropertyType->GetEngine();
        const std::string l_Namespace =
            get_member_type_namespace(p_PropertyType);

        l_Engine->SetDefaultNamespace(l_Namespace.c_str());
        asIScriptFunction *l_Function =
            l_Engine->GetGlobalFunctionByDecl("u16 get_TYPE_ID()");
        if (!l_Function) {
          l_Function = l_Engine->GetGlobalFunctionByDecl(
              "uint16 get_TYPE_ID()");
        }
        l_Engine->SetDefaultNamespace("");

        if (!l_Function) {
          return false;
        }

        asIScriptContext *l_Context = l_Engine->RequestContext();
        if (!l_Context) {
          return false;
        }

        if (l_Context->Prepare(l_Function) < 0) {
          l_Engine->ReturnContext(l_Context);
          return false;
        }

        const int l_Result = l_Context->Execute();
        if (l_Result != asEXECUTION_FINISHED) {
          l_Engine->ReturnContext(l_Context);
          return false;
        }

        p_TypeId = static_cast<u16>(l_Context->GetReturnWord());
        l_Engine->ReturnContext(l_Context);
        return true;
      }

      static bool fill_component_member_field_find_by_name(
          asIScriptObject *p_Object, asITypeInfo *p_Type,
          const MemberFieldFillMetadata &p_Metadata,
          Low::Util::RTTI::TypeInfo &p_PropertyTypeInfo)
      {
        const char *l_PropertyName = nullptr;
        p_Type->GetProperty(p_Metadata.propertyIndex,
                            &l_PropertyName);

        Low::Util::Handle l_Component = Low::Util::Handle::DEAD;
        if (p_PropertyTypeInfo.component) {
          Entity l_Entity = Entity::find_by_name(p_Metadata.value);
          if (l_Entity.is_alive()) {
            l_Component =
                l_Entity.get_component(p_PropertyTypeInfo.typeId);
          }
        } else if (p_PropertyTypeInfo.uiComponent) {
          UI::Element l_Element =
              UI::Element::find_by_name(p_Metadata.value);
          if (l_Element.is_alive()) {
            l_Component =
                l_Element.get_component(p_PropertyTypeInfo.typeId);
          }
        } else {
          return false;
        }

        if (!p_PropertyTypeInfo.is_alive(l_Component)) {
          LOW_LOG_WARN << "[FindByName] Could not find "
                       << p_PropertyTypeInfo.name << " component on "
                       << (p_PropertyTypeInfo.component
                               ? "Entity"
                               : "UI::Element")
                       << " named " << p_Metadata.value << " for "
                       << p_Type->GetName() << "."
                       << (l_PropertyName ? l_PropertyName
                                          : "<unknown>")
                       << LOW_LOG_END;
          return false;
        }

        return assign_member_field_handle(
            p_Object, p_Type, p_Metadata.propertyIndex, l_Component);
      }

      static bool fill_member_field_find_by_name(
          asIScriptObject *p_Object, asITypeInfo *p_Type,
          const MemberFieldFillMetadata &p_Metadata)
      {
        if (p_Metadata.propertyIndex >= p_Type->GetPropertyCount()) {
          return false;
        }

        const char *l_PropertyName = nullptr;
        int l_AngelScriptPropertyTypeId = 0;
        if (p_Type->GetProperty(p_Metadata.propertyIndex,
                                &l_PropertyName,
                                &l_AngelScriptPropertyTypeId) < 0) {
          return false;
        }

        asIScriptEngine *l_Engine = p_Type->GetEngine();
        asITypeInfo *l_PropertyType =
            l_Engine->GetTypeInfoById(l_AngelScriptPropertyTypeId);
        if (!l_PropertyType ||
            l_PropertyType->GetSize() != sizeof(Low::Util::Handle)) {
          LOW_LOG_WARN << "[FindByName] " << p_Type->GetName() << "."
                       << (l_PropertyName ? l_PropertyName
                                          : "<unknown>")
                       << " is not a handle value type."
                       << LOW_LOG_END;
          return false;
        }

        u16 l_PropertyTypeId = 0;
        if (get_member_field_low_type_id(l_PropertyType,
                                         l_PropertyTypeId) &&
            Low::Util::Handle::is_registered_type(l_PropertyTypeId)) {
          Low::Util::RTTI::TypeInfo &l_PropertyTypeInfo =
              Low::Util::Handle::get_type_info(l_PropertyTypeId);

          if (l_PropertyTypeInfo.component ||
              l_PropertyTypeInfo.uiComponent) {
            return fill_component_member_field_find_by_name(
                p_Object, p_Type, p_Metadata, l_PropertyTypeInfo);
          }
        }

        const std::string l_Namespace =
            get_member_type_namespace(l_PropertyType);
        const std::string l_Declaration =
            std::string(l_PropertyType->GetName()) +
            " find_by_name(Name)";

        l_Engine->SetDefaultNamespace(l_Namespace.c_str());
        asIScriptFunction *l_Function =
            l_Engine->GetGlobalFunctionByDecl(l_Declaration.c_str());
        l_Engine->SetDefaultNamespace("");

        if (!l_Function) {
          LOW_LOG_WARN << "[FindByName] " << p_Type->GetName() << "."
                       << (l_PropertyName ? l_PropertyName
                                          : "<unknown>")
                       << " type has no find_by_name(Name) function."
                       << LOW_LOG_END;
          return false;
        }

        asIScriptContext *l_Context = l_Engine->RequestContext();
        if (!l_Context) {
          return false;
        }

        if (l_Context->Prepare(l_Function) < 0 ||
            l_Context->SetArgObject(0, (void *)&p_Metadata.value) <
                0) {
          l_Engine->ReturnContext(l_Context);
          return false;
        }

        const int l_Result = l_Context->Execute();
        if (l_Result != asEXECUTION_FINISHED) {
          l_Engine->ReturnContext(l_Context);
          LOW_LOG_WARN << "[FindByName] Failed to resolve "
                       << p_Type->GetName() << "."
                       << (l_PropertyName ? l_PropertyName
                                          : "<unknown>")
                       << LOW_LOG_END;
          return false;
        }

        void *l_ReturnObject = l_Context->GetReturnObject();
        if (!l_ReturnObject) {
          l_Engine->ReturnContext(l_Context);
          return false;
        }

        Low::Util::Handle l_Handle =
            *(Low::Util::Handle *)l_ReturnObject;

        l_Engine->ReturnContext(l_Context);

        if (!l_Handle.is_registered_type() ||
            !Low::Util::Handle::get_type_info(l_Handle.get_type())
                 .is_alive(l_Handle)) {
          LOW_LOG_WARN << "[FindByName] Could not find "
                       << l_PropertyType->GetName() << " named "
                       << p_Metadata.value << " for "
                       << p_Type->GetName() << "."
                       << (l_PropertyName ? l_PropertyName
                                          : "<unknown>")
                       << LOW_LOG_END;
          return false;
        }

        return assign_member_field_handle(
            p_Object, p_Type, p_Metadata.propertyIndex, l_Handle);
      }

      bool fill_member_fields(ClassInstance p_Instance)
      {
        if (!p_Instance.is_alive()) {
          return false;
        }

        ScriptClass l_Class = p_Instance.get_script_class();
        auto l_MetadataIt =
            g_MemberFieldFillMetadata.find(l_Class.get_id());
        if (l_MetadataIt == g_MemberFieldFillMetadata.end()) {
          return true;
        }

        asIScriptObject *l_Object =
            (asIScriptObject *)p_Instance.get_ptr();
        asITypeInfo *l_Type = (asITypeInfo *)l_Class.as_class();
        if (!l_Object || !l_Type) {
          return false;
        }

        bool l_Success = true;
        for (const MemberFieldFillMetadata &i_Metadata :
             l_MetadataIt->second) {
          switch (i_Metadata.type) {
          case MemberFieldFillType::FindByName:
            l_Success = fill_member_field_find_by_name(
                            l_Object, l_Type, i_Metadata) &&
                        l_Success;
            break;
          default:
            LOW_ASSERT(false,
                       "Unsupported script member fill metadata");
            break;
          }
        }

        return l_Success;
      }

      static void
      collect_member_field_metadata(ScriptClass p_Class,
                                    asITypeInfo *p_Type,
                                    CScriptBuilder &p_Builder)
      {
        clear_member_field_metadata(p_Class);

        const asUINT l_PropertyCount = p_Type->GetPropertyCount();
        for (asUINT i = 0; i < l_PropertyCount; ++i) {
          auto l_Metadata = p_Builder.GetMetadataForTypeProperty(
              p_Type->GetTypeId(), i);

          for (const auto &i_Metadata : l_Metadata) {
            Low::Util::Name i_Name;
            if (parse_find_by_name_metadata(
                    Util::String(i_Metadata.c_str()), i_Name)) {
              add_find_by_name_metadata(p_Class, i, i_Name);
            }
          }
        }
      }

      static bool is_ticking_signature(asIScriptEngine *p_Engine,
                                       asIScriptFunction *p_Function)
      {
        if (p_Function->GetParamCount() != 0) {
          return false;
        }

        if (p_Function->GetReturnTypeId() !=
            p_Engine->GetTypeIdByDecl("void")) {
          return false;
        }

        return true;
      }

      static void handle_class(ScriptClass p_Class)
      {
        asITypeInfo *l_UiControllerInterface =
            g_Engine->GetTypeInfoByName("UiController");
        asITypeInfo *l_GameplaySystemInterface =
            g_Engine->GetTypeInfoByName("GameplaySystem");

        asITypeInfo *l_Type = (asITypeInfo *)p_Class.as_class();

        if (l_Type->Implements(l_UiControllerInterface)) {
          UI::Controller l_Controller =
              UI::Controller::find_by_scriptclass(p_Class);
          if (!l_Controller.is_alive()) {
            l_Controller = UI::Controller::make_script(
                p_Class.get_name(), p_Class);
          }
          l_Controller.update_instances();
        }
        if (l_Type->Implements(l_GameplaySystemInterface)) {
          GameplaySystem l_System =
              GameplaySystem::find_by_scriptclass(p_Class);
          if (!l_System.is_alive()) {
            l_System = GameplaySystem::make_script(p_Class.get_name(),
                                                   p_Class);
          }
          l_System.update_instances();
        }
      }

      static void fill_classes(Module p_Module,
                               CScriptBuilder &p_Builder)
      {
        asIScriptModule *l_Module =
            (asIScriptModule *)p_Module.get_as_module();

        const asUINT l_Count = l_Module->GetObjectTypeCount();
        for (asUINT i = 0; i < l_Count; ++i) {
          asITypeInfo *i_Type = l_Module->GetObjectTypeByIndex(i);
          if (!i_Type) {
            continue;
          }
          const asQWORD i_Flags = i_Type->GetFlags();

          const bool i_IsScriptClass =
              (i_Flags & asOBJ_SCRIPT_OBJECT) != 0 &&
              i_Type->GetSize() > 0;

          if (!i_IsScriptClass) {
            continue;
          }

          ScriptClass i_Class = p_Module.find_class_by_name(
              LOW_NAME(i_Type->GetName()));
          if (!i_Class.is_alive()) {
            i_Class = ScriptClass::make(LOW_NAME(i_Type->GetName()));
            i_Class.set_module(p_Module);
            p_Module.get_classes().push_back(i_Class.get_id());
          }

          i_Class.set_as_class((char *)i_Type);
          i_Class.set_reload_index(p_Module.get_reload_index());

          collect_member_field_metadata(i_Class, i_Type, p_Builder);
          handle_class(i_Class);
        }

        // Remove 'dead' classes
        for (auto it = p_Module.get_classes().begin();
             it != p_Module.get_classes().end();) {
          ScriptClass i_Class = *it;
          if (!i_Class.is_alive() || i_Class.needs_refresh()) {
            it = p_Module.get_classes().erase(it);
          } else {
            it++;
          }
        }
      }

      static void fill_ticking_functions(Module p_Module,
                                         CScriptBuilder &p_Builder)
      {
        p_Module.get_ticking_functions().clear();

        asIScriptModule *l_Module =
            (asIScriptModule *)p_Module.get_as_module();

        const asUINT l_Count = l_Module->GetFunctionCount();
        for (asUINT i = 0; i < l_Count; ++i) {
          asIScriptFunction *l_Function =
              l_Module->GetFunctionByIndex(i);
          if (!l_Function) {
            continue;
          }

          if (!has_metadata(p_Builder, l_Function, "Ticking")) {
            continue;
          }

          if (!is_ticking_signature(g_Engine, l_Function)) {
            LOW_LOG_WARN
                << "Ignoring [Ticking] function with wrong "
                   "signature: "
                << l_Function->GetDeclaration(true, true, true)
                << LOW_LOG_END;
            continue;
          }

          l_Function->AddRef();
          p_Module.get_ticking_functions().push_back(
              (char *)l_Function);
        }
      }

      void build_module(Module p_Module)
      {
        LOW_LOG_DEBUG << "Building module " << p_Module.get_name()
                      << LOW_LOG_END;

        CScriptBuilder l_Builder;

        int l_Result = l_Builder.StartNewModule(
            g_Engine, p_Module.get_name().c_str());
        LOW_ASSERT(l_Result >= 0,
                   "Failed to start AngelScript module build.");

        for (auto it = p_Module.get_scripts().begin();
             it != p_Module.get_scripts().end();) {
          ScriptAsset i_Script = *it;

          if (!i_Script.is_alive()) {
            it = p_Module.get_scripts().erase(it);
            continue;
          }

          if (!Util::FileIO::file_exists_sync(
                  i_Script.get_full_path().c_str())) {
            i_Script.destroy();
            it = p_Module.get_scripts().erase(it);
            continue;
          }

          Util::String i_SourceCode;
          LOW_ASSERT(
              load_script_to_file(i_Script.get_full_path().c_str(),
                                  i_SourceCode),
              "Failed to load AngelScript source code.");

          l_Result = l_Builder.AddSectionFromMemory(
              i_Script.get_full_path().c_str(), i_SourceCode.c_str(),
              static_cast<unsigned int>(i_SourceCode.size()));
          LOW_ASSERT(
              l_Result >= 0,
              "Failed to add script section to CScriptBuilder.");

          ++it;
        }

        Core::EventManager::dispatch_event(
            N(LOW_SCRIPTING_COMPILE),
            Util::Variant::from_handle(p_Module));

        l_Result = l_Builder.BuildModule();
        LOW_ASSERT_ERROR_RETURN(
            l_Result >= 0, "Failed to build AngelScript module.");

        asIScriptModule *l_AsModule = l_Builder.GetModule();

        LOW_ASSERT(l_AsModule,
                   "Failed to get built AngelScript module.");

        p_Module.set_as_module((char *)l_AsModule);

        p_Module.set_reload_index(p_Module.get_reload_index() + 1);

        LOW_LOG_INFO << "Created AngelScript module '"
                     << p_Module.get_name() << "'." << LOW_LOG_END;

        fill_ticking_functions(p_Module, l_Builder);
        fill_classes(p_Module, l_Builder);
      }

      static void message_callback(const asSMessageInfo *p_Message,
                                   void *p_Param)
      {
        EventMessage l_Message;
        l_Message.type = EventType::Error;
        if (p_Message->type == asMSGTYPE_INFORMATION) {
          l_Message.type = EventType::Info;
        } else if (p_Message->type == asMSGTYPE_WARNING) {
          l_Message.type = EventType::Warn;
        }
        l_Message.col = p_Message->col;
        l_Message.row = p_Message->row;

        l_Message.msg = p_Message->message;
        l_Message.msg.trim();

        if (l_Message.msg.empty()) {
          return;
        }

        l_Message.script = Util::Handle::DEAD;

        const Util::String l_SectionString = p_Message->section;

        for (u32 i = 0; i < ScriptAsset::living_count(); ++i) {
          ScriptAsset i_Script = ScriptAsset::living_instances()[i];

          if (i_Script.get_full_path() == l_SectionString) {
            l_Message.script = i_Script;
            break;
          }
        }

        Core::EventManager::dispatch_event(
            N(LOW_SCRIPTING_MESSAGE), (u32)l_Message.type,
            l_Message.row, l_Message.col,
            Util::Variant(l_Message.msg),
            Util::Variant::from_handle(l_Message.script));

        if (l_Message.type == EventType::Error) {
          LOW_LOG_ERROR << p_Message->message << LOW_LOG_END;
        } else if (l_Message.type == EventType::Warn) {
          LOW_LOG_WARN << p_Message->message << LOW_LOG_END;
        } else {
          LOW_LOG_INFO << p_Message->message << LOW_LOG_END;
        }
      }

      static bool init_modules()
      {

        for (u32 i = 0; i < Module::living_count(); ++i) {
          Module i_Module = Module::living_instances()[i];

          build_module(i_Module);
        }

        return true;
      }

      void initialize_as()
      {
        g_Engine = asCreateScriptEngine();
        LOW_ASSERT(g_Engine, "Failed to create AngelScript engine");

        int l_Result = g_Engine->SetMessageCallback(
            asFUNCTION(message_callback), nullptr, asCALL_CDECL);

        LOW_ASSERT(l_Result >= 0,
                   "Failed to set AngelScript message callback");

        expose(g_Engine);
        register_types(g_Engine);
        expose_types(g_Engine);
        register_interfaces(g_Engine);

        LOW_ASSERT(init_modules(),
                   "Failed to initialize AngelScript modules.");

        g_TickContext = g_Engine->CreateContext();

        g_Initialized = true;
      }

      void test_as()
      {
        asIScriptEngine *engine = g_Engine;
        asIScriptModule *mod =
            (asIScriptModule *)Module::find_by_name(N(low.misc))
                .get_as_module();

        asIScriptFunction *func =
            mod->GetFunctionByDecl("void test()");
        if (!func) {
          std::cout << "GetFunctionByDecl failed\n";
          engine->ShutDownAndRelease();
          return;
        }

        asIScriptContext *ctx = engine->CreateContext();
        if (!ctx) {
          std::cout << "CreateContext failed\n";
          engine->ShutDownAndRelease();
          return;
        }

        int r = ctx->Prepare(func);
        if (r < 0) {
          std::cout << "Prepare failed\n";
          ctx->Release();
          engine->ShutDownAndRelease();
          return;
        }

        r = ctx->Execute();
        if (r != asEXECUTION_FINISHED) {
          std::cout << "Execute failed, code = " << r << "\n";
          if (r == asEXECUTION_EXCEPTION) {
            std::cout << "Exception: " << ctx->GetExceptionString()
                      << "\n";
          }
          ctx->Release();
          engine->ShutDownAndRelease();
          return;
        }
      }

      void cleanup_as()
      {
        if (g_TickContext) {
          g_TickContext->Release();
          g_TickContext = nullptr;
        }
        if (g_Engine) {
          g_Engine->ShutDownAndRelease();
          g_Engine = nullptr;
        }

        g_Initialized = false;
      }

      static void call_ticking_functions(Module p_Module)
      {
        for (auto i_Ptr : p_Module.get_ticking_functions()) {
          asIScriptFunction *i_Function = (asIScriptFunction *)i_Ptr;

          int r = g_TickContext->Prepare(i_Function);
          LOW_ASSERT(r >= 0, "Prepare failed");

          r = g_TickContext->Execute();
          LOW_ASSERT(r == asEXECUTION_FINISHED,
                     "Ticking function execution failed");
        }
      }

      void tick_as(const float p_Delta)
      {
        if (!g_Initialized) {
          return;
        }

        for (u32 i = 0; i < Module::living_count(); ++i) {
          Module i_Module = Module::living_instances()[i];
          if (!i_Module.get_as_module()) {
            continue;
          }
          call_ticking_functions(i_Module);
        }
      }
    } // namespace Scripting
  } // namespace Core
} // namespace Low
