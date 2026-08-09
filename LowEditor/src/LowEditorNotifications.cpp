#include "LowEditorNotifications.h"

#include "LowEditorThemes.h"
#include "LowEditorFonts.h"

#include "LowUtilString.h"

#include "imgui.h"

namespace Low {
  namespace Editor {
    Util::List<Notification> g_Notifications;

    void push_notification(const Util::String &icon,
                           const Util::String &title,
                           const Util::String &subtitle,
                           const Util::String &message, float duration,
                           Math::Color color)
    {
      g_Notifications.emplace_back(icon, title, subtitle, message,
                                   duration, color);
    }

    // Single-purpose smoothstep, used for both the intro slide/fade-in
    // and the outro fade-out. Using a plain float here instead of a
    // Core::Tween to avoid an allocate/destroy handle per toast, given
    // how short-lived and high-churn notifications are.
    static float smoothstep01(float p_T)
    {
      const float t = Math::Util::clamp(p_T, 0.0f, 1.0f);
      return t * t * (3.0f - 2.0f * t);
    }

    void render_notifications(float p_Delta)
    {
      constexpr float k_Padding = 10.0f;
      constexpr float k_Width = 320.0f;
      constexpr float k_IntroTime = 0.22f;
      constexpr float k_OutroTime = 0.45f;
      constexpr float k_SlideDistance = 32.0f;
      constexpr float k_IconBadgeSize = 34.0f;
      constexpr float k_ProgressBarHeight = 3.0f;

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
      style.WindowRounding = 10.0f;

      ImVec2 anchor = ImVec2(screen_size.x - k_Width - k_Padding,
                             screen_size.y - k_Padding - 50.0f);

      const Theme &l_Theme = theme_get_current();

      for (int i = static_cast<int>(g_Notifications.size()) - 1;
           i >= 0; --i) {
        Notification &n = g_Notifications[i];
        n.age += p_Delta;
        n.time_remaining -= p_Delta;

        const float l_IntroT = smoothstep01(n.age / k_IntroTime);
        const float l_OutroT =
            smoothstep01(n.time_remaining / k_OutroTime);
        const float l_Alpha = std::min(l_IntroT, l_OutroT);
        const float l_SlideX = (1.0f - l_IntroT) * k_SlideDistance;

        ImGui::SetNextWindowBgAlpha(l_Alpha * 0.92f);
        ImGui::SetNextWindowPos(
            ImVec2(anchor.x + l_SlideX, anchor.y), ImGuiCond_Always,
            ImVec2(0.0f, 1.0f));
        ImGui::SetNextWindowSize(ImVec2(k_Width, 0));

        Util::StringBuilder l_IdBuilder;
        l_IdBuilder.append("##Notification_");
        l_IdBuilder.append(i);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                            ImVec2(14.0f, 12.0f));
        ImGui::Begin(l_IdBuilder.get().c_str(), nullptr,
                     ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoNav);

        ImDrawList *l_DrawList = ImGui::GetWindowDrawList();

        // Left accent bar, tinted with the notification color.
        {
          const ImVec2 l_WindowPos = ImGui::GetWindowPos();
          const ImVec2 l_WindowSize = ImGui::GetWindowSize();
          const ImU32 l_AccentColor = ImGui::GetColorU32(
              ImVec4(n.color.x, n.color.y, n.color.z, l_Alpha));
          l_DrawList->AddRectFilled(
              l_WindowPos,
              ImVec2(l_WindowPos.x + 3.0f,
                     l_WindowPos.y + l_WindowSize.y),
              l_AccentColor, 3.0f, ImDrawFlags_RoundCornersLeft);
        }

        ImGui::BeginGroup(); // Entire notification block

        // Icon badge
        {
          const ImVec2 l_BadgeMin = ImGui::GetCursorScreenPos();
          const ImVec2 l_BadgeMax =
              ImVec2(l_BadgeMin.x + k_IconBadgeSize,
                     l_BadgeMin.y + k_IconBadgeSize);
          const ImU32 l_BadgeBg = ImGui::GetColorU32(ImVec4(
              n.color.x, n.color.y, n.color.z, 0.16f * l_Alpha));
          l_DrawList->AddRectFilled(l_BadgeMin, l_BadgeMax, l_BadgeBg,
                                    8.0f);

          ImGui::PushFont(Fonts::UI(18.0f));
          const ImVec2 l_IconSize =
              ImGui::CalcTextSize(n.icon.c_str());
          const ImVec2 l_IconPos = ImVec2(
              l_BadgeMin.x + (k_IconBadgeSize - l_IconSize.x) * 0.5f,
              l_BadgeMin.y + (k_IconBadgeSize - l_IconSize.y) * 0.5f);
          const ImU32 l_IconColor = ImGui::GetColorU32(
              ImVec4(n.color.x, n.color.y, n.color.z, l_Alpha));
          l_DrawList->AddText(l_IconPos, l_IconColor, n.icon.c_str());
          ImGui::PopFont();

          ImGui::Dummy(ImVec2(k_IconBadgeSize, k_IconBadgeSize));
        }

        ImGui::SameLine(0.0f, 12.0f);

        // Text block: title, optional subtitle, optional message
        ImGui::BeginGroup();

        ImGui::PushFont(Fonts::UI(15.0f, Fonts::Weight::Bold));
        ImGui::PushStyleColor(
            ImGuiCol_Text, ImVec4(l_Theme.text.x, l_Theme.text.y,
                                  l_Theme.text.z, l_Alpha));
        ImGui::TextUnformatted(n.title.c_str());
        ImGui::PopStyleColor();
        ImGui::PopFont();

        if (!n.subtitle.empty()) {
          ImGui::PushStyleColor(
              ImGuiCol_Text,
              ImVec4(l_Theme.subtext.x, l_Theme.subtext.y,
                     l_Theme.subtext.z, l_Alpha));
          ImGui::TextUnformatted(n.subtitle.c_str());
          ImGui::PopStyleColor();
        }

        if (!n.message.empty()) {
          ImGui::Dummy(ImVec2(0.0f, 3.0f));
          ImGui::PushStyleColor(
              ImGuiCol_Text, ImVec4(l_Theme.text.x, l_Theme.text.y,
                                    l_Theme.text.z, l_Alpha * 0.9f));
          ImGui::PushTextWrapPos(
              ImGui::GetCursorPosX() +
              (k_Width - k_IconBadgeSize - 12.0f - 30.0f));
          ImGui::TextWrapped("%s", n.message.c_str());
          ImGui::PopTextWrapPos();
          ImGui::PopStyleColor();
        }

        ImGui::EndGroup();
        ImGui::EndGroup();

        // Timer bar showing remaining lifetime
        {
          ImGui::Dummy(ImVec2(0.0f, 10.0f));
          const ImVec2 l_BarMin = ImGui::GetCursorScreenPos();
          const float l_BarWidth = ImGui::GetContentRegionAvail().x;
          const ImVec2 l_BarMax = ImVec2(
              l_BarMin.x + l_BarWidth, l_BarMin.y + k_ProgressBarHeight);
          const ImU32 l_BarBg = ImGui::GetColorU32(
              ImVec4(1.0f, 1.0f, 1.0f, 0.08f * l_Alpha));
          l_DrawList->AddRectFilled(l_BarMin, l_BarMax, l_BarBg,
                                    k_ProgressBarHeight * 0.5f);

          const float l_Frac =
              n.duration > 0.0f
                  ? Math::Util::clamp(n.time_remaining / n.duration,
                                      0.0f, 1.0f)
                  : 0.0f;
          if (l_Frac > 0.0f) {
            const ImU32 l_BarFg = ImGui::GetColorU32(
                ImVec4(n.color.x, n.color.y, n.color.z, 0.9f * l_Alpha));
            l_DrawList->AddRectFilled(
                l_BarMin,
                ImVec2(l_BarMin.x + l_BarWidth * l_Frac, l_BarMax.y),
                l_BarFg, k_ProgressBarHeight * 0.5f);
          }
          ImGui::Dummy(ImVec2(l_BarWidth, k_ProgressBarHeight));
        }

        const ImVec2 l_NotificationSize = ImGui::GetWindowSize();

        ImGui::End();
        ImGui::PopStyleVar();

        anchor.y -= l_NotificationSize.y + k_Padding;
        if (n.time_remaining <= 0.0f) {
          g_Notifications.erase(g_Notifications.begin() + i);
        }
      }
      style.WindowRounding = prevRounding;
    }
  } // namespace Editor
} // namespace Low
