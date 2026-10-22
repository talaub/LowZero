#include "LowCliCommands.h"

#include "LowCliOutput.h"
#include "LowCliPackaging.h"
#include "LowCliProcess.h"

#include "LowMath.h"

#include <ctype.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

// low release project major|minor|patch
//
//   1. checks: modules + project.yaml committed, tag free, Git LFS set up
//   2. bumps version: in project.yaml
//   3. packages with the tag as build id (the tag only exists after the
//      commit, but the commit contains exactly what was built)
//   4. commits the bump and the binaries, tags the commit
//
// Nothing is pushed.

namespace Low {
  namespace Cli {
    namespace Commands {
      namespace fs = std::filesystem;

      // CLI11 writes the arguments here during parse(), so this has to
      // outlive register_release
      static struct ReleaseProjectArgs
      {
        std::string part;
        bool allowDirty = false;
        bool withoutLfs = false;
        std::string symbolStore;
      } g_ProjectArgs;

      struct Version
      {
        u32 major = 0;
        u32 minor = 0;
        u32 patch = 0;
      };

      // Plain MAJOR.MINOR.PATCH, nothing else
      static bool parse_version(const Util::String &p_Text,
                                Version &p_OutVersion)
      {
        u32 l_Parts[3] = {0, 0, 0};
        u32 l_Part = 0;
        bool l_HasDigit = false;
        for (const char i_Char : p_Text) {
          if (i_Char == '.') {
            if (!l_HasDigit || l_Part == 2) {
              return false;
            }
            ++l_Part;
            l_HasDigit = false;
            continue;
          }
          if (!isdigit(static_cast<unsigned char>(i_Char))) {
            return false;
          }
          l_Parts[l_Part] = l_Parts[l_Part] * 10 + (i_Char - '0');
          l_HasDigit = true;
        }
        if (l_Part != 2 || !l_HasDigit) {
          return false;
        }

        p_OutVersion.major = l_Parts[0];
        p_OutVersion.minor = l_Parts[1];
        p_OutVersion.patch = l_Parts[2];
        return true;
      }

      static Util::String to_string(const Version &p_Version)
      {
        const std::string l_Text = std::to_string(p_Version.major) + "." +
                                   std::to_string(p_Version.minor) + "." +
                                   std::to_string(p_Version.patch);
        return l_Text.c_str();
      }

      static Version bump(Version p_Version, const std::string &p_Part)
      {
        if (p_Part == "major") {
          ++p_Version.major;
          p_Version.minor = 0;
          p_Version.patch = 0;
        } else if (p_Part == "minor") {
          ++p_Version.minor;
          p_Version.patch = 0;
        } else {
          ++p_Version.patch;
        }
        return p_Version;
      }

      static Util::String to_lower(Util::String p_Text)
      {
        for (char &i_Char : p_Text) {
          i_Char = static_cast<char>(
              tolower(static_cast<unsigned char>(i_Char)));
        }
        return p_Text;
      }

      // Exit code, -1 if git could not be started. stdout is read on
      // the calling thread, so collecting into the list is safe.
      static int run_git(const Util::String &p_Repository,
                         const Util::List<Util::String> &p_Arguments,
                         Util::List<Util::String> *p_OutLines = nullptr)
      {
        Process::Options l_Options;
        l_Options.executable = "git";
        l_Options.arguments = {"-C", p_Repository};
        for (const Util::String &i_Argument : p_Arguments) {
          l_Options.arguments.push_back(i_Argument);
        }
        l_Options.onStdoutLine = [p_OutLines](Util::StringView p_Line) {
          if (p_OutLines) {
            p_OutLines->push_back(Util::String(p_Line.data(), p_Line.size()));
          }
        };
        l_Options.onStderrLine = [](Util::StringView p_Line) {
          Output::debug(p_Line);
        };
        return Process::run(l_Options);
      }

      static bool read_file(const Util::String &p_Path,
                            std::string &p_OutContent)
      {
        std::ifstream l_File(to_path(p_Path), std::ios::binary);
        if (!l_File) {
          return false;
        }
        std::ostringstream l_Content;
        l_Content << l_File.rdbuf();
        p_OutContent = l_Content.str();
        return true;
      }

