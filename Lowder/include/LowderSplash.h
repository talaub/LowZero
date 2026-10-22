#pragma once

#include <string>

namespace Lowder {
  namespace Splash {
    // Call before start(), shows the SOURCE BUILD badge
    void set_source_build(bool p_SourceBuild);
    // Call before start(), folder with the logo images
    // (<engine>/LowData/editor/images)
    void set_image_directory(const std::string &p_Directory);
    void start();
    void set_status(const std::string &p_Status);
    void stop();
  } // namespace Splash
} // namespace Lowder
