#pragma once

namespace Mtd {
  enum class CombatState
  {
    FIGHTER_1_TURN,
    FIGHTER_2_TURN
  };

  struct CombatDelay
  {
    float remaining;
    CombatState nextState;
  };

  enum class DamageType
  {
    PHYSICAL,
    SHADOW,
    FIRE,
    POISON,
    ARCANE
  };
} // namespace Mtd
