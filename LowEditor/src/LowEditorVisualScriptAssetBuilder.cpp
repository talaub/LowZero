#include "LowEditorVisualScriptAssetBuilder.h"

#include "LowCoreScripting.h"
#include "LowCoreScriptAssetGenerator.h"
#include "LowUtil.h"
#include "LowUtilSerialization.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace {
        static Util::String normalize_script_source_path(
            const Util::String &p_Path)
        {
          Util::String l_NormalizedPath =
              Util::PathHelper::normalize(p_Path);
          Util::String l_NormalizedDataPath =
              Util::PathHelper::normalize(Util::get_project().dataPath);

          while (Util::StringHelper::begins_with(l_NormalizedPath,
                                                 "./")) {
            l_NormalizedPath = l_NormalizedPath.substr(2);
          }
          while (Util::StringHelper::begins_with(
              l_NormalizedDataPath, "./")) {
            l_NormalizedDataPath =
                l_NormalizedDataPath.substr(2);
          }

          if (!l_NormalizedDataPath.empty() &&
              Util::StringHelper::begins_with(
                  l_NormalizedPath, l_NormalizedDataPath + "/")) {
            return l_NormalizedPath.substr(
                l_NormalizedDataPath.size() + 1);
          }

          return Util::PathHelper::normalize(p_Path);
        }
      } // namespace

      ScriptAssetBuilder::ScriptAssetBuilder()
          : graph_builder(document.graph, &document.schema)
      {
        compile_profile_registry =
            &VisualScript::get_compile_profile_registry();
      }

      ScriptAssetBuilder::ScriptAssetBuilder(
          const ContextDefinition &p_Context)
          : ScriptAssetBuilder()
      {
        apply_context(p_Context);
      }

      void ScriptAssetBuilder::apply_context(
          const ContextDefinition &p_Context)
      {
        _LOW_ASSERT(compile_profile_registry);
        document.apply_context(p_Context, *compile_profile_registry);
        graph_builder.set_graph(document.graph)
            .set_schema(&document.schema);
      }

      bool ScriptAssetBuilder::create_script_asset(
          Util::Name p_Name, Core::Scripting::Module p_Module,
          const Util::String &p_SourcePath)
      {
        if (!p_Name.is_valid() || !p_Module.is_alive() ||
            p_SourcePath.empty()) {
          return false;
        }

        script_asset = Core::ScriptAsset::make(p_Name);
        if (!script_asset.is_alive()) {
          return false;
        }

        script_asset.set_generator(
            Core::Scripting::AssetGenerator::VISUALSCRIPT);
        script_asset.set_module(p_Module);
        script_asset.set_source_path(
            normalize_script_source_path(p_SourcePath));

        script_asset_sidecar_path = Util::get_project().assetCachePath;
        script_asset_sidecar_path += "/";
        script_asset_sidecar_path +=
            Util::hash_to_string(script_asset.get_unique_id());
        script_asset_sidecar_path += ".script.yaml";

        return true;
      }

      bool ScriptAssetBuilder::save_script_asset_sidecar()
      {
        if (!script_asset.is_alive() ||
            script_asset_sidecar_path.empty()) {
          return false;
        }

        Util::Serial::Node l_OutNode;
        script_asset.serialize(l_OutNode);
        Util::Serial::write_yaml_file(
            script_asset_sidecar_path.c_str(), l_OutNode);
        return true;
      }

      bool ScriptAssetBuilder::save_compile_and_build_module(
          const Util::String &p_VisualScriptPath)
      {
        if (!document.save_as(p_VisualScriptPath)) {
          return false;
        }

        if (script_asset.is_alive() && !save_script_asset_sidecar()) {
          return false;
        }

        _LOW_ASSERT(compile_profile_registry);
        if (!document.compile_and_write(*compile_profile_registry)) {
          return false;
        }

        if (script_asset.is_alive()) {
          Core::Scripting::build_module(script_asset.get_module());
        }

        return true;
      }
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
