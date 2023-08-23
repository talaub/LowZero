using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Low
{
    public
      unsafe partial class Entity : Low.Internal.Handle
    {

        public
          Entity() : this(0)
        {
        }

        public
          Entity(ulong p_Id) : base(p_Id)
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
          static Entity GetByIndex(uint p_Index)
        {
            ulong id = Low.Internal.HandleHelper.GetLivingInstance(type, p_Index);
            return new Entity(id);
        }
        public
          string name
        {
            get
            {
                return Low.Internal.HandleHelper.GetNameValue(id, 1579384326);
            }
            set
            {
                Low.Internal.HandleHelper.SetNameValue(id, 1579384326, value);
            }
        } // namespace Low

        // LOW_CODEGEN:BEGIN:CUSTOM:TYPE_CODE

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern ulong
        GetComponent(ulong p_HandleId, ushort p_TypeId);

        public
        T GetComponent<T>() where T : Internal.Handle, new()
        {
            System.Reflection.FieldInfo fieldInfo =
                typeof(T).GetField("type", System.Reflection.BindingFlags.Public |
                                                   System.Reflection.BindingFlags.Static);

            ushort typeId = (ushort)fieldInfo.GetValue(null);
            ulong l_Id = GetComponent(id, typeId);
            return CreateHandle<T>(l_Id);
        }
        // LOW_CODEGEN::END::CUSTOM:TYPE_CODE
    }
}
