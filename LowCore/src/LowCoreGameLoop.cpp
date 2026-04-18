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

#include "LowCoreInput.h"
#include "LowCoreScriptAsset.h"

#include "LowCoreUiElement.h"
#include "LowCoreUiDisplay.h"
#include "LowCoreUiImage.h"
#include "LowCoreUiText.h"
#include "LowCoreUiDisplaySystem.h"
#include "LowCoreUiImageSystem.h"
#include "LowCoreUiTextSystem.h"
#include "LowCoreUiViewSystem.h"
#include "LowCoreUiWidgetAsset.h"

#include "LowCoreScripting.h"

#include <chrono>

#include "LowMath.h"
#include "LowRendererPrimitives.h"
#include "LowRendererUiRenderObject.h"
#include "LowUtilEnums.h"
#include "LowUtilLogger.h"
#include "LowUtilContainers.h"
#include "LowUtilJobManager.h"
#include "LowUtilProfiler.h"
#include "LowUtil.h"

#include "LowRenderer.h"

#include "LowRendererUiCanvas.h"
#include "LowRendererResourceManager.h"
#include "LowRendererResourceImporter.h"

namespace Low {
  namespace Core {
    namespace GameLoop {
      using namespace std::chrono;

      float g_TestCounter = 0.0f;

      float g_DeltaTime = 0.0f;

      bool g_Running = false;
      uint32_t g_LastFps = 0u;

      uint64_t g_Frames = 0ull;

      Util::List<System::TickCallback> g_TickCallbacks;

      // FIX: Remove test code
      Low::Renderer::UiRenderObject g_FontRenderObject;

      static void execute_ticks(float p_Delta)
      {
        if (g_Frames == 180) {
          // Scripting::test_as();
        }
        static bool l_FirstRun = true;

        Util::tick(p_Delta);

        if (Util::Window::get_main_window().minimized) {
          return;
        }

        Renderer::check_window_resize(p_Delta);

        if (l_FirstRun) {
          ScriptAsset::initialize();
          Scripting::initialize_as();
        }

        Renderer::prepare_tick(p_Delta);
        System::Transform::tick(p_Delta, get_engine_state());
        UI::System::View::tick(p_Delta, get_engine_state());
        UI::System::Display::tick(p_Delta, get_engine_state());
        System::Region::tick(p_Delta, get_engine_state());
        System::Camera::tick(p_Delta, get_engine_state());
        if (!l_FirstRun) {
          System::Physics::tick(p_Delta, get_engine_state());
          // System::Navmesh::tick(p_Delta, get_engine_state());
        }

        Scripting::tick(p_Delta, get_engine_state());

        g_TestCounter += p_Delta;

        if (g_TestCounter > 1.0f) {
          g_TestCounter = 0.0f;

          float param = 1.2f;

          CflatVoidCall(MtdScripts::StatusEffect::Ignite::tick,
                        CflatArg(param));
        }

        for (auto it = g_TickCallbacks.begin();
             it != g_TickCallbacks.end(); ++it) {
          (*it)(p_Delta, get_engine_state());
        }

        if (get_engine_state() == Util::EngineState::PLAYING) {
          Util::String l_TickFunctionName =
              get_current_gamemode().get_tick_function_name();
          Cflat::Identifier l_FunctionNameCflat(
              l_TickFunctionName.c_str());

          /*
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
          */
        }

        System::Light::tick(p_Delta, get_engine_state());
        System::MeshRenderer::tick(p_Delta, get_engine_state());
        UI::System::Image::tick(p_Delta, get_engine_state());
        UI::System::Text::tick(p_Delta, get_engine_state());

        Scripting::tick_as(p_Delta);

        l_FirstRun = false;
      }

      static void execute_late_ticks(float p_Delta)
      {
        static bool l_FirstRun = true;
        if (!l_FirstRun) {
          System::Physics::late_tick(p_Delta, get_engine_state());
        }
        System::Transform::late_tick(p_Delta, get_engine_state());

        Input::late_tick(p_Delta);
        Renderer::tick(p_Delta);
        l_FirstRun = false;
      }

