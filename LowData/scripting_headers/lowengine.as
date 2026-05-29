class Vector2 {
    Vecotr2(){}
    Vector2(float value){}
    Vector2(float x, float y){}

    float x;
    float y;

    Vector2 opAdd(const Vector2& other) const {return Vector2();}
    Vector2& opAddAssign(const Vector2& other) {return this;}
    Vector2 opSub(const Vector2& other) const {return Vector2();}
    Vector2 opMul(const float& other) const {return Vector2();}
    Vector2 opDiv(const float& other) const {return Vector2();}

    bool opEquals(const Vector2& other) const {return true;}
};

class Vector3 {
    Vecotr3(){}
    Vector3(float value){}
    Vector3(float x, float y, float z){}
    static Vector3 lerp(const Vector3& from, const Vector3& to, float delta) {return Vector3();}
    static Vector3 get_forward() property {return Vector3(0, 0, -1);}
    static Vector3 get_back() property {return Vector3(0, 0, 1);}
    static Vector3 get_up() property {return Vector3(0, 1, 0);}
    static Vector3 get_down() property {return Vector3(0, -1, 0);}
    static Vector3 get_right() property {return Vector3(1, 0, 0);}
    static Vector3 get_left() property {return Vector3(-1, 0, 0);}

    float get_magnitude()const property {return 0.0f;}
    Vector3 get_normalized()const property{return Vector3();}
    
    float x;
    float y;
    float z;

    Vector3 opAdd(const Vector3& other) const {return Vector3();}
    Vector3& opAddAssign(const Vector3& other) {return this;}
    Vector3 opSub(const Vector3& other) const {return Vector3();}
    Vector3 opMul(const float& other) const {return Vector3();}
    Vector3 opDiv(const float& other) const {return Vector3();}

    bool opEquals(const Vector3& other) const {return true;}
};

class Vector4 {
    Vecotr4(){}
    Vector4(float value){}
    Vector4(float x, float y, float z, float w){}

    float x;
    float y;
    float z;
    float w;

    Vector4 opAdd(const Vector4& other) const {return Vector4();}
    Vector4& opAddAssign(const Vector4& other) {return this;}
    Vector4 opSub(const Vector4& other) const {return Vector4();}
    Vector4 opMul(const float& other) const {return Vector4();}
    Vector4 opDiv(const float& other) const {return Vector4();}

    bool opEquals(const Vector4& other) const {return true;}
};

class Quaternion {
    Quaternion(){}
    Quaternion(float x, float y, float z, float w){}
    static Quaternion from_direction(const Vector3& direction, const Vector3& up) {return Quaternion();}
    static Quaternion slerp(const Quaternion& from, const Quaternion& to, float t) {return Quaternion();}

    float x;
    float y;
    float z;
    float w;
};

namespace Runtime {
    enum EngineState {
        Editing,
        Playing
    }

    float get_delta_time()const property {return 0.0f;}
    EngineState get_state()const property {return EngineState::Editing;}
    bool get_is_playing()const property {return false;}
}

namespace Input {
    enum KeyboardButton {
        Q,
        W,
        E,
        R,
        T,
        Y,
        U,
        I,
        O,
        P,
        A,
        S,
        D,
        F,
        G,
        H,
        J,
        K,
        L,
        Z,
        X,
        C,
        V,
        B,
        N,
        M
    }

    bool is_key_down(KeyboardButton button) {return false;}
}

interface GameplaySystem {
    void tick(const float deltaTime);
};

interface UiController {
    void on_click(UI::Element element);
};
