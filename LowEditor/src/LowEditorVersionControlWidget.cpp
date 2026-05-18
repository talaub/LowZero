#include "LowEditorVersionControlWidget.h"

#include "LowEditorGui.h"
#include "LowEditorPropertyEditors.h"
#include "LowEditorThemes.h"

#include "LowUtil.h"
#include "LowUtilAssetManager.h"
#include "LowUtilHandle.h"
#include "LowUtilLogger.h"
#include "LowUtilString.h"

#include <imgui.h>
#include "IconsLucide.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <future>

namespace Low {
  namespace Editor {
    namespace {
      enum class ChangeKind
      {
        Modified,
        Added,
        Deleted,
        Renamed,
        Untracked,
        Conflict,
        Other
      };

      struct GitChange
      {
        Util::String status;
        Util::String path;
        Util::String displayName;
        Util::String location;
        ChangeKind kind = ChangeKind::Other;
      };

      struct GitSnapshot
      {
        bool available = false;
        bool loading = false;
        Util::String branch;
        Util::String upstream;
        Util::String message;
        uint32_t ahead = 0;
        uint32_t behind = 0;
        Util::List<GitChange> changes;
      };

      struct GitCommandResult
      {
        int status = 0;
        Util::String output;
      };

      static GitSnapshot g_Snapshot;
      static float g_RefreshTimer = 5.0f;
      static std::future<GitSnapshot> g_PendingRefresh;
      static std::future<GitCommandResult> g_PendingCommand;
      static Util::String g_OperationLabel;
      static Util::String g_LastOperation;
      static Util::String g_LastError;
      static Util::String g_CommitMessage = "";
      static Util::String g_CommitSystem = "";
      static int g_CommitTypeIndex = 1;
      struct CommitTypeOption
      {
        const char *label;
        const char *tag;
      };
      static CommitTypeOption g_CommitTypes[] = {
          {"Add", "ADD"},       {"Change", "CHANGE"},
          {"Fix", "FIX"},       {"Refactor", "REFACTOR"},
          {"Remove", "REMOVE"}, {"Docs", "DOCS"}};
      static constexpr int g_CommitTypeCount =
          sizeof(g_CommitTypes) / sizeof(g_CommitTypes[0]);

      static Util::String trim(const Util::String &p_Text)
      {
        size_t l_Begin = 0;
        while (l_Begin < p_Text.size() &&
               std::isspace(
                   static_cast<unsigned char>(p_Text[l_Begin]))) {
          l_Begin++;
        }

        size_t l_End = p_Text.size();
        while (l_End > l_Begin &&
               std::isspace(
                   static_cast<unsigned char>(p_Text[l_End - 1]))) {
          l_End--;
        }

        return p_Text.substr(l_Begin, l_End - l_Begin);
      }

      static bool starts_with(const Util::String &p_Text,
                              const char *p_Prefix)
      {
        return p_Text.rfind(p_Prefix, 0) == 0;
      }

      static Util::String
      quote_command_argument(const Util::String &p_Text)
      {
        Util::String l_Result = "\"";
        for (char i_Char : p_Text) {
          if (i_Char == '"') {
            l_Result += "\\\"";
          } else {
            l_Result += i_Char;
          }
        }
        l_Result += "\"";
        return l_Result;
      }

      static Util::String
      sanitize_commit_message(const Util::String &p_Text)
      {
        Util::String l_Result;
        for (char i_Char : p_Text) {
          if (i_Char == '\r' || i_Char == '\n') {
            l_Result += ' ';
          } else {
            l_Result += i_Char;
          }
        }
        return trim(l_Result);
      }

      static Util::String build_commit_message()
      {
        Util::String l_Message =
            sanitize_commit_message(g_CommitMessage.data());
        if (l_Message.empty()) {
          return "";
        }

        Util::String l_Result = "#";
        l_Result += g_CommitTypes[g_CommitTypeIndex].tag;

        const Util::String l_System =
            sanitize_commit_message(g_CommitSystem.data());
        if (!l_System.empty()) {
          l_Result += " [";
          l_Result += l_System;
          l_Result += "]";
        }

        l_Result += " ";
        l_Result += l_Message;
        return l_Result;
      }

