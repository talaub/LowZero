#include "LowEditorNotifications.h"

#include "LowEditorThemes.h"

#include "LowRendererImGuiHelper.h"

#include "LowUtilString.h"

#include "imgui.h"

namespace Low {
  namespace Editor {
    Util::List<Notification> g_Notifications;

    void push_notification(const Util::String icon,
                           const Util::String title,
                           const Util::String msg, float duration,
                           Math::Color color)
    {
      g_Notifications.emplace_back(icon, title, msg, duration, color);
    }

    void render_notifications(float p_Delta)
    {
      const float padding = 10.0f;
      const float notificationWidth = 300.0f;
      ImVec2 screen_size = ImGui::GetIO().DisplaySize;

      ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();
      if (!platform_io.Monitors.empty()) {
        const ImGuiPlatformMonitor &main_monitor =
            platform_io.Monitors[0];
        screen_size = main_monitor.MainSize;
      }

      ImGuiStyle &style = ImGui::GetStyle();

      // Save current rounding
      float prevRounding = style.WindowRounding;

      // Set custom rounding
      style.WindowRounding = 12.0f; // Adjust as needed

      ImVec2 start_pos =
          ImVec2(screen_size.x - notificationWidth - padding,
                 screen_size.y - padding - 50.0f);

      for (int i = static_cast<int>(g_Notifications.size()) - 1;
           i >= 0; --i) {
        auto &n = g_Notifications[i];

        float alpha = std::min(n.time_remaining, 1.0f);
        ImGui::SetNextWindowBgAlpha(alpha * 0.85f);
        ImGui::SetNextWindowPos(start_pos, ImGuiCond_Always,
                                ImVec2(0.0f, 1.0f));
        ImGui::SetNextWindowSize(ImVec2(notificationWidth, 0));

        Util::StringBuilder l_IdBuilder;
        l_IdBuilder.append("##Notification_");
        l_IdBuilder.append(i);

        ImGui::Begin(l_IdBuilder.get().c_str(), nullptr,
                     ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoNav);

        float spacing = 10.0f;

        ImGui::BeginGroup(); // Entire notification block

        // Icon block
        ImGui::BeginGroup();
        ImGui::PushFont(Renderer::ImGuiHelper::fonts()
                            .icon_800); // Use your icon font
        ImGui::PushStyleColor(ImGuiCol_Text,
                              color_to_imvec4(n.color));
        ImGui::TextUnformatted(
            n.icon.c_str()); // Just the icon character
        ImGui::PopStyleColor();
        ImGui::PopFont();
        ImGui::EndGroup();

        ImGui::SameLine(0.0f, spacing);

        // Text block
        ImGui::BeginGroup();

        // Title - larger, normal text color
        ImGui::PushStyleColor(
            ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_Text));
        ImGui::SetWindowFontScale(1.2f); // Slightly larger
        ImGui::TextUnformatted(n.title.c_str());
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();

        // Message - regular
        ImGui::TextWrapped("%s", n.message.c_str());

        ImGui::EndGroup();

        ImGui::EndGroup();

        ImVec2 notificationSize = ImGui::GetWindowSize();

        ImGui::End();

        start_pos.y -= notificationSize.y + padding;
        n.time_remaining -= p_Delta;
        if (n.time_remaining <= 0) {
          g_Notifications.erase(g_Notifications.begin() + i);
        }
      }
      style.WindowRounding = prevRounding;
    }
  } // namespace Editor
} // namespace Low
