using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Low
{
    public class Log
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Info(string p_String);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Warning(string p_String);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Error(string p_String);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Debug(string p_String);
    }

}