using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Low
{
    public class Quaternion
    {
        private float _x;
        private float _y;
        private float _z;
        private float _w;

        public float x
        {
            get { return _x; }
            set { _x = value; }
        }
        public float y
        {
            get { return _y; }
            set { _y = value; }
        }
        public float z
        {
            get { return _z; }
            set { _z = value; }
        }

        public float w
        {
            get { return _w; }
            set { _w = value; }
        }

        public Quaternion() : this(1.0f, 0.0f, 0.0f, 0.0f)
        {
        }

        public Quaternion(float w, float x, float y, float z)
        {
            this.x = x;
            this.y = y;
            this.z = z;
            this.w = w;
        }

        public float magnitudeSquared
        {
            get
            {
                return x * x + y + y + z * z + w * w;
            }
        }

        public float magnitude
        {
            get
            {
                return (float)Math.Sqrt(magnitudeSquared);
            }
        }

        public void normalize()
        {
            float length = magnitude;
            x /= length;
            y /= length;
            z /= length;
            w /= length;
        }

        public override string ToString()
        {
            return "Quaternion(" + w + ", " + x + ", " + y + ", " + z + ")";
        }

    }
    public class Vector4
    {
        private float _x;
        private float _y;
        private float _z;
        private float _w;

        public float x
        {
            get { return _x; }
            set { _x = value; }
        }
        public float y
        {
            get { return _y; }
            set { _y = value; }
        }
        public float z
        {
            get { return _z; }
            set { _z = value; }
        }

        public float w
        {
            get { return _w; }
            set { _w = value; }
        }

        public Vector4() : this(0.0f)
        {
        }

        public Vector4(float v) : this(v, v, v, v)
        {
        }

        public Vector4(float x, float y, float z, float w)
        {
            this.x = x;
            this.y = y;
            this.z = z;
            this.w = w;
        }

        public float magnitudeSquared
        {
            get
            {
                return x * x + y + y + z * z + w * w;
            }
        }

        public float magnitude
        {
            get
            {
                return (float)Math.Sqrt(magnitudeSquared);
            }
        }

        public static float DistanceSquared(Vector4 p_From, Vector4 p_To)
        {
            float diffX = p_To.x - p_From.x;
            float diffY = p_To.y - p_From.y;
            float diffZ = p_To.z - p_From.z;
            float diffW = p_To.w - p_From.w;

            return diffX * diffX + diffY * diffY + diffZ * diffZ + diffW * diffW;
        }

        public static float Distance(Vector4 p_From, Vector4 p_To)
        {
            return (float)Math.Sqrt(DistanceSquared(p_From, p_To));
        }

        public void normalize()
        {
            float length = magnitude;
            x /= length;
            y /= length;
            z /= length;
            w /= length;
        }

        public static Vector4 operator +(Vector4 p_Vec) => p_Vec;
        public static Vector4 operator -(Vector4 p_Vec) => new Vector4(-p_Vec.x, -p_Vec.y, -p_Vec.z, -p_Vec.z);
        public static Vector4 operator +(Vector4 p_Vec0, Vector4 p_Vec1)
        {
            return new Vector4(p_Vec0.x + p_Vec1.x, p_Vec0.y + p_Vec1.y, p_Vec0.z + p_Vec1.z, p_Vec0.w + p_Vec1.w);
        }
        public static Vector4 operator -(Vector4 p_Vec0, Vector4 p_Vec1)
        {
            return p_Vec0 + (-p_Vec1);
        }

        public static Vector4 operator *(Vector4 p_Vec, float p_Scalar)
        {
            return new Vector4(p_Vec.x * p_Scalar, p_Vec.y * p_Scalar, p_Vec.z * p_Scalar, p_Vec.w * p_Scalar);
        }
        public static Vector4 operator /(Vector4 p_Vec, float p_Scalar)
        {
            return new Vector4(p_Vec.x / p_Scalar, p_Vec.y / p_Scalar, p_Vec.z / p_Scalar, p_Vec.w / p_Scalar);
        }

        public override string ToString()
        {
            return "Vector4(" + x + ", " + y + ", " + z + ", " + w + ")";
        }

    }

    public class Vector3
    {
        private float _x;
        private float _y;
        private float _z;

        public float x
        {
            get { return _x; }
            set { _x = value; }
        }
        public float y
        {
            get { return _y; }
            set { _y = value; }
        }
        public float z
        {
            get { return _z; }
            set { _z = value; }
        }

        public Vector3() : this(0.0f)
        {
        }

        public Vector3(float v) : this(v, v, v)
        {
        }

        public Vector3(float x, float y, float z)
        {
            this.x = x;
            this.y = y;
            this.z = z;
        }

        public float magnitudeSquared
        {
            get
            {
                return x * x + y + y + z * z;
            }
        }

        public float magnitude
        {
            get
            {
                return (float)Math.Sqrt(magnitudeSquared);
            }
        }

        public static float DistanceSquared(Vector3 p_From, Vector3 p_To)
        {
            float diffX = p_To.x - p_From.x;
            float diffY = p_To.y - p_From.y;
            float diffZ = p_To.z - p_From.z;

            return diffX * diffX + diffY * diffY + diffZ * diffZ;
        }

        public static float Distance(Vector3 p_From, Vector3 p_To)
        {
            return (float)Math.Sqrt(DistanceSquared(p_From, p_To));
        }

        public void normalize()
        {
            float length = magnitude;
            x /= length;
            y /= length;
            z /= length;
        }

        public static Vector3 operator +(Vector3 p_Vec) => p_Vec;
        public static Vector3 operator -(Vector3 p_Vec) => new Vector3(-p_Vec.x, -p_Vec.y, -p_Vec.z);
        public static Vector3 operator +(Vector3 p_Vec0, Vector3 p_Vec1)
        {
            return new Vector3(p_Vec0.x + p_Vec1.x, p_Vec0.y + p_Vec1.y, p_Vec0.z + p_Vec1.z);
        }
        public static Vector3 operator -(Vector3 p_Vec0, Vector3 p_Vec1)
        {
            return p_Vec0 + (-p_Vec1);
        }

        public static Vector3 operator *(Vector3 p_Vec, float p_Scalar)
        {
            return new Vector3(p_Vec.x * p_Scalar, p_Vec.y * p_Scalar, p_Vec.z * p_Scalar);
        }
        public static Vector3 operator /(Vector3 p_Vec, float p_Scalar)
        {
            return new Vector3(p_Vec.x / p_Scalar, p_Vec.y / p_Scalar, p_Vec.z / p_Scalar);
        }

        public override string ToString()
        {
            return "Vector3(" + x + ", " + y + ", " + z + ")";
        }

    }

    public class Vector2
    {
        private float _x;
        private float _y;

        public float x
        {
            get { return _x; }
            set { _x = value; }
        }
        public float y
        {
            get { return _y; }
            set { _y = value; }
        }

        public Vector2() : this(0.0f)
        {
        }

        public Vector2(float v) : this(v, v)
        {
        }

        public Vector2(float x, float y)
        {
            this.x = x;
            this.y = y;
        }

        public float magnitudeSquared
        {
            get
            {
                return x * x + y + y;
            }
        }

        public float magnitude
        {
            get
            {
                return (float)Math.Sqrt(magnitudeSquared);
            }
        }

        public static float DistanceSquared(Vector2 p_From, Vector2 p_To)
        {
            float diffX = p_To.x - p_From.x;
            float diffY = p_To.y - p_From.y;

            return diffX * diffX + diffY * diffY;
        }

        public static float Distance(Vector2 p_From, Vector2 p_To)
        {
            return (float)Math.Sqrt(DistanceSquared(p_From, p_To));
        }

        public void normalize()
        {
            float length = magnitude;
            x /= length;
            y /= length;
        }

        public static Vector2 operator +(Vector2 p_Vec) => p_Vec;
        public static Vector2 operator -(Vector2 p_Vec) => new Vector2(-p_Vec.x, -p_Vec.y);
        public static Vector2 operator +(Vector2 p_Vec0, Vector2 p_Vec1)
        {
            return new Vector2(p_Vec0.x + p_Vec1.x, p_Vec0.y + p_Vec1.y);
        }
        public static Vector2 operator -(Vector2 p_Vec0, Vector2 p_Vec1)
        {
            return p_Vec0 + (-p_Vec1);
        }

        public static Vector2 operator *(Vector2 p_Vec, float p_Scalar)
        {
            return new Vector2(p_Vec.x * p_Scalar, p_Vec.y * p_Scalar);
        }
        public static Vector2 operator /(Vector2 p_Vec, float p_Scalar)
        {
            return new Vector2(p_Vec.x / p_Scalar, p_Vec.y / p_Scalar);
        }

        public override string ToString()
        {
            return "Vector2(" + x + ", " + y + ")";
        }

    }
}
