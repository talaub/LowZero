using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Low.Component {
public
  unsafe partial class DirectionalLight : Low.Internal.Handle
  {

  public
    DirectionalLight() : this(0)
    {
    }

  public
    DirectionalLight(ulong p_Id) : base(p_Id)
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
    static DirectionalLight GetByIndex(uint p_Index)
    {
      ulong id = Low.Internal.HandleHelper.GetLivingInstance(type, p_Index);
      return new DirectionalLight(id);
    }
  public
    Low.Vector3 color
    {
      get
      {
        return Low.Internal.HandleHelper.GetVector3Value(id, 1716930793);
      }
      set
      {
        Low.Internal.HandleHelper.SetVector3Value(id, 1716930793, value);
      }
    }
  public
    float intensity
    {
      get
      {
        return Low.Internal.HandleHelper.GetFloatValue(id, 3920803986);
      }
      set
      {
        Low.Internal.HandleHelper.SetFloatValue(id, 3920803986, value);
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
