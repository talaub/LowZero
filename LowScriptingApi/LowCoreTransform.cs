using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Low.Component {
public
  unsafe partial class Transform : Low.Internal.Handle
  {

  public
    Transform() : this(0)
    {
    }

  public
    Transform(ulong p_Id) : base(p_Id)
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
    static Transform GetByIndex(uint p_Index)
    {
      ulong id = Low.Internal.HandleHelper.GetLivingInstance(type, p_Index);
      return new Transform(id);
    }
  public
    Low.Vector3 position
    {
      get
      {
        return Low.Internal.HandleHelper.GetVector3Value(id, 1177347317);
      }
      set
      {
        Low.Internal.HandleHelper.SetVector3Value(id, 1177347317, value);
      }
    }
  public
    Low.Quaternion rotation
    {
      get
      {
        return Low.Internal.HandleHelper.GetQuaternionValue(id, 696031473);
      }
      set
      {
        Low.Internal.HandleHelper.SetQuaternionValue(id, 696031473, value);
      }
    }
  public
    Low.Vector3 scale
    {
      get
      {
        return Low.Internal.HandleHelper.GetVector3Value(id, 3964020100);
      }
      set
      {
        Low.Internal.HandleHelper.SetVector3Value(id, 3964020100, value);
      }
    }
  public
    Low.Vector3 worldPosition
    {
      get
      {
        return Low.Internal.HandleHelper.GetVector3Value(id, 3016517004);
      }
    }
  public
    Low.Quaternion worldRotation
    {
      get
      {
        return Low.Internal.HandleHelper.GetQuaternionValue(id, 3701217672);
      }
    }
  public
    Low.Vector3 worldScale
    {
      get
      {
        return Low.Internal.HandleHelper.GetVector3Value(id, 1831270283);
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
