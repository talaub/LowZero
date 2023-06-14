using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Low.Internal
{
    public class Handle
    {
        protected
          ulong _id;

        public
          ulong id
        {
            get
            {
                return _id;
            }
        }

        public Handle() : this(0)
        {
        }
        public Handle(ulong id)
        {
            _id = id;
        }

        public static T GetHandleValue<T>(ulong p_Id, ulong p_Offset) where T : Handle, new()
        {
            ulong id = HandleHelper.GetUlongValue(p_Id, p_Offset);

            return CreateHandle<T>(id);
        }

        public static void SetHandleValue<T>(ulong p_Id, ulong p_Offset, T p_Value) where T : Handle
        {
            HandleHelper.SetUlongValue(p_Id, p_Offset, p_Value._id);
        }

        public static T CreateHandle<T>(ulong p_Id) where T : Handle, new()
        {
            T e = new T();
            e._id = p_Id;

            return e;
        }
    }

    public class HandleHelper
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern uint GetLivingInstancesCount(ushort p_TypeId);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern ulong GetLivingInstance(ushort p_TypeId, uint p_Index);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern ulong GetUlongValue(ulong p_Id, ulong p_Offset);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetUlongValue(ulong p_Id, ulong p_Offset, ulong p_Value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern float GetFloatValue(ulong p_Id, uint p_NameHash);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetFloatValue(ulong p_Id, ulong p_Offset, float p_Value);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetVector3Value(ulong p_Id, ulong p_Offset, Vector3 p_Value);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern Vector3 GetVector3Value(ulong p_Id, ulong p_Offset);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetQuaternionValue(ulong p_Id, ulong p_Offset, Quaternion p_Value);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern Quaternion GetQuaternionValue(ulong p_Id, ulong p_Offset);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetNameValue(ulong p_Id, ulong p_Offset, string p_Value);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern string GetNameValue(ulong p_Id, ulong p_Offset);
    }
}
