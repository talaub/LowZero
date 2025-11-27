#include "LowEditorSaveHelper.h"

#include "LowUtil.h"
#include "LowUtilLogger.h"
#include "LowUtilHashing.h"

namespace Low {
  namespace Editor {
    namespace SaveHelper {
      void save_region(Core::Region p_Region, bool p_ShowMessage)
      {
        {
          Util::Serial::Node l_Node;
          p_Region.serialize(l_Node);
          Util::String l_Path =
              Util::get_project().dataPath + "/assets/regions/" +
              Util::hash_to_string(p_Region.get_unique_id()) +
              ".region.yaml";
          Util::Serial::write_yaml_file(l_Path.c_str(), l_Node);
        }

        if (p_Region.is_loaded()) {
          Util::Serial::Node l_Node;
          p_Region.serialize_entities(l_Node);
          Util::String l_Path =
              Util::get_project().dataPath + "/assets/regions/" +
              Util::hash_to_string(p_Region.get_unique_id()) +
              ".entities.yaml";
          Util::Serial::write_yaml_file(l_Path.c_str(), l_Node);
        }

        if (p_ShowMessage) {
          LOW_LOG_INFO << "Saved region '" << p_Region.get_name()
                       << "' to file." << LOW_LOG_END;
        }
      }

      void save_scene(Core::Scene p_Scene, bool p_ShowMessage)
      {
        Util::Serial::Node l_Node;
        p_Scene.serialize(l_Node);
        Util::String l_Path =
            Util::get_project().dataPath + "/assets/scenes/" +
            Util::hash_to_string(p_Scene.get_unique_id()) +
            ".scene.yaml";
        Util::Serial::write_yaml_file(l_Path.c_str(), l_Node);

        for (auto it = p_Scene.get_regions().begin();
             it != p_Scene.get_regions().end(); ++it) {
          Core::Region i_Region =
              Util::find_handle_by_unique_id(*it).get_id();

          save_region(i_Region, false);
        }

        if (p_ShowMessage) {
          LOW_LOG_INFO << "Saved scene '" << p_Scene.get_name()
                       << "' to file." << LOW_LOG_END;
        }
      }
    } // namespace SaveHelper
  } // namespace Editor
} // namespace Low
