#include "FlodeCastNodes.h"

namespace Flode {
  namespace CastNodes {

    void CastNumberToString::setup_default_pins()
    {
      create_pin(PinDirection::Input, "", PinType::Number);

      create_pin(PinDirection::Output, "", PinType::String);
    }

    void CastNumberToString::compile_output_pin(
        Low::Util::StringBuilder &p_Builder,
        NodeEd::PinId p_PinId) const
    {
      Pin *l_Pin = find_output_pin_checked(p_PinId);

      p_Builder.append("Low::Util::String(std::to_string(");
      compile_input_pin(p_Builder, pins[0]->id);
      p_Builder.append(").c_str())");
    }

    Node *castnumbertostring_create_instance()
    {
      return new CastNumberToString;
    }

    void register_nodes()
    {
      {
        Low::Util::Name l_TypeName = N(FlodeCastNumberToString);
        register_node(l_TypeName,
                      &castnumbertostring_create_instance);
        register_cast_node(PinType::Number, PinType::String,
                           N(FlodeCastNumberToString));
      }
    }

  } // namespace CastNodes
} // namespace Flode