      static void run()
      {
        const auto l_TimeStep = 1'000'000'000ns / 144;

        auto l_Accumulator = 0ns;
        auto l_SecondAccumulator = 0ns;
        auto l_LastTime = steady_clock::now();

        int l_Fps = 0;

        static u64 l_Frames = 0;

        // FIX: Remove test code
        static bool l_IsInit = false;

        while (g_Running) {
          {
            if (0) {
              // FIX: Remove test code
              Low::Renderer::Font l_Font =
                  Low::Renderer::Font::find_by_name(N(roboto));
              if (!l_IsInit && l_Font.is_alive() &&
                  l_Font.is_fully_loaded()) {
                const char l_Char = 'E';

                Math::Vector4 l_UvRect;
                l_UvRect.x = l_Font.get_glyphs()[l_Char].uvMin.x;
                l_UvRect.y = l_Font.get_glyphs()[l_Char].uvMin.y;
                l_UvRect.z = l_Font.get_glyphs()[l_Char].uvMax.x;
                l_UvRect.w = l_Font.get_glyphs()[l_Char].uvMax.y;

                g_FontRenderObject.set_uv_rect(l_UvRect);

                l_IsInit = true;
              }
            }

            auto i_Now = steady_clock::now();
            l_Accumulator += i_Now - l_LastTime;
            l_LastTime = i_Now;

            if (l_Accumulator < l_TimeStep) {
              TaskScheduler::tick();
              continue;
            } else if (l_Frames % 100 == 0) {
              // TaskScheduler::tick();
            }
            LOW_PROFILE_CPU("Core", "Tick");

            l_SecondAccumulator += l_Accumulator;

            l_Fps++;
            l_Frames++;

            const float l_DeltaTime =
                duration_cast<duration<float>>(l_Accumulator).count();

            g_DeltaTime = l_DeltaTime;

            g_Frames++;
            execute_ticks(l_DeltaTime);

            if (l_SecondAccumulator > 1'000'000'000ns) {
              l_SecondAccumulator = 0ns;
              g_LastFps = l_Fps;
              l_Fps = 0;
            }

            l_Accumulator = 0ns;

            if (Util::Window::get_main_window().shouldClose) {
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

#if 0
        Low::Renderer::ResourceImporter::import_font(
            "D:\\Roboto-Regular.ttf", "roboto");
        return;
#endif

        Renderer::UiCanvas l_Canvas =
            Renderer::UiCanvas::make(N(TestCanvas));
        Renderer::get_game_renderview().add_ui_canvas(l_Canvas);

        if (0) {
          using namespace Low::Renderer;

          MaterialTypes &l_MaterialTypes = get_material_types();

          Material l_UiMaterial =
              Material::make(N(UiMaterial), l_MaterialTypes.uiBase);
          Material l_TextMaterial =
              Material::make(N(TextMaterial), l_MaterialTypes.uiText);

          UI::View l_View = UI::View::make(N(View));
          l_View.set_canvas(l_Canvas);

          if (1) {
            {
              UI::Element l_Element =
                  UI::Element::make(N(Img), l_Canvas);
              UI::Component::Display l_Display =
                  UI::Component::Display::make(l_Element);

              l_Display.pixel_position(120.0f, 100.0f);
              l_Display.pixel_scale(150, 150);
              l_Display.rotation(0);
              l_Display.layer(0);

              UI::Component::Image l_Image =
                  UI::Component::Image::make(l_Element);

              l_Image.set_texture(get_default_texture());
            }

            if (1) {
              UI::Element l_Element =
                  UI::Element::make(N(txt), l_Canvas);
              UI::Component::Display l_Display =
                  UI::Component::Display::make(l_Element);

              l_Display.pixel_position(120.0f, 250.0f);
              l_Display.pixel_scale(300, 150);
              l_Display.rotation(0);
              l_Display.layer(0);

              UI::Component::Text l_Text =
                  UI::Component::Text::make(l_Element);
              l_Text.set_text("Hello World!");
              l_Text.set_size(32.0f);
              Font l_Font = Font::find_by_name(N(roboto));
              l_Text.set_font(l_Font);
              l_Text.set_color(Math::Color(1.0f, 1.0f, 1.0f, 1.0f));
            }
          }

          if (0) {
            UiRenderObject l_RenderObject = UiRenderObject::make(
                l_Canvas, get_primitives().unitQuad);

            l_RenderObject.set_position_x(200.f);
            l_RenderObject.set_position_y(140.f);

            Font l_Font = Font::find_by_name(N(roboto));

            ResourceManager::load_font(l_Font);

            l_RenderObject.set_texture(l_Font.get_texture());

            l_RenderObject.set_rotation2D(0.f);
            l_RenderObject.set_z_sorting(1);

            l_RenderObject.set_size(150.0f, 150.0f);

            l_RenderObject.set_material(l_TextMaterial);

            g_FontRenderObject = l_RenderObject;
          }
        }

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

      float get_delta_time()
      {
        return g_DeltaTime;
      }
    } // namespace GameLoop
  } // namespace Core
} // namespace Low
