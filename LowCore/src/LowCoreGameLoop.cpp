#include "LowCoreGameLoop.h"

#include "LowCore.h"
#include "LowCoreRegionSystem.h"
#include "LowCoreTransformSystem.h"
#include "LowCoreLightSystem.h"
#include "LowCoreMeshRendererSystem.h"
#include "LowCoreTaskScheduler.h"
#include "LowCoreTransform.h"
#include "LowCorePhysicsSystem.h"
#include "LowCoreNavmeshSystem.h"
#include "LowCoreCflatScripting.h"
#include "LowCoreCameraSystem.h"

#include "LowCoreMeshResource.h"
#include "LowCoreTexture2D.h"
#include "LowCoreFont.h"

#include "LowCoreUiElement.h"
#include "LowCoreUiDisplay.h"
#include "LowCoreUiImage.h"
#include "LowCoreUiText.h"
#include "LowCoreUiDisplaySystem.h"
#include "LowCoreUiImageSystem.h"
#include "LowCoreUiTextSystem.h"
#include "LowCoreUiViewSystem.h"

#include <chrono>
#include <microprofile.h>

#include "LowUtilLogger.h"
#include "LowUtilContainers.h"
#include "LowUtilJobManager.h"
#include "LowUtilProfiler.h"
#include "LowUtil.h"

#include "LowRenderer.h"

namespace Low {
  namespace Core {
    namespace GameLoop {
      using namespace std::chrono;

      float g_TestCounter = 0.0f;

      bool g_Running = false;
      uint32_t g_LastFps = 0u;

      uint64_t g_Frames = 0ull;

      Util::List<System::TickCallback> g_TickCallbacks;

      static void execute_ticks(float p_Delta)
      {
        static bool l_FirstRun = true;
        Util::tick(p_Delta);

        Renderer::tick(p_Delta, get_engine_state());
        System::Transform::tick(p_Delta, get_engine_state());
        UI::System::View::tick(p_Delta, get_engine_state());
        UI::System::Display::tick(p_Delta, get_engine_state());
        System::Light::tick(p_Delta, get_engine_state());
        System::Region::tick(p_Delta, get_engine_state());
        System::Camera::tick(p_Delta, get_engine_state());
        if (!l_FirstRun) {
          System::Physics::tick(p_Delta, get_engine_state());
          System::Navmesh::tick(p_Delta, get_engine_state());
        }

        Scripting::tick(p_Delta, get_engine_state());

        g_TestCounter += p_Delta;

        if (g_TestCounter > 1.0f) {
          g_TestCounter = 0.0f;

          float param = 1.2f;

          CflatVoidCall(
              MtdScripts::StatusEffect::Ignite::test_function,
              CflatArg(param));
        }

        MeshResource::update();
        Texture2D::update();
        Font::update();

        for (auto it = g_TickCallbacks.begin();
             it != g_TickCallbacks.end(); ++it) {
          (*it)(p_Delta, get_engine_state());
        }

        if (get_engine_state() == Util::EngineState::PLAYING) {
          Util::String l_TickFunctionName =
              get_current_gamemode().get_tick_function_name();
          Cflat::Identifier l_FunctionNameCflat(
              l_TickFunctionName.c_str());

          Cflat::Function *l_Function =
              CflatGlobal::getEnvironment()->getFunction(
                  l_FunctionNameCflat);

          LOW_ASSERT(l_Function,
                     "Could not find gamemode tick function");

          static const Cflat::TypeUsage kTypeUsageFloat =
              Scripting::get_environment()->getTypeUsage("float");

          Cflat::Value l_ReturnValue;
          CflatArgsVector(Cflat::Value) l_Arguments;
          Cflat::Value l_Val;
          l_Val.initExternal(kTypeUsageFloat);
          l_Val.set(&p_Delta);
          l_Arguments.push_back(l_Val);
          l_Function->execute(l_Arguments, &l_ReturnValue);
        }

        System::MeshRenderer::tick(p_Delta, get_engine_state());
        UI::System::Image::tick(p_Delta, get_engine_state());
        UI::System::Text::tick(p_Delta, get_engine_state());

        l_FirstRun = false;
      }

