#include "LowCliCommands.h"

#include "LowCliOutput.h"

#include <chrono>
#include <thread>

namespace Low {
  namespace Cli {
    namespace Commands {
      static void sleep_ms(int p_Milliseconds)
      {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(p_Milliseconds));
      }

      // Shows every output element, useful when working on the renderer
      static int run_output_demo(Context &)
      {
        Output::section("Output demo");
        Output::info("Plain info line");
        Output::detail("A detail line");
        Output::debug("A debug line (only with -v)");
        Output::success("Something worked");

        {
          Output::Step l_Step("Doing something that takes a while");
          sleep_ms(1500);
        }

        Output::task_begin("progress", "Processing items", 40);
        for (int i = 0; i <= 40; ++i) {
          Output::task_update("progress", i,
                              i % 2 == 0 ? "Mesh.yaml" : "Material.yaml");
          sleep_ms(50);
        }
        Output::task_end("progress");

        Output::task_begin("a", "Two tasks at once");
        Output::task_begin("b", "Second task", 10);
        for (int i = 0; i <= 10; ++i) {
          Output::task_update("b", i);
          sleep_ms(120);
        }
        Output::task_end("b");
        Output::task_end("a");

        {
          Output::Step l_Step("A step that fails");
          sleep_ms(800);
          l_Step.fail();
        }

        Output::warn("Something looks odd",
                     {"first detail line", "second detail line"});
        Output::error("Something went wrong",
                      {"modules/Gameplay/src/Foo.cpp:12: expected ';'"},
                      "run with -v for the full output");

        Output::result("this line goes to stdout");
        return 1;
      }

      void register_debug(CLI::App &p_App)
      {
        CLI::App *l_Debug =
            p_App.add_subcommand("debug", "Tools for working on low");
        // Empty group hides it from --help
        l_Debug->group("");
        l_Debug->require_subcommand(1);

        CLI::App *l_Output =
            l_Debug->add_subcommand("output", "Show all output elements");
        add_command(l_Output, run_output_demo);
      }
    } // namespace Commands
  } // namespace Cli
} // namespace Low
