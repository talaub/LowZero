#pragma once

#include <string>

namespace Lowder {
  namespace CrashReporter {
    // p_ImageDirectory: folder with the logo images
    // (<engine>/LowData/editor/images), may be empty
    void initialize(const std::string &p_ProjectPath,
                    const std::string &p_ImageDirectory);
    // Call once the project and its modules are loaded. Kept as copies,
    // while crashing nothing of the engine is touched anymore
    void set_project_info(const std::string &p_Name,
                          const std::string &p_Version,
                          const std::string &p_BuildId);
    // From crash_reporting: in project.yaml. Without them reports are
    // only saved locally
    void set_upload_target(const std::string &p_Url,
                           const std::string &p_Token);
    void shutdown();
  } // namespace CrashReporter
} // namespace Lowder