      static void execute_late_ticks(float p_Delta)
      {
        static bool l_FirstRun = true;
        if (!l_FirstRun) {
          System::Physics::late_tick(p_Delta, get_engine_state());
        }
        System::Transform::late_tick(p_Delta, get_engine_state());

        Renderer::late_tick(p_Delta, get_engine_state());
        l_FirstRun = false;
      }

      static void create_combat_ui()
      {

        Util::String l_Path = "arial.ttf";
        Font l_Font = Font::make(l_Path);

        {
          UI::View l_View = UI::View::make(N(EndTurnView));
          l_View.set_view_template(false);
          l_View.load_elements();

          l_View.pixel_position(Math::Vector2(50.0f, 50.0f));
          l_View.layer_offset(0);
          l_View.scale_multiplier(1.0f);

          {
            UI::Element l_ResourceElement =
                UI::Element::make(N(endturntext), l_View);
            l_ResourceElement.set_click_passthrough(false);

            UI::Component::Display l_Display =
                UI::Component::Display::make(l_ResourceElement);

            l_Display.pixel_position(Math::Vector2(0.0f, 0.0f));
            l_Display.rotation(0.0f);
            l_Display.pixel_scale(Math::Vector2(100.0f, 30.0f));
            l_Display.layer(3);

            UI::Component::Text l_Text =
                UI::Component::Text::make(l_ResourceElement);
            l_Text.set_font(l_Font);
            l_Text.set_text(Util::String("End turn"));
            l_Text.set_color(Math::Color(0.2f, 0.2f, 0.2f, 1.0f));
            l_Text.set_size(0.5f);
          }
        }

        {
          UI::View l_View = UI::View::make(N(Player_HP));
          l_View.set_view_template(false);
          l_View.load_elements();

          l_View.pixel_position(Math::Vector2(50.0f, 50.0f));
          l_View.layer_offset(0);
          l_View.scale_multiplier(1.0f);

          {
            UI::Element l_ResourceElement =
                UI::Element::make(N(playerhp), l_View);
            l_ResourceElement.set_click_passthrough(true);

            UI::Component::Display l_Display =
                UI::Component::Display::make(l_ResourceElement);

            l_Display.pixel_position(Math::Vector2(0.0f, 0.0f));
            l_Display.rotation(0.0f);
            l_Display.pixel_scale(Math::Vector2(30.0f, 100.0f));
            l_Display.layer(3);

            UI::Component::Text l_Text =
                UI::Component::Text::make(l_ResourceElement);
            l_Text.set_font(l_Font);
            l_Text.set_text(Util::String("HP: 100"));
            l_Text.set_color(Math::Color(0.2f, 0.2f, 0.2f, 1.0f));
            l_Text.set_size(0.7f);
          }

          {
            UI::Element l_ResourceElement =
                UI::Element::make(N(playermana), l_View);
            l_ResourceElement.set_click_passthrough(true);

            UI::Component::Display l_Display =
                UI::Component::Display::make(l_ResourceElement);

            l_Display.pixel_position(Math::Vector2(0.0f, 50.0f));
            l_Display.rotation(0.0f);
            l_Display.pixel_scale(Math::Vector2(30.0f, 100.0f));
            l_Display.layer(3);

            UI::Component::Text l_Text =
                UI::Component::Text::make(l_ResourceElement);
            l_Text.set_font(l_Font);
            l_Text.set_text(Util::String("Mana: 5"));
            l_Text.set_color(Math::Color(0.2f, 0.2f, 0.5f, 1.0f));
            l_Text.set_size(0.7f);
          }
        }

        {
          UI::View l_View = UI::View::make(N(Enemy_HP));
          l_View.set_view_template(false);
          l_View.load_elements();

          l_View.pixel_position(Math::Vector2(200.0f, 50.0f));
          l_View.layer_offset(0);
          l_View.scale_multiplier(1.0f);

          {
            UI::Element l_ResourceElement =
                UI::Element::make(N(enemyhp), l_View);
            l_ResourceElement.set_click_passthrough(true);

            UI::Component::Display l_Display =
                UI::Component::Display::make(l_ResourceElement);

            l_Display.pixel_position(Math::Vector2(0.0f, 0.0f));
            l_Display.rotation(0.0f);
            l_Display.pixel_scale(Math::Vector2(30.0f, 100.0f));
            l_Display.layer(3);

            UI::Component::Text l_Text =
                UI::Component::Text::make(l_ResourceElement);
            l_Text.set_font(l_Font);
            l_Text.set_text(Util::String("Enemy HP: 100"));
            l_Text.set_color(Math::Color(0.8f, 0.2f, 0.2f, 1.0f));
            l_Text.set_size(0.7f);
          }
        }
      }