      static Util::String strip_quotes(const Util::String &p_Text)
      {
        Util::String l_Text = trim(p_Text);
        if (l_Text.size() >= 2 &&
            ((l_Text.front() == '"' && l_Text.back() == '"') ||
             (l_Text.front() == '\'' && l_Text.back() == '\''))) {
          return l_Text.substr(1, l_Text.size() - 2);
        }
        return l_Text;
      }

      static Util::String
      normalize_git_path(const Util::String &p_Path)
      {
        Util::String l_Path = p_Path;
        const size_t l_RenameArrow = l_Path.find(" -> ");
        if (l_RenameArrow != Util::String::npos) {
          l_Path = l_Path.substr(l_RenameArrow + 4);
        }

        std::replace(l_Path.begin(), l_Path.end(), '\\', '/');
        return l_Path;
      }

      static Util::String
      compact_asset_type(const Util::String &p_Path)
      {
        const Util::String l_Path = normalize_git_path(p_Path);
        size_t l_FileStart = l_Path.find_last_of('/');
        if (l_FileStart == Util::String::npos) {
          return "File";
        }

        Util::String l_ParentPath = l_Path.substr(0, l_FileStart);
        size_t l_ParentStart = l_ParentPath.find_last_of('/');
        Util::String l_Type =
            l_ParentStart == Util::String::npos
                ? l_ParentPath
                : l_ParentPath.substr(l_ParentStart + 1);
        if (l_Type.empty()) {
          return "File";
        }

        l_Type[0] = static_cast<char>(
            std::toupper(static_cast<unsigned char>(l_Type[0])));
        return l_Type;
      }

      static Util::String
      fallback_asset_name(const Util::String &p_Path)
      {
        const Util::String l_Path = normalize_git_path(p_Path);
        const size_t l_FileStart = l_Path.find_last_of('/');
        Util::String l_Name = l_FileStart == Util::String::npos
                                  ? l_Path
                                  : l_Path.substr(l_FileStart + 1);

        const Util::String l_YamlSuffix = ".yaml";
        if (l_Name.size() > l_YamlSuffix.size() &&
            l_Name.compare(l_Name.size() - l_YamlSuffix.size(),
                           l_YamlSuffix.size(), l_YamlSuffix) == 0) {
          l_Name =
              l_Name.substr(0, l_Name.size() - l_YamlSuffix.size());
        }

        const size_t l_Extension = l_Name.find_last_of('.');
        if (l_Extension != Util::String::npos) {
          l_Name = l_Name.substr(0, l_Extension);
        }

        return l_Name.empty() ? p_Path : l_Name;
      }

      static Util::String
      read_top_level_asset_name(const Util::String &p_RelativePath)
      {
        namespace fs = std::filesystem;

        const Util::String l_NormalizedPath =
            normalize_git_path(p_RelativePath);
        if (l_NormalizedPath.find(".yaml") == Util::String::npos) {
          return "";
        }

        std::error_code l_Error;
        fs::path l_FilePath(Util::get_project().rootPath.c_str());
        l_FilePath /= fs::path(l_NormalizedPath.c_str());
        l_FilePath = fs::absolute(l_FilePath, l_Error);

        std::ifstream l_File(l_FilePath);
        if (!l_File.is_open()) {
          return "";
        }

        std::string l_Line;
        uint32_t l_LinesRead = 0;
        while (std::getline(l_File, l_Line) && l_LinesRead < 80) {
          l_LinesRead++;
          if (l_Line.empty() ||
              std::isspace(static_cast<unsigned char>(l_Line[0]))) {
            continue;
          }

          const Util::String l_Text = l_Line.c_str();
          if (!starts_with(l_Text, "name:")) {
            continue;
          }

          return strip_quotes(l_Text.substr(5));
        }

        return "";
      }

      static Util::String
      build_change_location(const Util::String &p_Path)
      {
        const Util::String l_Path = normalize_git_path(p_Path);
        const size_t l_DataStart = l_Path.find("data/");
        Util::String l_Location =
            l_DataStart == Util::String::npos
                ? l_Path
                : l_Path.substr(l_DataStart + 5);

        const size_t l_FileStart = l_Location.find_last_of('/');
        if (l_FileStart != Util::String::npos) {
          l_Location = l_Location.substr(0, l_FileStart);
        }

        return l_Location.empty() ? compact_asset_type(p_Path)
                                  : l_Location;
      }

