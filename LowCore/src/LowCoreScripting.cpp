#include "LowCoreScripting.h"

#include "LowCoreScriptAsset.h"
#include "LowCoreScriptModule.h"
#include "LowUtilAssert.h"
#include "LowUtilFileIO.h"

#include <angelscript.h>

#include <iostream>

namespace Low {
  namespace Core {
    namespace Scripting {
      static asIScriptEngine *g_Engine = nullptr;

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
        const u32 l_ReadBytes =
            Low::Util::FileIO::read_sync(l_File, p_Source.data());

        Low::Util::FileIO::close(l_File);

        LOW_ASSERT_ERROR_RETURN_FALSE(
            l_ReadBytes == l_Size,
            "Failed to read complete script file");

        return true;
      }

      void build_module(Module p_Module)
      {
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
                  i_Script.get_source_path().c_str())) {
            i_Script.destroy();
            it = p_Module.get_scripts().erase(it);
            continue;
          }

          Util::String i_SourceCode;
          LOW_ASSERT(
              load_script_to_file(i_Script.get_source_path().c_str(),
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
      }

      static int Add(int a, int b)
      {
        return a + b;
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

        LOW_ASSERT(init_modules(),
                   "Failed to initialize AngelScript modules.");

#if 0
        {
          asIScriptEngine *engine = g_Engine;
          int r = 0;

          // Register a single global function
          r = engine->RegisterGlobalFunction(
              "int Add(int, int)", asFUNCTION(Add), asCALL_CDECL);
          if (r < 0) {
            std::cout << "RegisterGlobalFunction failed\n";
            engine->ShutDownAndRelease();
            return;
          }

          // Build a tiny script from a string
          const char *script = "int Test() {          \n"
                               "  return Add(20, 22); \n"
                               "}                     \n";

          asIScriptModule *mod =
              engine->GetModule("TestModule", asGM_ALWAYS_CREATE);
          r = mod->AddScriptSection("inline", script);
          if (r < 0) {
            std::cout << "AddScriptSection failed\n";
            engine->ShutDownAndRelease();
            return;
          }

          r = mod->Build();
          if (r < 0) {
            std::cout << "Build failed\n";
            engine->ShutDownAndRelease();
            return;
          }

          asIScriptFunction *func =
              mod->GetFunctionByDecl("int Test()");
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

          r = ctx->Prepare(func);
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

          int result = (int)ctx->GetReturnDWord();
          std::cout << "AngelScript test result = " << result
                    << std::endl;
        }
#endif
      }

      void cleanup_as()
      {
        if (g_Engine) {
          g_Engine->ShutDownAndRelease();
          g_Engine = nullptr;
        }
      }
    } // namespace Scripting
  } // namespace Core
} // namespace Low
