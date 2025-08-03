#pragma once

#include "Core/CObject.h"
#include "Containers/CString.h"

#include <cmath>
#include <cfloat>
#include <random>

namespace NLib
{

/**
 * @brief 数学常量和基础函数库
 */
class LIBNLIB_API NMath
{
public:
    // 数学常量
    static constexpr float PI = 3.14159265358979323846f;
    static constexpr float TWO_PI = 2.0f * PI;
    static constexpr float HALF_PI = 0.5f * PI;
    static constexpr float INV_PI = 1.0f / PI;
    static constexpr float INV_TWO_PI = 1.0f / TWO_PI;
    
    static constexpr float E = 2.71828182845904523536f;
    static constexpr float SQRT_2 = 1.41421356237309504880f;
    static constexpr float SQRT_3 = 1.73205080756887729352f;
    static constexpr float INV_SQRT_2 = 1.0f / SQRT_2;
    static constexpr float INV_SQRT_3 = 1.0f / SQRT_3;
    
    // 精度常量
    static constexpr float EPSILON = FLT_EPSILON;
    static constexpr float SMALL_NUMBER = 1e-8f;
    static constexpr float KINDA_SMALL_NUMBER = 1e-4f;
    static constexpr float BIG_NUMBER = 3.4e+38f;
    
    // 角度转换
    static constexpr float DEG_TO_RAD = PI / 180.0f;
    static constexpr float RAD_TO_DEG = 180.0f / PI;
    
    // 基础数学函数
    static float Sin(float X);
    static float Cos(float X);
    static float Tan(float X);
    static float Asin(float X);
    static float Acos(float X);
    static float Atan(float X);
    static float Atan2(float Y, float X);
    
    static float Sinh(float X);
    static float Cosh(float X);
    static float Tanh(float X);
    
    static float Sqrt(float X);
    static float InvSqrt(float X);
    static float Pow(float Base, float Exp);
    static float Exp(float X);
    static float Exp2(float X);
    static float Log(float X);
    static float Log2(float X);
    static float Log10(float X);
    
    // 取整函数
    static float Floor(float X);
    static float Ceil(float X);
    static float Round(float X);
    static float Trunc(float X);
    static float Frac(float X);
    static float Modf(float X, float& IntPart);
    
    // 符号和绝对值
    static float Abs(float X);
    static int32_t Sign(float X);
    static float CopySign(float X, float Y);
    
    // 比较函数
    static float Min(float A, float B);
    static float Max(float A, float B);
    static float Min3(float A, float B, float C);
    static float Max3(float A, float B, float C);
    
    template<typename TType>
    static TType Min(TType A, TType B);
    
    template<typename TType>
    static TType Max(TType A, TType B);
    
    template<typename TType>
    static TType Clamp(TType Value, TType MinValue, TType MaxValue);
    
    // 插值函数
    static float Lerp(float A, float B, float Alpha);
    static float LerpClamped(float A, float B, float Alpha);
    static float InverseLerp(float A, float B, float Value);
    static float SmoothStep(float A, float B, float X);
    static float SmootherStep(float A, float B, float X);
    
    // 角度函数
    static float DegreesToRadians(float Degrees);
    static float RadiansToDegrees(float Radians);
    static float NormalizeAngle(float AngleRadians);
    static float NormalizeAngleDegrees(float AngleDegrees);
    static float AngleDifference(float A, float B);
    static float LerpAngle(float A, float B, float Alpha);
    
    // 浮点数比较
    static bool IsNearlyEqual(float A, float B, float Tolerance = EPSILON);
    static bool IsNearlyZero(float Value, float Tolerance = SMALL_NUMBER);
    static bool IsFinite(float Value);
    static bool IsNaN(float Value);
    static bool IsInfinite(float Value);
    
    // 随机数生成
    static void SetRandomSeed(uint32_t Seed);
    static float Random(); // [0, 1)
    static float RandomRange(float Min, float Max);
    static int32_t RandomInt(int32_t Min, int32_t Max);
    static bool RandomBool();
    
    // 位操作
    static bool IsPowerOfTwo(uint32_t Value);
    static uint32_t NextPowerOfTwo(uint32_t Value);
    static uint32_t PrevPowerOfTwo(uint32_t Value);
    static int32_t CountLeadingZeros(uint32_t Value);
    static int32_t CountTrailingZeros(uint32_t Value);
    static int32_t PopulationCount(uint32_t Value);
    
    // 快速数学近似
    static float FastSin(float X);
    static float FastCos(float X);
    static float FastTan(float X);
    static float FastAsin(float X);
    static float FastAcos(float X);
    static float FastInvSqrt(float X);

private:
    static std::mt19937 RandomGenerator;
    static std::uniform_real_distribution<float> UniformDistribution;
    