      static Util::String get_project_data_pathspec()
      {
        namespace fs = std::filesystem;

        const Util::String l_Root = Util::get_project().rootPath;
        const Util::String l_Data = Util::get_project().dataPath;
        if (l_Data.empty()) {
          return "data";
        }

        std::error_code l_Error;
        fs::path l_DataPath(l_Data.c_str());
        fs::path l_RootPath(l_Root.c_str());

        if (!l_Root.empty()) {
          l_DataPath = fs::absolute(l_DataPath, l_Error);
          l_RootPath = fs::absolute(l_RootPath, l_Error);
          fs::path l_Relative =
              fs::relative(l_DataPath, l_RootPath, l_Error);
          if (!l_Error && !l_Relative.empty()) {
            return l_Relative.generic_string().c_str();
          }
        }

        return l_DataPath.generic_string().c_str();
      }

      static Util::String
      build_git_command(const Util::String &p_Args)
      {
        const Util::String l_ProjectRoot =
            Util::get_project().rootPath;
        return Util::StringBuilder()
            .append("git -C ")
            .append(quote_command_argument(l_ProjectRoot))
            .append(" ")
            .append(p_Args)
            .get();
      }

      static GitCommandResult
      run_git_command(const Util::String &p_Args)
      {
        GitCommandResult l_Result;
        Util::String l_Output;
        l_Result.status = Util::execute_command(
            build_git_command(p_Args), true, &l_Output);
        l_Result.output = trim(l_Output);
        return l_Result;
      }

      static void parse_branch_line(const Util::String &p_Line,
                                    GitSnapshot &p_Snapshot)
      {
        Util::String l_Branch = trim(p_Line.substr(3));
        const size_t l_TrackingStart = l_Branch.find("...");
        if (l_TrackingStart != Util::String::npos) {
          p_Snapshot.upstream =
              trim(l_Branch.substr(l_TrackingStart + 3));
          l_Branch = l_Branch.substr(0, l_TrackingStart);
        }

        const size_t l_StatusStart = l_Branch.find(" [");
        if (l_StatusStart != Util::String::npos) {
          l_Branch = l_Branch.substr(0, l_StatusStart);
        }

        const size_t l_UpstreamStatus =
            p_Snapshot.upstream.find(" [");
        if (l_UpstreamStatus != Util::String::npos) {
          Util::String l_Status =
              p_Snapshot.upstream.substr(l_UpstreamStatus);
          p_Snapshot.upstream =
              trim(p_Snapshot.upstream.substr(0, l_UpstreamStatus));

          const size_t l_Ahead = l_Status.find("ahead ");
          if (l_Ahead != Util::String::npos) {
            p_Snapshot.ahead = static_cast<uint32_t>(
                std::atoi(l_Status.c_str() + l_Ahead + 6));
          }

          const size_t l_Behind = l_Status.find("behind ");
          if (l_Behind != Util::String::npos) {
            p_Snapshot.behind = static_cast<uint32_t>(
                std::atoi(l_Status.c_str() + l_Behind + 7));
          }
        }

        p_Snapshot.branch = trim(l_Branch);
      }

      static ChangeKind get_change_kind(const Util::String &p_Status)
      {
        if (p_Status == "??") {
          return ChangeKind::Untracked;
        }
        if (p_Status.find('U') != Util::String::npos ||
            p_Status == "AA" || p_Status == "DD") {
          return ChangeKind::Conflict;
        }
        if (p_Status.find('R') != Util::String::npos) {
          return ChangeKind::Renamed;
        }
        if (p_Status.find('D') != Util::String::npos) {
          return ChangeKind::Deleted;
        }
        if (p_Status.find('A') != Util::String::npos) {
          return ChangeKind::Added;
        }
        if (p_Status.find('M') != Util::String::npos) {
          return ChangeKind::Modified;
        }
        return ChangeKind::Other;
      }