      // Written next to it first and then swapped in, so an interrupted
      // write never leaves a half written project.yaml
      static bool write_file(const Util::String &p_Path,
                             const std::string &p_Content)
      {
        const fs::path l_Path = to_path(p_Path);
        fs::path l_TempPath = l_Path;
        l_TempPath += ".tmp";
        {
          std::ofstream l_File(l_TempPath,
                               std::ios::binary | std::ios::trunc);
          if (!l_File) {
            return false;
          }
          l_File << p_Content;
          l_File.close();
          if (l_File.fail()) {
            return false;
          }
        }
        std::error_code l_Error;
        fs::rename(l_TempPath, l_Path, l_Error);
        return !l_Error;
      }

      // Replaces the value of the top-level version: line as text, so
      // comments, key order and formatting of project.yaml stay as they
      // are (writing it through yaml-cpp would reformat everything)
      static bool replace_version_line(std::string &p_Content,
                                       const Util::String &p_Version)
      {
        size_t l_LineStart = 0;
        while (l_LineStart < p_Content.size()) {
          size_t l_LineEnd = p_Content.find('\n', l_LineStart);
          if (l_LineEnd == std::string::npos) {
            l_LineEnd = p_Content.size();
          }
          size_t l_ValueEnd = l_LineEnd;
          if (l_ValueEnd > l_LineStart && p_Content[l_ValueEnd - 1] == '\r') {
            --l_ValueEnd;
          }

          if (p_Content.compare(l_LineStart, 8, "version:") == 0) {
            p_Content.replace(l_LineStart, l_ValueEnd - l_LineStart,
                              std::string("version: ") + p_Version.c_str());
            return true;
          }
          l_LineStart = l_LineEnd + 1;
        }
        return false;
      }