      static void test_ui()
      {
        create_combat_ui();

        UI::View l_View = UI::View::make(N(CardTemplateView));
        l_View.set_view_template(true);
        l_View.load_elements();

        UI::Element l_Element = UI::Element::make(N(test), l_View);
        l_Element.set_click_passthrough(false);

        Util::String l_Path = "arial.ttf";
        Font l_Font = Font::make(l_Path);

        // Background
        {
          UI::Component::Display l_Display =
              UI::Component::Display::make(l_Element);
          l_Display.pixel_position(Math::Vector2(0.0f, 0.0f));
          l_Display.rotation(0.0f);
          l_Display.pixel_scale(Math::Vector2(200.0f, 280.0f));
          l_Display.layer(1);

          UI::Component::Image l_Image =
              UI::Component::Image::make(l_Element);

          Core::Texture2D l_Texture =
              Core::Texture2D::make(Util::String("card_bg.ktx"));
          l_Image.set_texture(l_Texture);
        }

        // Background icon
        {
          UI::Element l_BgIconElement =
              UI::Element::make(N(cardbg), l_View);
          l_BgIconElement.set_click_passthrough(true);

          UI::Component::Display l_Display =
              UI::Component::Display::make(l_BgIconElement);

          l_Display.pixel_position(Math::Vector2(10.0f, 50.0f));
          l_Display.rotation(0.0f);
          l_Display.pixel_scale(Math::Vector2(180.0f, 180.0f));
          l_Display.layer(2);

          l_Display.set_parent(l_Element.get_display().get_id());

          UI::Component::Image l_Image =
              UI::Component::Image::make(l_BgIconElement);

          /*
          Core::Texture2D l_Texture =
              Core::Texture2D::make(Util::String("magic.ktx"));
          l_Image.set_texture(l_Texture);
          */
        }

        // Resource icon
        {
          UI::Element l_ResourceIconElement =
              UI::Element::make(N(resourcebg), l_View);
          l_ResourceIconElement.set_click_passthrough(true);

          UI::Component::Display l_Display =
              UI::Component::Display::make(l_ResourceIconElement);

          l_Display.pixel_position(Math::Vector2(5.0f, 5.0f));
          l_Display.rotation(0.0f);
          l_Display.pixel_scale(Math::Vector2(50.0f, 50.0f));
          l_Display.layer(2);

          l_Display.set_parent(l_Element.get_display().get_id());

          UI::Component::Image l_Image =
              UI::Component::Image::make(l_ResourceIconElement);

          Core::Texture2D l_Texture =
              Core::Texture2D::make(Util::String("crystal.ktx"));
          l_Image.set_texture(l_Texture);
        }

        // ResourceText
        {
          UI::Element l_ResourceElement =
              UI::Element::make(N(resourcetxt), l_View);
          l_ResourceElement.set_click_passthrough(true);

          UI::Component::Display l_Display =
              UI::Component::Display::make(l_ResourceElement);

          l_Display.pixel_position(Math::Vector2(20.0f, 15.0f));
          l_Display.rotation(0.0f);
          l_Display.pixel_scale(Math::Vector2(30.0f, 40.0f));
          l_Display.layer(3);

          l_Display.set_parent(l_Element.get_display().get_id());

          UI::Component::Text l_Text =
              UI::Component::Text::make(l_ResourceElement);
          l_Text.set_font(l_Font);
          l_Text.set_text(Util::String("6"));
          l_Text.set_color(Math::Color(0.2f, 0.2f, 0.2f, 1.0f));
          l_Text.set_size(0.7f);
        }

        // Title
        {
          UI::Element l_TitleElement =
              UI::Element::make(N(cardtitle), l_View);
          l_TitleElement.set_click_passthrough(true);

          UI::Component::Display l_Display =
              UI::Component::Display::make(l_TitleElement);

          l_Display.pixel_position(Math::Vector2(60.0f, 10.0f));
          l_Display.rotation(0.0f);
          l_Display.pixel_scale(Math::Vector2(130.0f, 40.0f));
          l_Display.layer(3);

          l_Display.set_parent(l_Element.get_display().get_id());

          UI::Component::Text l_Text =
              UI::Component::Text::make(l_TitleElement);
          l_Text.set_font(l_Font);
          l_Text.set_text(Util::String("Nightcrawler"));
          l_Text.set_color(Math::Color(0.2f, 0.2f, 0.2f, 1.0f));
          l_Text.set_size(0.45f);

          l_Text.set_content_fit_approach(
              UI::Component::TextContentFitOptions::Fit);
        }

        // Description
        {
          UI::Element l_DescriptionElement =
              UI::Element::make(N(carddesc), l_View);
          l_DescriptionElement.set_click_passthrough(true);

          UI::Component::Display l_Display =
              UI::Component::Display::make(l_DescriptionElement);

          l_Display.pixel_position(Math::Vector2(10.0f, 60.0f));
          l_Display.rotation(0.0f);
          l_Display.pixel_scale(Math::Vector2(180.0f, 170.0f));
          l_Display.layer(3);

          l_Display.set_parent(l_Element.get_display().get_id());

          UI::Component::Text l_Text =
              UI::Component::Text::make(l_DescriptionElement);
          l_Text.set_font(l_Font);
          l_Text.set_text(
              Util::String("Send a dark creature out for your "
                           "opponent dealing 12 shadow damage."));
          l_Text.set_color(Math::Color(0.2f, 0.2f, 0.2f, 1.0f));
          l_Text.set_size(0.3f);

          l_Text.set_content_fit_approach(
              UI::Component::TextContentFitOptions::WordWrap);
        }
      }

