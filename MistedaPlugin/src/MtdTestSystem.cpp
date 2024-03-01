#include "MtdTestSystem.h"

#include "LowCorePhysics.h"
#include "LowCoreDebugGeometry.h"
#include "LowCoreNavmeshAgent.h"

#include "LowUtilLogger.h"
#include "LowUtilGlobals.h"

#include "LowMath.h"

#include "LowRenderer.h"

#include "imgui.h"

namespace Mtd {
  namespace System {
    namespace Test {
      void tick(float p_Delta, Low::Util::EngineState p_State)
      {
        /*
        Low::Core::DebugGeometry::render_triangle(
            Low::Math::Vector3(-6.2f, 1.0f, -0.09f),
            Low::Math::Vector3(7.65f, 1.0f, -0.09f),
            Low::Math::Vector3(0.76f, 1.0f, -7.14f),
            Low::Math::Color(1.0f, 0.0f, 0.0f, 1.0f), true, false);
            */

        if (p_State != Low::Util::EngineState::PLAYING) {
          return;
        }

        Low::Renderer::RenderFlow l_RenderFlow =
            Low::Renderer::get_main_renderflow();

        Low::Math::UVector2 l_ScreenOffset = G(LOW_SCREEN_OFFSET);

        ImVec2 l_ImGuiMousePosition = ImGui::GetMousePos();
        Low::Math::UVector2 l_WindowMousePos = {
            l_ImGuiMousePosition.x - l_ScreenOffset.x,
            l_ImGuiMousePosition.y - l_ScreenOffset.y};

        Low::Math::UVector2 l_Dimensions =
            l_RenderFlow.get_dimensions();

        if (l_WindowMousePos.x < 0 ||
            l_WindowMousePos.x > l_Dimensions.x ||
            l_WindowMousePos.y < 0 ||
            l_WindowMousePos.y > l_Dimensions.y) {
          return;
        }

        Low::Math::Vector2 l_RelativePosition;
        l_RelativePosition.x =
            ((float)l_WindowMousePos.x) / ((float)l_Dimensions.x);
        l_RelativePosition.y =
            ((float)l_WindowMousePos.y) / ((float)l_Dimensions.y);

        Low::Math::Vector2 l_NDC =
            (l_RelativePosition - Low::Math::Vector2(0.5f)) * 2.0f;

        float ndcX = l_NDC.x;
        float ndcY = l_NDC.y;

        glm::vec4 clipCoords = glm::vec4(ndcX, ndcY, -1.0f, 1.0f);

        glm::mat4 projectionMatrix =
            l_RenderFlow.get_projection_matrix();
        glm::mat4 viewMatrix = l_RenderFlow.get_view_matrix();

        // Calculate the eye space coordinates
        glm::mat4 inverseProjection = glm::inverse(
            projectionMatrix); // Replace with your projection matrix
        glm::vec4 eyeCoords = inverseProjection * clipCoords;
        eyeCoords = glm::vec4(eyeCoords.x, eyeCoords.y, -1.0f, 0.0f);

        // Calculate the world space coordinates
        glm::mat4 inverseView =
            glm::inverse(viewMatrix); // Replace with your view matrix
        glm::vec4 rayWorld = inverseView * eyeCoords;
        glm::vec3 rayDirection = glm::normalize(glm::vec3(rayWorld));

        glm::vec3 l_RayStartCamera =
            l_RenderFlow.get_camera_position();
        glm::vec3 l_RayDir = rayDirection;

        Low::Core::Physics::RaycastHit l_Hit;

        if (Low::Core::Physics::raycast(l_RayStartCamera, l_RayDir,
                                        1000.0f, l_Hit)) {
          Low::Math::Sphere l_Sphere;
          l_Sphere.position = l_Hit.position;
          l_Sphere.radius = 0.2f;

          Low::Core::DebugGeometry::render_sphere(
              l_Sphere, Low::Math::Color(1.0f, 0.0f, 0.0f, 1.0f),
              true, false);

          if (Low::Renderer::get_window().mouse_button_down(
                  Low::Util::MouseButton::LEFT)) {
            Low::Core::Component::NavmeshAgent::ms_LivingInstances[0]
                .set_target_position(l_Hit.position);
          }
        }
      }
    } // namespace Test
  }   // namespace System
} // namespace Mtd
