class Vector2 {
    Vecotr2(){}
    Vector2(float x, float y){}

    float x;
    float y;

    Vector2 opAdd(const Vector2& other) const {return Vector2();}
    Vector2 opSub(const Vector2& other) const {return Vector2();}
    Vector2 opMul(const float& other) const {return Vector2();}
    Vector2 opDiv(const float& other) const {return Vector2();}

    bool opEquals(const Vector2& other) const {return true;}
};

class Vector3 {
    Vecotr3(){}
    Vector3(float x, float y, float z){}

    float get_magnitude()const property {return 0.0f;}
    Vector3 get_normalized()const property{return Vector3();}
    
    float x;
    float y;
    float z;

    Vector3 opAdd(const Vector3& other) const {return Vector3();}
    Vector3 opSub(const Vector3& other) const {return Vector3();}
    Vector3 opMul(const float& other) const {return Vector3();}
    Vector3 opDiv(const float& other) const {return Vector3();}

    bool opEquals(const Vector3& other) const {return true;}
};

class Vector4 {
    Vecotr4(){}
    Vector4(float x, float y, float z, float w){}

    float x;
    float y;
    float z;
    float w;

    Vector4 opAdd(const Vector4& other) const {return Vector4();}
    Vector4 opSub(const Vector4& other) const {return Vector4();}
    Vector4 opMul(const float& other) const {return Vector4();}
    Vector4 opDiv(const float& other) const {return Vector4();}

    bool opEquals(const Vector4& other) const {return true;}
};

namespace Runtime {
    float get_delta_time()const property {return 0.0f;}
}

interface GameplaySystem {
    void tick(const float deltaTime);
};

interface UiController {
    void on_click(UI::Element element);
};