    NMath() = delete; // 静态类
};

/**
 * @brief 2D向量
 */
class LIBNLIB_API NVector2 : public CObject
{
    NCLASS()
class NVector2 : public NObject
{
    GENERATED_BODY()

public:
    float X, Y;
    
    // 构造函数
    NVector2();
    NVector2(float InX, float InY);
    explicit NVector2(float Scalar);
    NVector2(const NVector2& Other);
    NVector2& operator=(const NVector2& Other);
    
    // 访问器
    float& operator[](int32_t Index);
    const float& operator[](int32_t Index) const;
    
    // 基础运算
    NVector2 operator+(const NVector2& Other) const;
    NVector2 operator-(const NVector2& Other) const;
    NVector2 operator*(float Scalar) const;
    NVector2 operator*(const NVector2& Other) const;
    NVector2 operator/(float Scalar) const;
    NVector2 operator/(const NVector2& Other) const;
    NVector2 operator-() const;
    
    NVector2& operator+=(const NVector2& Other);
    NVector2& operator-=(const NVector2& Other);
    NVector2& operator*=(float Scalar);
    NVector2& operator*=(const NVector2& Other);
    NVector2& operator/=(float Scalar);
    NVector2& operator/=(const NVector2& Other);
    
    // 比较运算
    bool operator==(const NVector2& Other) const;
    bool operator!=(const NVector2& Other) const;
    
    // 向量运算
    float Dot(const NVector2& Other) const;
    float Cross(const NVector2& Other) const; // 2D叉积返回标量
    float Length() const;
    float LengthSquared() const;
    float Distance(const NVector2& Other) const;
    float DistanceSquared(const NVector2& Other) const;
    
    NVector2 Normalize() const;
    NVector2& NormalizeSelf();
    bool IsNormalized() const;
    bool IsNearlyZero(float Tolerance = NMath::SMALL_NUMBER) const;
    bool IsZero() const;
    
    // 特殊向量运算
    NVector2 Perpendicular() const; // 逆时针垂直向量
    NVector2 Reflect(const NVector2& Normal) const;
    NVector2 Project(const NVector2& OnVector) const;
    NVector2 Reject(const NVector2& OnVector) const;
    
    // 角度相关
    float Angle() const; // 与X轴的角度
    float AngleTo(const NVector2& Other) const;
    NVector2 Rotate(float AngleRadians) const;
    NVector2 RotateAround(const NVector2& Center, float AngleRadians) const;
    
    // 插值
    static NVector2 Lerp(const NVector2& A, const NVector2& B, float Alpha);
    static NVector2 Slerp(const NVector2& A, const NVector2& B, float Alpha);
    
    // 静态向量
    static const NVector2 Zero;
    static const NVector2 One;
    static const NVector2 UnitX;
    static const NVector2 UnitY;
    static const NVector2 Up;
    static const NVector2 Down;
    static const NVector2 Left;
    static const NVector2 Right;
    
    // 工厂方法
    static NVector2 FromAngle(float AngleRadians);
    static NVector2 FromAngleDegrees(float AngleDegrees);
    static NVector2 Random();
    static NVector2 RandomUnit();
    
    virtual CString ToString() const override;
};

/**
 * @brief 3D向量
 */
class LIBNLIB_API NVector3 : public CObject
{
    NCLASS()
class NVector3 : public NObject
{
    GENERATED_BODY()

public:
    float X, Y, Z;
    
    // 构造函数
    NVector3();
    NVector3(float InX, float InY, float InZ);
    explicit NVector3(float Scalar);
    NVector3(const NVector2& XY, float InZ);
    NVector3(const NVector3& Other);
    NVector3& operator=(const NVector3& Other);
    
    // 访问器
    float& operator[](int32_t Index);
    const float& operator[](int32_t Index) const;
    
    // 基础运算
    NVector3 operator+(const NVector3& Other) const;
    NVector3 operator-(const NVector3& Other) const;
    NVector3 operator*(float Scalar) const;
    NVector3 operator*(const NVector3& Other) const;
    NVector3 operator/(float Scalar) const;
    NVector3 operator/(const NVector3& Other) const;
    NVector3 operator-() const;
    
    NVector3& operator+=(const NVector3& Other);
    NVector3& operator-=(const NVector3& Other);
    NVector3& operator*=(float Scalar);
    NVector3& operator*=(const NVector3& Other);
    NVector3& operator/=(float Scalar);
    NVector3& operator/=(const NVector3& Other);
    
    // 比较运算
    bool operator==(const NVector3& Other) const;
    bool operator!=(const NVector3& Other) const;
    
    // 向量运算
    float Dot(const NVector3& Other) const;
    NVector3 Cross(const NVector3& Other) const;
    float Length() const;
    float LengthSquared() const;
    float Distance(const NVector3& Other) const;
    float DistanceSquared(const NVector3& Other) const;
    