      static void parse_change_line(const Util::String &p_Line,
                                    GitSnapshot &p_Snapshot)
      {
        if (p_Line.size() < 4) {
          return;
        }

        GitChange l_Change;
        l_Change.status = p_Line.substr(0, 2);
        l_Change.path = "./" + trim(p_Line.substr(2));
        Util::Handle l_Handle =
            Util::AssetManager::_find_by_path(l_Change.path);
        if (l_Handle.is_registered_type()) {
          Util::RTTI::TypeInfo &l_TypeInfo =
              Util::Handle::get_type_info(l_Handle.get_type());
          if (l_TypeInfo.is_alive(l_Handle)) {
            auto l_Pos = l_TypeInfo.properties.find(N(name));
            if (l_Pos != l_TypeInfo.properties.end()) {
              Util::Name l_Name;
              l_Pos->second.get(l_Handle, &l_Name);
              l_Change.displayName = l_Name.c_str();
            }
          }
        }

        if (l_Change.displayName.empty()) {
          l_Change.displayName =
              read_top_level_asset_name(l_Change.path);
        }
        if (l_Change.displayName.empty()) {
          l_Change.displayName = fallback_asset_name(l_Change.path);
        }
        l_Change.location = build_change_location(l_Change.path);
        l_Change.kind = get_change_kind(l_Change.status);
        p_Snapshot.changes.push_back(l_Change);
      }

      static GitSnapshot refresh_git_snapshot()
      {
        GitSnapshot l_Snapshot;
        const Util::String l_Pathspec = get_project_data_pathspec();
        GitCommandResult l_Result = run_git_command(
            Util::StringBuilder()
                .append("status --short --branch -- ")
                .append(quote_command_argument(l_Pathspec))
                .get());

        if (l_Result.status != 0) {
          l_Snapshot.message = l_Result.output.empty()
                                   ? "Git status failed."
                                   : l_Result.output;
          return l_Snapshot;
        }

        l_Snapshot.available = true;
        size_t l_LineStart = 0;
        while (l_LineStart < l_Result.output.size()) {
          size_t l_LineEnd = l_Result.output.find('\n', l_LineStart);
          if (l_LineEnd == Util::String::npos) {
            l_LineEnd = l_Result.output.size();
          }

          Util::String l_Line = trim(l_Result.output.substr(
              l_LineStart, l_LineEnd - l_LineStart));
          if (!l_Line.empty()) {
            if (starts_with(l_Line, "## ")) {
              parse_branch_line(l_Line, l_Snapshot);
            } else {
              parse_change_line(l_Line, l_Snapshot);
            }
          }

          l_LineStart = l_LineEnd + 1;
        }

        return l_Snapshot;
      }

      static void request_refresh(bool p_Force)
      {
        if (g_PendingRefresh.valid() || g_PendingCommand.valid()) {
          return;
        }

        if (!p_Force && g_RefreshTimer < 5.0f) {
          return;
        }

        g_RefreshTimer = 0.0f;
        g_Snapshot.loading = true;
        g_PendingRefresh = std::async(std::launch::async, [] {
          return refresh_git_snapshot();
        });
      }

      static void update_async_state(float p_Delta)
      {
        g_RefreshTimer += p_Delta;

        if (g_PendingCommand.valid() &&
            g_PendingCommand.wait_for(std::chrono::seconds(0)) ==
                std::future_status::ready) {
          GitCommandResult l_Result = g_PendingCommand.get();
          if (l_Result.status == 0) {
            g_LastOperation = g_OperationLabel + " completed.";
            g_LastError = "";
          } else {
            g_LastOperation = "";
            g_LastError = l_Result.output.empty()
                              ? g_OperationLabel + " failed."
                              : l_Result.output;
          }
          g_OperationLabel = "";
          request_refresh(true);
        }

        if (g_PendingRefresh.valid() &&
            g_PendingRefresh.wait_for(std::chrono::seconds(0)) ==
                std::future_status::ready) {
          g_Snapshot = g_PendingRefresh.get();
          g_Snapshot.loading = false;
        }

        request_refresh(false);
      }

      static bool is_busy()
      {
        return g_PendingCommand.valid() || g_PendingRefresh.valid();
      }

      static void start_operation(const Util::String &p_Label,
                                  const Util::String &p_Args)
      {
        if (g_PendingCommand.valid()) {
          return;
        }

        g_OperationLabel = p_Label;
        g_LastOperation = "";
        g_LastError = "";
        g_PendingCommand = std::async(std::launch::async, [p_Args] {
          return run_git_command(p_Args);
        });
      }

