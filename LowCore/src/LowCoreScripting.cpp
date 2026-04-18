#include "LowCoreScripting.h"

#include "LowCoreScriptAsset.h"
#include "LowCoreScriptModule.h"
#include "LowCoreScriptClass.h"
#include "LowCoreUiController.h"
#include "LowUtilAssert.h"
#include "LowUtilFileIO.h"
#include "LowUtilLogger.h"
#include "angelscript.h"

#include <angelscript.h>
#include <scriptbuilder/scriptbuilder.h>

#include <iostream>

namespace Low {
  namespace Core {
    void expose_types(asIScriptEngine *p_Engine);
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

      /*
      void build_module(Module p_Module)
      {
        LOW_LOG_DEBUG << "Building module " << p_Module.get_name()
                      << LOW_LOG_END;

        if (!p_Module.get_as_module()) {
          p_Module.set_as_module((char *)g_Engine->GetModule(
              p_Module.get_name().c_str(), asGM_ALWAYS_CREATE));
        }

        asIScriptModule *l_AsModule =
            (asIScriptModule *)p_Module.get_as_module();

        LOW_ASSERT(l_AsModule,
                   "Failed to create AngelScript module.");

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

          int i_Result = l_AsModule->AddScriptSection(
              "main", i_SourceCode.c_str(),
              static_cast<unsigned int>(i_SourceCode.size()));
          LOW_ASSERT(i_Result >= 0, "Failed to add script section.");
          ++it;
        }

        int l_Result = l_AsModule->Build();
        LOW_ASSERT(l_Result >= 0,
                   "Failed to build AngelScript module.");

        LOW_LOG_INFO << "Created AngelScript module '"
                     << p_Module.get_name() << "'." << LOW_LOG_END;

        p_Module.set_as_module((char *)l_AsModule);

        fill_ticking_functions(p_Module);
      }
      */
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

        l_Result = l_Builder.BuildModule();
        LOW_ASSERT(l_Result >= 0,
                   "Failed to build AngelScript module.");

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
        const char *l_Type = "ERR";
        if (p_Message->type == asMSGTYPE_WARNING) {
          l_Type = "WARN";
        } else if (p_Message->type == asMSGTYPE_INFORMATION) {
          l_Type = "INFO";
        }

        LOW_LOG_INFO << "[AngelScript][" << l_Type << "] "
                     << p_Message->section << " (" << p_Message->row
                     << ", " << p_Message->col
                     << "): " << p_Message->message << LOW_LOG_END;
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
