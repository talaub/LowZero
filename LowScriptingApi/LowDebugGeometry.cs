using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Low
{
    public class Debug
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void DrawSphere(Vector3 p_Position, float p_Radius, Vector3 p_Color, bool p_DepthTested, bool p_Wireframe);
        public static void DrawSphere(Vector3 p_Position, float p_Radius, Vector3 p_Color)
        {
            DrawSphere(p_Position, p_Radius, p_Color, true, false);
        }
    }
}