      static void start_commit_operation()
      {
        const Util::String l_Message = build_commit_message();
        if (l_Message.empty()) {
          g_LastError = "Please write a short commit message first.";
          return;
        }

        const Util::String l_Pathspec = get_project_data_pathspec();
        if (g_PendingCommand.valid()) {
          return;
        }

        g_OperationLabel = "Commit";
        g_LastOperation = "";
        g_LastError = "";
        g_PendingCommand =
            std::async(std::launch::async, [l_Pathspec, l_Message] {
              GitCommandResult l_Add = run_git_command(
                  Util::StringBuilder()
                      .append("add -- ")
                      .append(quote_command_argument(l_Pathspec))
                      .get());
              if (l_Add.status != 0) {
                return l_Add;
              }

              return run_git_command(
                  Util::StringBuilder()
                      .append("commit -m ")
                      .append(quote_command_argument(l_Message))
                      .append(" -- ")
                      .append(quote_command_argument(l_Pathspec))
                      .get());
            });
      }

      static Math::Color get_change_color(ChangeKind p_Kind)
      {
        Theme &l_Theme = theme_get_current();
        switch (p_Kind) {
        case ChangeKind::Added:
        case ChangeKind::Untracked:
          return l_Theme.add;
        case ChangeKind::Deleted:
          return l_Theme.remove;
        case ChangeKind::Renamed:
          return l_Theme.info;
        case ChangeKind::Conflict:
          return l_Theme.error;
        case ChangeKind::Modified:
          return l_Theme.warning;
        case ChangeKind::Other:
          return l_Theme.subtext;
        }
        return l_Theme.subtext;
      }

      static const char *get_change_label(ChangeKind p_Kind)
      {
        switch (p_Kind) {
        case ChangeKind::Added:
          return "Added";
        case ChangeKind::Deleted:
          return "Deleted";
        case ChangeKind::Renamed:
          return "Renamed";
        case ChangeKind::Untracked:
          return "New";
        case ChangeKind::Conflict:
          return "Conflict";
        case ChangeKind::Modified:
          return "Modified";
        case ChangeKind::Other:
          return "Changed";
        }
        return "Changed";
      }

      static const char *get_change_icon(ChangeKind p_Kind)
      {
        switch (p_Kind) {
        case ChangeKind::Added:
        case ChangeKind::Untracked:
          return ICON_LC_FILE_PLUS;
        case ChangeKind::Deleted:
          return ICON_LC_FILE_MINUS;
        case ChangeKind::Renamed:
          return ICON_LC_GIT_COMPARE_ARROWS;
        case ChangeKind::Conflict:
          return ICON_LC_CIRCLE_ALERT;
        case ChangeKind::Modified:
          return ICON_LC_FILE_PEN;
        case ChangeKind::Other:
          return ICON_LC_FILE;
        }
        return ICON_LC_FILE;
      }

      static void render_metric(const char *p_Label,
                                const char *p_Value,
                                const Math::Color &p_Color)
      {
        Theme &l_Theme = theme_get_current();
        ImDrawList *l_DrawList = ImGui::GetWindowDrawList();
        const ImVec2 l_Pos = ImGui::GetCursorScreenPos();
        const float l_Width = ImGui::GetContentRegionAvail().x;
        const float l_Height = 32.0f;
        const ImVec2 l_Max(l_Pos.x + l_Width, l_Pos.y + l_Height);

        l_DrawList->AddRectFilled(
            l_Pos, l_Max, color_to_imcolor(l_Theme.input), 3.0f);
        l_DrawList->AddRectFilled(l_Pos,
                                  ImVec2(l_Pos.x + 3.0f, l_Max.y),
                                  color_to_imcolor(p_Color), 3.0f,
                                  ImDrawFlags_RoundCornersLeft);

        ImGui::SetCursorScreenPos(
            ImVec2(l_Pos.x + 9.0f, l_Pos.y + 5.0f));
        ImGui::PushStyleColor(ImGuiCol_Text,
                              color_to_imvec4(l_Theme.subtext));
        ImGui::TextUnformatted(p_Label);
        ImGui::PopStyleColor();

        const ImVec2 l_ValueSize = ImGui::CalcTextSize(p_Value);
        ImGui::SetCursorScreenPos(
            ImVec2(l_Max.x - l_ValueSize.x - 9.0f, l_Pos.y + 5.0f));
        ImGui::TextUnformatted(p_Value);

        ImGui::SetCursorScreenPos(ImVec2(l_Pos.x, l_Max.y + 5.0f));
      }