      static void run()
      {
        const auto l_TimeStep = 1'000'000'000ns / 144;

        auto l_Accumulator = 0ns;
        auto l_SecondAccumulator = 0ns;
        auto l_LastTime = steady_clock::now();

        int l_Fps = 0;

        static u64 l_Frames = 0;

        test_ui();

        while (g_Running) {
          {
            auto i_Now = steady_clock::now();
            l_Accumulator += i_Now - l_LastTime;
            l_LastTime = i_Now;

            if (l_Accumulator < l_TimeStep) {
              TaskScheduler::tick();
              continue;
            }
            LOW_PROFILE_CPU("Core", "Tick");

            l_SecondAccumulator += l_Accumulator;

            l_Fps++;
            l_Frames++;

            float l_DeltaTime =
                duration_cast<duration<float>>(l_Accumulator).count();

            g_Frames++;
            execute_ticks(l_DeltaTime);

            if (l_SecondAccumulator > 1'000'000'000ns) {
              l_SecondAccumulator = 0ns;
              g_LastFps = l_Fps;
              l_Fps = 0;
            }

            l_Accumulator = 0ns;

            if (!Renderer::window_is_open()) {
              stop();
              continue;
            }

            execute_late_ticks(l_DeltaTime);
          }

          Util::Profiler::flip();
        }
      }

      void initialize()
      {
        System::Physics::initialize();
      }

      void start()
      {
        g_Running = true;
        run();
      }

      void stop()
      {
        g_Running = false;
      }

      void cleanup()
      {
      }

      void register_tick_callback(System::TickCallback p_Callback)
      {
        g_TickCallbacks.push_back(p_Callback);
      }

      uint32_t get_fps()
      {
        return g_LastFps;
      }
    } // namespace GameLoop
  }   // namespace Core
} // namespace Low
