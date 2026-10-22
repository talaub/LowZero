#pragma once

#include "LowUtil.h"
#include "LowUtilContainers.h"
#include <filesystem>

#include <CLI/CLI.hpp>

namespace Low {
  namespace Cli {
    struct Context
    {
      Util::String engineRoot;
      Util::String projectRoot;
      Util::EngineSource engineSource = Util::EngineSource::Unknown;

      bool require_engine_root() const;
      bool require_project_root() const;
    };

    Util::String get_current_directory();

    /** Warns if the project is associated with a different engine
    than the one this low belongs to, e.g. when <engine>/bin/low.exe
    is called directly instead of through the shim*/
    void warn_on_engine_mismatch(const Context &p_Context);

    void add_command(CLI::App *p_App,
                     Util::Function<int(Context &)> p_Run);

    int run_selected_command(Context &p_Context);

    /** Our strings are UTF-8, a plain char path would use the ANSI
    codepage on Windows */
    std::filesystem::path to_path(Util::StringView p_Path);
    Util::String from_path(const std::filesystem::path &p_Path);

    /** Raw scalar text so ids like 2026.6 stay strings. Empty if the
     file or key is missing.*/
    Util::String read_yaml_scalar(const Util::String &p_File,
                                  const char *p_Key);

    int run_project_configure(
        Context &p_Context,
        const Util::List<Util::String> &p_ExtraArguments);

    namespace Commands {
      void register_all(CLI::App &p_App);

      void register_info(CLI::App &p_App);
      void register_debug(CLI::App &p_App);
      void register_configure(CLI::App &p_App);
      void register_engine(CLI::App &p_App);
      void register_package(CLI::App &p_App);
      void register_config(CLI::App &p_App);
      void register_release(CLI::App &p_App);
    } // namespace Commands
  } // namespace Cli
} // namespace Low
