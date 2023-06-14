using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Low.Component {
public
  unsafe partial class Rigidbody : Low.Internal.Handle
  {

  public
    Rigidbody() : this(0)
    {
    }

  public
    Rigidbody(ulong p_Id) : base(p_Id)
    {
    }

  public
    static ushort type;

  public
    static uint livingInstancesCount
    {
      get
      {
        return Low.Internal.HandleHelper.GetLivingInstancesCount(type);
      }
    }

  public
    static Rigidbody GetByIndex(uint p_Index)
    {
      ulong id = Low.Internal.HandleHelper.GetLivingInstance(type, p_Index);
      return new Rigidbody(id);
    }
  public
    float mass
    {
      get
      {
        return Low.Internal.HandleHelper.GetFloatValue(id, 1812159334);
      }
      set
      {
        Low.Internal.HandleHelper.SetFloatValue(id, 1812159334, value);
      }
    }
  public
    Low.Entity entity
    {
      get
      {
        return Low.Internal.Handle.GetHandleValue<Low.Entity>(id, 237519976);
      }
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:TYPE_CODE
  }
} // namespace Component
