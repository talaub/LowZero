#include "LowUtilTest.h"

#include <iostream>

#include <yaml-cpp/yaml.h>

#include "LowUtilLogger.h"

namespace Low {
  namespace Util {
    void test()
    {
      YAML::Node config =
          YAML::LoadFile("D:"
                         "\\projects\\engine\\engine\\LowEngine\\LowEngine\\low"
                         "\\documentation\\docs\\components\\camera.yaml");

      const std::string desc = config["description"].as<std::string>();

      LOW_LOG_DEBUG(desc.c_str());
    }
  } // namespace Util
} // namespace Low
