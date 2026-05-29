#include "LowCoreTweenSystem.h"

#include "LowCoreTween.h"
#include "LowCoreTweenEase.h"

#include "LowMath.h"

namespace Low {
  namespace Core {
    namespace System {
      namespace Tween {
        static float clamp_progress(const float p_Progress)
        {
          return Math::Util::clamp(p_Progress, 0.0f, 1.0f);
        }

        static float in_quad(const float p_Progress)
        {
          return p_Progress * p_Progress;
        }

        static float out_quad(const float p_Progress)
        {
          return 1.0f - ((1.0f - p_Progress) *
                         (1.0f - p_Progress));
        }

        static float in_out_quad(const float p_Progress)
        {
          if (p_Progress < 0.5f) {
            return 2.0f * p_Progress * p_Progress;
          }

          const float l_Tail = -2.0f * p_Progress + 2.0f;
          return 1.0f - ((l_Tail * l_Tail) * 0.5f);
        }

        static float in_cubic(const float p_Progress)
        {
          return p_Progress * p_Progress * p_Progress;
        }

        static float out_cubic(const float p_Progress)
        {
          const float l_Tail = 1.0f - p_Progress;
          return 1.0f - (l_Tail * l_Tail * l_Tail);
        }

        static float in_quart(const float p_Progress)
        {
          return p_Progress * p_Progress * p_Progress * p_Progress;
        }

        static float out_quart(const float p_Progress)
        {
          const float l_Tail = 1.0f - p_Progress;
          return 1.0f -
                 (l_Tail * l_Tail * l_Tail * l_Tail);
        }

        static float in_out_quart(const float p_Progress)
        {
          if (p_Progress < 0.5f) {
            return 8.0f * p_Progress * p_Progress *
                   p_Progress * p_Progress;
          }

          const float l_Tail = -2.0f * p_Progress + 2.0f;
          return 1.0f -
                 ((l_Tail * l_Tail * l_Tail * l_Tail) * 0.5f);
        }

        static float out_back(const float p_Progress)
        {
          const float l_C1 = 1.70158f;
          const float l_C3 = l_C1 + 1.0f;
          const float l_Tail = p_Progress - 1.0f;
          return 1.0f + l_C3 * l_Tail * l_Tail * l_Tail +
                 l_C1 * l_Tail * l_Tail;
        }

        static float in_back(const float p_Progress)
        {
          const float l_C1 = 1.70158f;
          const float l_C3 = l_C1 + 1.0f;
          return l_C3 * p_Progress * p_Progress * p_Progress -
                 l_C1 * p_Progress * p_Progress;
        }

        static float in_out_back(const float p_Progress)
        {
          const float l_C1 = 1.70158f;
          const float l_C2 = l_C1 * 1.525f;

          if (p_Progress < 0.5f) {
            const float l_Progress = 2.0f * p_Progress;
            return (l_Progress * l_Progress *
                    ((l_C2 + 1.0f) * l_Progress - l_C2)) *
                   0.5f;
          }

          const float l_Progress = 2.0f * p_Progress - 2.0f;
          return (l_Progress * l_Progress *
                      ((l_C2 + 1.0f) * l_Progress + l_C2) +
                  2.0f) *
                 0.5f;
        }

        float apply_ease(const TweenEase p_Ease,
                         const float p_Progress)
        {
          const float l_Progress = clamp_progress(p_Progress);

          if (p_Ease == TweenEase::LINEAR) {
            return l_Progress;
          }
          if (p_Ease == TweenEase::INQUAD) {
            return in_quad(l_Progress);
          }
          if (p_Ease == TweenEase::OUTQUAD) {
            return out_quad(l_Progress);
          }
          if (p_Ease == TweenEase::INOUTQUAD) {
            return in_out_quad(l_Progress);
          }
          if (p_Ease == TweenEase::INCUBIC) {
            return in_cubic(l_Progress);
          }
          if (p_Ease == TweenEase::OUTCUBIC) {
            return out_cubic(l_Progress);
          }
          if (p_Ease == TweenEase::INOUTCUBIC) {
            return in_out_cubic(l_Progress);
          }
          if (p_Ease == TweenEase::INQUART) {
            return in_quart(l_Progress);
          }
          if (p_Ease == TweenEase::OUTQUART) {
            return out_quart(l_Progress);
          }
          if (p_Ease == TweenEase::INOUTQUART) {
            return in_out_quart(l_Progress);
          }
          if (p_Ease == TweenEase::INBACK) {
            return in_back(l_Progress);
          }
          if (p_Ease == TweenEase::OUTBACK) {
            return out_back(l_Progress);
          }
          if (p_Ease == TweenEase::INOUTBACK) {
            return in_out_back(l_Progress);
          }

          return l_Progress;
        }

        float get_eased_progress(const Core::Tween &p_Tween)
        {
          if (!p_Tween.is_alive()) {
            return 1.0f;
          }

          return apply_ease(p_Tween.get_ease(),
                            p_Tween.get_progress());
        }

        float in_out_cubic(const float p_Progress)
        {
          const float l_Progress = clamp_progress(p_Progress);

          if (l_Progress < 0.5f) {
            return 4.0f * l_Progress * l_Progress * l_Progress;
          }

          const float l_Tail = -2.0f * l_Progress + 2.0f;
          return 1.0f - ((l_Tail * l_Tail * l_Tail) * 0.5f);
        }

        void tick(const float p_Delta,
                  const Util::EngineState p_State)
        {
          (void)p_State;

          Util::List<Core::Tween> l_Tweens =
              Core::Tween::ms_LivingInstances;

          for (Core::Tween &i_Tween : l_Tweens) {
            if (!i_Tween.is_alive() || i_Tween.is_finished()) {
              continue;
            }

            const float l_FullDuration = i_Tween.get_full_duration();
            const float l_CurrentDuration =
                i_Tween.get_current_duration();

            if (l_FullDuration <= 0.0f) {
              i_Tween.set_current_duration(0.0f);
              continue;
            }

            i_Tween.set_current_duration(Math::Util::clamp(
                l_CurrentDuration + p_Delta, 0.0f, l_FullDuration));
          }
        }
      } // namespace Tween
    } // namespace System
  } // namespace Core
} // namespace Low