      static int release_project(Context &p_Context)
      {
        if (!p_Context.require_project_root()) {
          return 1;
        }

        const Util::String l_Root = p_Context.projectRoot;
        const Util::String l_ProjectFile = l_Root + "/project.yaml";

        const Util::String l_Name = read_yaml_scalar(l_ProjectFile, "name");
        if (l_Name.empty()) {
          Output::error("The project has no name", {l_ProjectFile},
                        "add name: to project.yaml");
          return 1;
        }

        const Util::String l_CurrentText =
            read_yaml_scalar(l_ProjectFile, "version");
        Version l_Current;
        if (!parse_version(l_CurrentText, l_Current)) {
          Output::error("The project version is not MAJOR.MINOR.PATCH",
                        {"version: " + l_CurrentText, l_ProjectFile},
                        "set it to e.g. version: 0.1.0");
          return 1;
        }
        const Util::String l_New =
            to_string(bump(l_Current, g_ProjectArgs.part));
        // Same prefix build_project.js uses for git describe
        const Util::String l_Tag = to_lower(l_Name) + "-v" + l_New;

        Output::section("Releasing " + l_Name + " " + l_New);
        Output::detail(l_CurrentText + " to " + l_New + ", tag " + l_Tag);

        // Everything that ends up in the binaries has to be committed,
        // otherwise the tag would not describe what was built
        const Packaging::GitState l_State = Packaging::get_git_state(
            l_Root, {"modules", "project.yaml"});
        if (l_State == Packaging::GitState::Unknown) {
          Output::error("The project is not a git repository", {l_Root},
                        "releases are git commits with a tag, run git "
                        "init or check that git is installed");
          return 1;
        }
        if (l_State == Packaging::GitState::Dirty) {
          Output::error("The project modules or project.yaml have "
                        "uncommitted changes",
                        {l_Root},
                        "commit them first, the release commit only "
                        "contains the version bump and the binaries");
          return 1;
        }

        if (run_git(l_Root, {"rev-parse", "-q", "--verify",
                             "refs/tags/" + l_Tag}) == 0) {
          Output::error("The tag " + l_Tag + " already exists", {},
                        "this version was released before, bump "
                        "another part or delete the tag");
          return 1;
        }

        // Without LFS every release puts ~70 MB of binaries into the
        // project's history for good
        if (!g_ProjectArgs.withoutLfs) {
          Util::List<Util::String> l_Lines;
          run_git(l_Root,
                  {"check-attr", "filter", "--", "engine/bin/lowder.exe"},
                  &l_Lines);
          const bool l_UsesLfs =
              !l_Lines.empty() &&
              l_Lines[0].find(": filter: lfs") != Util::String::npos;
          if (!l_UsesLfs) {
            Output::error(
                "The project does not store the engine binaries with "
                "Git LFS",
                {".gitattributes has no lfs filter for "
                 "engine/bin/lowder.exe"},
                "set up Git LFS for engine/**, bin/*.dll and *.exe "
                "first, or pass --without-lfs to commit them to plain "
                "git");
            return 1;
          }
        }

        std::string l_Original;
        if (!read_file(l_ProjectFile, l_Original)) {
          Output::error("Could not read project.yaml", {l_ProjectFile});
          return 1;
        }
        std::string l_Bumped = l_Original;
        if (!replace_version_line(l_Bumped, l_New) ||
            !write_file(l_ProjectFile, l_Bumped)) {
          Output::error("Could not update the version in project.yaml",
                        {l_ProjectFile},
                        "version: has to be a top-level line");
          return 1;
        }

        Packaging::BundledOptions l_Options;
        l_Options.allowDirty = g_ProjectArgs.allowDirty;
        l_Options.buildId = l_Tag;
        l_Options.symbolStore = g_ProjectArgs.symbolStore.c_str();
        l_Options.checkProjectState = false;
        if (Packaging::package_bundled(p_Context, l_Options) != 0) {
          // Nothing was committed, so the release didn't happen
          write_file(l_ProjectFile, l_Original);
          Output::info("project.yaml is back at version " + l_CurrentText);
          return 1;
        }

        const Util::String l_Message = "Release " + l_Name + " " + l_New;
        const Util::List<Util::String> l_Paths = {
            "engine", "bin", "project.yaml", l_Name + " Editor.exe"};

        // add -A also stages deletions, e.g. old files in bin/. Paths
        // after commit -- limit the commit to them, anything else that
        // happens to be staged stays out of the release commit.
        Util::List<Util::String> l_Add = {"-C", l_Root, "add", "-A", "--"};
        Util::List<Util::String> l_Commit = {"-C", l_Root, "commit", "-m",
                                             l_Message, "--"};
        for (const Util::String &i_Path : l_Paths) {
          l_Add.push_back(i_Path);
          l_Commit.push_back(i_Path);
        }

        if (!Packaging::run_step("Staging the release", "git", l_Add,
                                 l_Root) ||
            !Packaging::run_step("Committing", "git", l_Commit, l_Root)) {
          Output::info("The package and the version bump are in place, "
                       "commit and tag them by hand: git tag -a " +
                       l_Tag);
          return 1;
        }

        if (!Packaging::run_step("Tagging " + l_Tag, "git",
                                 {"-C", l_Root, "tag", "-a", l_Tag, "-m",
                                  l_Name + " " + l_New},
                                 l_Root)) {
          Output::info("The release is committed, tag it by hand: git "
                       "tag -a " +
                       l_Tag);
          return 1;
        }

        Output::success("Released " + l_Name + " " + l_New);
        Output::detail("commit \"" + l_Message + "\", tag " + l_Tag);
        Output::detail("nothing was pushed yet: git push --follow-tags");
        return 0;
      }

      void register_release(CLI::App &p_App)
      {
        CLI::App *l_Release = p_App.add_subcommand(
            "release", "Bump the version, package, commit and tag");
        l_Release->require_subcommand(1);

        CLI::App *l_Project = l_Release->add_subcommand(
            "project", "Release the project with the bundled engine");
        l_Project
            ->add_option("part", g_ProjectArgs.part,
                         "Which part of the version to bump")
            ->required()
            ->check(CLI::IsMember({"major", "minor", "patch"}));
        l_Project->add_flag(
            "--allow-dirty", g_ProjectArgs.allowDirty,
            "Release even if the engine has uncommitted changes (the "
            "project itself always has to be committed)");
        l_Project->add_flag(
            "--without-lfs", g_ProjectArgs.withoutLfs,
            "Commit the binaries even though the project does not use "
            "Git LFS for them");
        l_Project->add_option(
            "--symbol-store", g_ProjectArgs.symbolStore,
            "Store the PDBs here instead of the configured symbol store");

        add_command(l_Project, release_project);
      }
    } // namespace Commands
  } // namespace Cli
} // namespace Low
