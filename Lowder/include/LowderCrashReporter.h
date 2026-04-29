#pragma once

#include <string>

namespace Lowder {
  namespace CrashReporter {
    void initialize(const std::string &p_ProjectPath);
    void shutdown();
  } // namespace CrashReporter
} // namespace Lowder