      static bool
      render_toolbar_button(const char *p_Label, const char *p_Icon,
                            const Math::Color &p_IconColor,
                            bool p_Disabled)
      {
        return Gui::Button(p_Label, p_Disabled, p_Icon, p_IconColor);
      }

      static void render_status_line()
      {
        Theme &l_Theme = theme_get_current();
        const ImVec2 l_Start = ImGui::GetCursorScreenPos();
        const float l_Height = ImGui::GetFrameHeight();

        ImGui::BeginChild("VersionControlStatusLine",
                          ImVec2(0.0f, l_Height), false,
                          ImGuiWindowFlags_NoScrollbar |
                              ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
        if (is_busy()) {
          Gui::spinner("VersionControlBusy", 6.0f, 2, l_Theme.info);
          ImGui::SameLine(0.0f, 8.0f);
          ImGui::TextUnformatted(g_OperationLabel.empty()
                                     ? "Refreshing..."
                                     : g_OperationLabel.c_str());
        } else if (!g_LastError.empty()) {
          ImGui::PushStyleColor(ImGuiCol_Text,
                                color_to_imvec4(l_Theme.error));
          ImGui::TextUnformatted(g_LastError.c_str());
          ImGui::PopStyleColor();
        } else if (!g_LastOperation.empty()) {
          ImGui::PushStyleColor(ImGuiCol_Text,
                                color_to_imvec4(l_Theme.success));
          ImGui::TextUnformatted(g_LastOperation.c_str());
          ImGui::PopStyleColor();
        } else {
          ImGui::TextDisabled("Project data only");
        }

        ImGui::EndChild();
        ImGui::SetCursorScreenPos(
            ImVec2(l_Start.x, l_Start.y + l_Height));
      }

      static void render_labeled_text_input(const char *p_Label,
                                            char *p_Buffer,
                                            int p_Length,
                                            float p_LabelWidth)
      {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(p_Label);
        ImGui::SameLine(p_LabelWidth);
        ImGui::PushItemWidth(-1.0f);
        Gui::InputText(p_Label, p_Buffer, p_Length);
        ImGui::PopItemWidth();
      }

      static void render_change_table(const GitSnapshot &p_Snapshot)
      {
        Theme &l_Theme = theme_get_current();
        ImGui::PushStyleColor(ImGuiCol_TableHeaderBg,
                              color_to_imvec4(l_Theme.header));
        ImGui::PushStyleColor(ImGuiCol_TableRowBg,
                              color_to_imvec4(l_Theme.base));
        ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt,
                              color_to_imvec4(l_Theme.input));

        if (ImGui::BeginTable("VersionControlChanges", 3,
                              ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_BordersInnerV |
                                  ImGuiTableFlags_SizingStretchProp |
                                  ImGuiTableFlags_ScrollY,
                              ImVec2(0.0f, 0.0f))) {
          ImGui::TableSetupColumn(
              "Change", ImGuiTableColumnFlags_WidthFixed, 92.0f);
          ImGui::TableSetupColumn("Asset");
          ImGui::TableSetupColumn(
              "Location", ImGuiTableColumnFlags_WidthStretch, 0.65f);
          ImGui::TableHeadersRow();

          for (const GitChange &i_Change : p_Snapshot.changes) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::PushStyleColor(
                ImGuiCol_Text,
                color_to_imvec4(get_change_color(i_Change.kind)));
            ImGui::Text("%s %s", get_change_icon(i_Change.kind),
                        get_change_label(i_Change.kind));
            ImGui::PopStyleColor();

            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(i_Change.displayName.c_str());
            if (ImGui::IsItemHovered()) {
              ImGui::SetTooltip("%s", i_Change.path.c_str());
            }

            ImGui::TableSetColumnIndex(2);
            ImGui::PushStyleColor(ImGuiCol_Text,
                                  color_to_imvec4(l_Theme.subtext));
            ImGui::TextUnformatted(i_Change.location.c_str());
            ImGui::PopStyleColor();
          }

          if (p_Snapshot.changes.empty()) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled("Clean");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextDisabled("No project/data changes");
            ImGui::TableSetColumnIndex(2);
            ImGui::TextDisabled("-");
          }