    NVector3 Normalize() const;
    NVector3& NormalizeSelf();
    bool IsNormalized() const;
    bool IsNearlyZero(float Tolerance = NMath::SMALL_NUMBER) const;
    bool IsZero() const;
    
    // 特殊向量运算
    NVector3 Reflect(const NVector3& Normal) const;
    NVector3 Project(const NVector3& OnVector) const;
    NVector3 Reject(const NVector3& OnVector) const;
    NVector3 ProjectOnPlane(const NVector3& PlaneNormal) const;
    
    // 坐标系转换
    NVector2 ToVector2() const; // 丢弃Z分量
    NVector2 ToVector2XZ() const; // 使用X和Z分量
    
    // 插值
    static NVector3 Lerp(const NVector3& A, const NVector3& B, float Alpha);
    static NVector3 Slerp(const NVector3& A, const NVector3& B, float Alpha);
    
    // 静态向量
    static const NVector3 Zero;
    static const NVector3 One;
    static const NVector3 UnitX;
    static const NVector3 UnitY;
    static const NVector3 UnitZ;
    static const NVector3 Forward;
    static const NVector3 Right;
    static const NVector3 Up;
    
    // 工厂方法
    static NVector3 Random();
    static NVector3 RandomUnit();
    static NVector3 RandomInSphere();
    static NVector3 RandomOnSphere();
    
    virtual CString ToString() const override;
};

/**
 * @brief 4D向量
 */
class LIBNLIB_API NVector4 : public CObject
{
    NCLASS()
class NVector4 : public NObject
{
    GENERATED_BODY()

public:
    float X, Y, Z, W;
    
    // 构造函数
    NVector4();
    NVector4(float InX, float InY, float InZ, float InW);
    explicit NVector4(float Scalar);
    NVector4(const NVector3& XYZ, float InW);
    NVector4(const NVector2& XY, const NVector2& ZW);
    NVector4(const NVector4& Other);
    NVector4& operator=(const NVector4& Other);
    
    // 访问器
    float& operator[](int32_t Index);
    const float& operator[](int32_t Index) const;
    
    // 基础运算
    NVector4 operator+(const NVector4& Other) const;
    NVector4 operator-(const NVector4& Other) const;
    NVector4 operator*(float Scalar) const;
    NVector4 operator*(const NVector4& Other) const;
    NVector4 operator/(float Scalar) const;
    NVector4 operator/(const NVector4& Other) const;
    NVector4 operator-() const;
    
    NVector4& operator+=(const NVector4& Other);
    NVector4& operator-=(const NVector4& Other);
    NVector4& operator*=(float Scalar);
    NVector4& operator*=(const NVector4& Other);
    NVector4& operator/=(float Scalar);
    NVector4& operator/=(const NVector4& Other);
    
    // 比较运算
    bool operator==(const NVector4& Other) const;
    bool operator!=(const NVector4& Other) const;
    
    // 向量运算
    float Dot(const NVector4& Other) const;
    float Length() const;
    float LengthSquared() const;
    float Distance(const NVector4& Other) const;
    float DistanceSquared(const NVector4& Other) const;
    
    NVector4 Normalize() const;
    NVector4& NormalizeSelf();
    bool IsNormalized() const;
    bool IsNearlyZero(float Tolerance = NMath::SMALL_NUMBER) const;
    bool IsZero() const;
    
    // 坐标系转换
    NVector3 ToVector3() const; // 丢弃W分量
    NVector3 ToVector3Homogeneous() const; // 齐次坐标转换 (X/W, Y/W, Z/W)
    NVector2 ToVector2() const; // 丢弃Z和W分量
    
    // 插值
    static NVector4 Lerp(const NVector4& A, const NVector4& B, float Alpha);
    
    // 静态向量
    static const NVector4 Zero;
    static const NVector4 One;
    static const NVector4 UnitX;
    static const NVector4 UnitY;
    static const NVector4 UnitZ;
    static const NVector4 UnitW;
    
    virtual CString ToString() const override;
};

// 全局运算符重载
LIBNLIB_API NVector2 operator*(float Scalar, const NVector2& Vector);
LIBNLIB_API NVector3 operator*(float Scalar, const NVector3& Vector);
LIBNLIB_API NVector4 operator*(float Scalar, const NVector4& Vector);

// 模板实现
template<typename TType>
TType NMath::Min(TType A, TType B)
{
    return (A < B) ? A : B;
}

template<typename TType>
TType NMath::Max(TType A, TType B)
{
    return (A > B) ? A : B;
}

template<typename TType>
TType NMath::Clamp(TType Value, TType MinValue, TType MaxValue)
{
    return Max(MinValue, Min(MaxValue, Value));
}

} // namespace NLib