#pragma once

#include "LowCoreScriptAsset.h"
#include "LowEditorVisualScriptBuilder.h"
#include "LowEditorVisualScriptEditor.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      struct LOW_EDITOR_API ScriptAssetBuilder
      {
        Document document;
        CompileProfileRegistry *compile_profile_registry = nullptr;
        GraphBuilder graph_builder;
        Core::ScriptAsset script_asset;
        Util::String script_asset_sidecar_path;

        ScriptAssetBuilder();
        ScriptAssetBuilder(const ContextDefinition &p_Context);

        void apply_context(const ContextDefinition &p_Context);

        Document &get_document()
        {
          return document;
        }

        const Document &get_document() const
        {
          return document;
        }

        GraphBuilder &get_graph_builder()
        {
          return graph_builder;
        }

        const GraphBuilder &get_graph_builder() const
        {
          return graph_builder;
        }

        CompileProfileRegistry &get_compile_profile_registry()
        {
          _LOW_ASSERT(compile_profile_registry);
          return *compile_profile_registry;
        }

        const CompileProfileRegistry &
        get_compile_profile_registry() const
        {
          _LOW_ASSERT(compile_profile_registry);
          return *compile_profile_registry;
        }

        template <typename T> T *get_compile_settings()
        {
          return document.get_compile_settings<T>();
        }

        template <typename T> const T *get_compile_settings() const
        {
          return document.get_compile_settings<T>();
        }

        bool create_script_asset(Util::Name p_Name,
                                 Core::Scripting::Module p_Module,
                                 const Util::String &p_SourcePath);
        bool save_script_asset_sidecar();
        bool save_compile_and_build_module(
            const Util::String &p_VisualScriptPath);
      };
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