          ImGui::EndTable();
        }

        ImGui::PopStyleColor(3);
      }
    } // namespace

    void VersionControlWidget::render(float p_Delta)
    {
      update_async_state(p_Delta);

      ImGui::Begin(ICON_LC_GIT_BRANCH " Version control", &m_Open);

      Theme &l_Theme = theme_get_current();

      render_status_line();

      ImGui::Spacing();

      char l_CountBuffer[32];
      std::snprintf(l_CountBuffer, sizeof(l_CountBuffer), "%u",
                    static_cast<uint32_t>(g_Snapshot.changes.size()));
      render_metric(ICON_LC_GIT_BRANCH " Branch",
                    g_Snapshot.branch.empty()
                        ? "unknown"
                        : g_Snapshot.branch.c_str(),
                    l_Theme.info);
      render_metric(ICON_LC_FILES " Changed files", l_CountBuffer,
                    g_Snapshot.changes.empty() ? l_Theme.success
                                               : l_Theme.warning);

      char l_SyncBuffer[64];
      std::snprintf(l_SyncBuffer, sizeof(l_SyncBuffer),
                    "%u ahead  %u behind", g_Snapshot.ahead,
                    g_Snapshot.behind);
      render_metric(ICON_LC_CLOUD " Remote", l_SyncBuffer,
                    (g_Snapshot.ahead || g_Snapshot.behind)
                        ? l_Theme.warning
                        : l_Theme.success);

      ImGui::Spacing();
      const bool l_Busy = is_busy();
      if (render_toolbar_button("Refresh", ICON_LC_REFRESH_CW,
                                l_Theme.info, l_Busy)) {
        request_refresh(true);
      }
      ImGui::SameLine();
      if (render_toolbar_button("Pull", ICON_LC_CLOUD_DOWNLOAD,
                                l_Theme.success, l_Busy)) {
        start_operation("Pull", "pull --ff-only");
      }
      ImGui::SameLine();
      if (render_toolbar_button("Push", ICON_LC_CLOUD_UPLOAD,
                                l_Theme.warning,
                                l_Busy || g_Snapshot.ahead == 0)) {
        start_operation("Push", "push");
      }

      ImGui::Spacing();
      ImGui::TextDisabled("Commit message");

      const float l_LabelWidth = 78.0f;
      PropertyEditors::render_line("Type", []() -> bool {
        if (ImGui::BeginCombo(
                "##committype",
                g_CommitTypes[g_CommitTypeIndex].label)) {
          for (int i = 0; i < g_CommitTypeCount; ++i) {
            const bool l_Selected = g_CommitTypeIndex == i;
            if (ImGui::Selectable(g_CommitTypes[i].label,
                                  l_Selected)) {
              g_CommitTypeIndex = i;
            }
            if (l_Selected) {
              ImGui::SetItemDefaultFocus();
            }
          }
          ImGui::EndCombo();
        }
        return false;
      });

      PropertyEditors::render_line("Project", []() -> bool {
        return Gui::InputText("systeminput", g_CommitSystem);
      });
      PropertyEditors::render_line("Message", []() -> bool {
        return Gui::InputText("messageinput", g_CommitMessage);
      });

      const Util::String l_CommitPreview = build_commit_message();
      PropertyEditors::render_line(
          "Final", [&l_CommitPreview]() -> bool {
            ImGui::Text(l_CommitPreview.c_str());
            return false;
          });

      ImGui::Dummy(ImVec2{0, 5});

      const bool l_CanCommit = !l_Busy &&
                               !g_Snapshot.changes.empty() &&
                               !l_CommitPreview.empty();
      if (render_toolbar_button("Commit project data",
                                ICON_LC_GIT_COMMIT_HORIZONTAL,
                                l_Theme.save, !l_CanCommit)) {
        start_commit_operation();
      }

      ImGui::Spacing();
      if (Gui::CollapsibleHeader("Project data changes",
                                 ICON_LC_LIST_CHECKS, l_Theme.info)) {
        render_change_table(g_Snapshot);
      }

      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
