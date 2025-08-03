#include "Math/NMath.h"
#include <cmath>
#include <random>

namespace NLib
{

// === 数学常量 ===

const float NMath::PI = 3.14159265358979323846f;
const float NMath::TWO_PI = 2.0f * PI;
const float NMath::HALF_PI = 0.5f * PI;
const float NMath::QUARTER_PI = 0.25f * PI;
const float NMath::INV_PI = 1.0f / PI;
const float NMath::INV_TWO_PI = 1.0f / TWO_PI;

const float NMath::E = 2.71828182845904523536f;
const float NMath::SQRT_2 = 1.41421356237309504880f;
const float NMath::SQRT_3 = 1.73205080756887729353f;
const float NMath::INV_SQRT_2 = 1.0f / SQRT_2;
const float NMath::INV_SQRT_3 = 1.0f / SQRT_3;

const float NMath::DEG_TO_RAD = PI / 180.0f;
const float NMath::RAD_TO_DEG = 180.0f / PI;

const float NMath::EPSILON = 1e-6f;
const float NMath::SMALL_NUMBER = 1e-8f;
const float NMath::BIG_NUMBER = 3.4e+38f;

// === 基础数学函数 ===

float NMath::Abs(float Value)
{
    return std::abs(Value);
}

double NMath::Abs(double Value)
{
    return std::abs(Value);
}

int32_t NMath::Abs(int32_t Value)
{
    return std::abs(Value);
}

int64_t NMath::Abs(int64_t Value)
{
    return std::abs(Value);
}

float NMath::Sign(float Value)
{
    if (Value > 0.0f) return 1.0f;
    if (Value < 0.0f) return -1.0f;
    return 0.0f;
}

double NMath::Sign(double Value)
{
    if (Value > 0.0) return 1.0;
    if (Value < 0.0) return -1.0;
    return 0.0;
}

int32_t NMath::Sign(int32_t Value)
{
    if (Value > 0) return 1;
    if (Value < 0) return -1;
    return 0;
}

float NMath::Sqrt(float Value)
{
    return std::sqrt(Value);
}

double NMath::Sqrt(double Value)
{
    return std::sqrt(Value);
}

float NMath::InvSqrt(float Value)
{
    return 1.0f / Sqrt(Value);
}

float NMath::Pow(float Base, float Exponent)
{
    return std::pow(Base, Exponent);
}

double NMath::Pow(double Base, double Exponent)
{
    return std::pow(Base, Exponent);
}

float NMath::Exp(float Value)
{
    return std::exp(Value);
}

double NMath::Exp(double Value)
{
    return std::exp(Value);
}

float NMath::Log(float Value)
{
    return std::log(Value);
}

double NMath::Log(double Value)
{
    return std::log(Value);
}

float NMath::Log10(float Value)
{
    return std::log10(Value);
}

double NMath::Log10(double Value)
{
    return std::log10(Value);
}

float NMath::Log2(float Value)
{
    return std::log2(Value);
}

double NMath::Log2(double Value)
{
    return std::log2(Value);
}

// === 三角函数 ===

float NMath::Sin(float Value)
{
    return std::sin(Value);
}

double NMath::Sin(double Value)
{
    return std::sin(Value);
}

float NMath::Cos(float Value)
{
    return std::cos(Value);
}

double NMath::Cos(double Value)
{
    return std::cos(Value);
}

float NMath::Tan(float Value)
{
    return std::tan(Value);
}

double NMath::Tan(double Value)
{
    return std::tan(Value);
}

float NMath::Asin(float Value)
{
    return std::asin(Value);
}

double NMath::Asin(double Value)
{
    return std::asin(Value);
}

float NMath::Acos(float Value)
{
    return std::acos(Value);
}

double NMath::Acos(double Value)
{
    return std::acos(Value);
}

float NMath::Atan(float Value)
{
    return std::atan(Value);
}

double NMath::Atan(double Value)
{
    return std::atan(Value);
}

float NMath::Atan2(float Y, float X)
{
    return std::atan2(Y, X);
}

double NMath::Atan2(double Y, double X)
{
    return std::atan2(Y, X);
}

void NMath::SinCos(float Value, float& OutSin, float& OutCos)
{
    OutSin = Sin(Value);
    OutCos = Cos(Value);
}

void NMath::SinCos(double Value, double& OutSin, double& OutCos)
{
    OutSin = Sin(Value);
    OutCos = Cos(Value);
}

// === 取整函数 ===

float NMath::Floor(float Value)
{
    return std::floor(Value);
}

double NMath::Floor(double Value)
{
    return std::floor(Value);
}

float NMath::Ceil(float Value)
{
    return std::ceil(Value);
}

double NMath::Ceil(double Value)
{
    return std::ceil(Value);
}

float NMath::Round(float Value)
{
    return std::round(Value);
}

double NMath::Round(double Value)
{
    return std::round(Value);
}

float NMath::Trunc(float Value)
{
    return std::trunc(Value);
}

double NMath::Trunc(double Value)
{
    return std::trunc(Value);
}

float NMath::Frac(float Value)
{
    return Value - Floor(Value);
}

double NMath::Frac(double Value)
{
    return Value - Floor(Value);
}

float NMath::Fmod(float X, float Y)
{
    return std::fmod(X, Y);
}

double NMath::Fmod(double X, double Y)
{
    return std::fmod(X, Y);
}

// === 比较和范围函数 ===

float NMath::Min(float A, float B)
{
    return (A < B) ? A : B;
}

double NMath::Min(double A, double B)
{
    return (A < B) ? A : B;
}

int32_t NMath::Min(int32_t A, int32_t B)
{
    return (A < B) ? A : B;
}

int64_t NMath::Min(int64_t A, int64_t B)
{
    return (A < B) ? A : B;
}

float NMath::Max(float A, float B)
{
    return (A > B) ? A : B;
}

double NMath::Max(double A, double B)
{
    return (A > B) ? A : B;
}

int32_t NMath::Max(int32_t A, int32_t B)
{
    return (A > B) ? A : B;
}

int64_t NMath::Max(int64_t A, int64_t B)
{
    return (A > B) ? A : B;
}

float NMath::Clamp(float Value, float MinVal, float MaxVal)
{
    return Min(Max(Value, MinVal), MaxVal);
}

double NMath::Clamp(double Value, double MinVal, double MaxVal)
{
    return Min(Max(Value, MinVal), MaxVal);
}

int32_t NMath::Clamp(int32_t Value, int32_t MinVal, int32_t MaxVal)
{
    return Min(Max(Value, MinVal), MaxVal);
}

float NMath::Saturate(float Value)
{
    return Clamp(Value, 0.0f, 1.0f);
}

double NMath::Saturate(double Value)
{
    return Clamp(Value, 0.0, 1.0);
}

// === 插值函数 ===

float NMath::Lerp(float A, float B, float Alpha)
{
    return A + Alpha * (B - A);
}

double NMath::Lerp(double A, double B, double Alpha)
{
    return A + Alpha * (B - A);
}

float NMath::LerpAngle(float A, float B, float Alpha)
{
    float Delta = Fmod(B - A, TWO_PI);
    if (Delta > PI)
    {
        Delta -= TWO_PI;
    }
    else if (Delta < -PI)
    {
        Delta += TWO_PI;
    }
    
    return A + Alpha * Delta;
}

float NMath::SmoothStep(float Edge0, float Edge1, float Value)
{
    float TType = Saturate((Value - Edge0) / (Edge1 - Edge0));
    return TType * TType * (3.0f - 2.0f * TType);
}

double NMath::SmoothStep(double Edge0, double Edge1, double Value)
{
    double TType = Saturate((Value - Edge0) / (Edge1 - Edge0));
    return TType * TType * (3.0 - 2.0 * TType);
}

float NMath::SmootherStep(float Edge0, float Edge1, float Value)
{
    float TType = Saturate((Value - Edge0) / (Edge1 - Edge0));
    return TType * TType * TType * (TType * (TType * 6.0f - 15.0f) + 10.0f);
}

// === 比较函数 ===

bool NMath::IsEqual(float A, float B, float Tolerance)
{
    return Abs(A - B) <= Tolerance;
}

bool NMath::IsEqual(double A, double B, double Tolerance)
{
    return Abs(A - B) <= Tolerance;
}

bool NMath::IsZero(float Value, float Tolerance)
{
    return Abs(Value) <= Tolerance;
}

bool NMath::IsZero(double Value, double Tolerance)
{
    return Abs(Value) <= Tolerance;
}

bool NMath::IsNearlyZero(float Value)
{
    return IsZero(Value, SMALL_NUMBER);
}

bool NMath::IsNearlyEqual(float A, float B)
{
    return IsEqual(A, B, EPSILON);
}

bool NMath::IsFinite(float Value)
{
    return std::isfinite(Value);
}

bool NMath::IsFinite(double Value)
{
    return std::isfinite(Value);
}

bool NMath::IsNaN(float Value)
{
    return std::isnan(Value);
}

bool NMath::IsNaN(double Value)
{
    return std::isnan(Value);
}

bool NMath::IsInfinite(float Value)
{
    return std::isinf(Value);
}

bool NMath::IsInfinite(double Value)
{
    return std::isinf(Value);
}

// === 角度转换 ===

float NMath::DegreesToRadians(float Degrees)
{
    return Degrees * DEG_TO_RAD;
}

double NMath::DegreesToRadians(double Degrees)
{
    return Degrees * DEG_TO_RAD;
}

float NMath::RadiansToDegrees(float Radians)
{
    return Radians * RAD_TO_DEG;
}

double NMath::RadiansToDegrees(double Radians)
{
    return Radians * RAD_TO_DEG;
}

float NMath::NormalizeAngle(float Angle)
{
    while (Angle > PI)
    {
        Angle -= TWO_PI;
    }
    while (Angle < -PI)
    {
        Angle += TWO_PI;
    }
    return Angle;
}

float NMath::NormalizeAngleDegrees(float Angle)
{
    while (Angle > 180.0f)
    {
        Angle -= 360.0f;
    }
    while (Angle < -180.0f)
    {
        Angle += 360.0f;
    }
    return Angle;
}

// === 随机数生成 ===

static std::random_device RandomDevice;
static std::mt19937 RandomGenerator(RandomDevice());

float NMath::Random()
{
    static std::uniform_real_distribution<float> Distribution(0.0f, 1.0f);
    return Distribution(RandomGenerator);
}

float NMath::RandomRange(float Min, float Max)
{
    return Min + Random() * (Max - Min);
}

int32_t NMath::RandomInt(int32_t Min, int32_t Max)
{
    std::uniform_int_distribution<int32_t> Distribution(Min, Max);
    return Distribution(RandomGenerator);
}

bool NMath::RandomBool()
{
    return Random() >= 0.5f;
}

void NMath::SetRandomSeed(uint32_t Seed)
{
    RandomGenerator.seed(Seed);
}

// === 二进制操作 ===

bool NMath::IsPowerOfTwo(uint32_t Value)
{
    return Value != 0 && (Value & (Value - 1)) == 0;
}

uint32_t NMath::NextPowerOfTwo(uint32_t Value)
{
    if (Value == 0) return 1;
    
    Value--;
    Value |= Value >> 1;
    Value |= Value >> 2;
    Value |= Value >> 4;
    Value |= Value >> 8;
    Value |= Value >> 16;
    Value++;
    
    return Value;
}

uint32_t NMath::CountLeadingZeros(uint32_t Value)
{
    if (Value == 0) return 32;
    
    uint32_t Count = 0;
    if ((Value & 0xFFFF0000) == 0) { Count += 16; Value <<= 16; }
    if ((Value & 0xFF000000) == 0) { Count += 8; Value <<= 8; }
    if ((Value & 0xF0000000) == 0) { Count += 4; Value <<= 4; }
    if ((Value & 0xC0000000) == 0) { Count += 2; Value <<= 2; }
    if ((Value & 0x80000000) == 0) { Count += 1; }
    
    return Count;
}

uint32_t NMath::CountTrailingZeros(uint32_t Value)
{
    if (Value == 0) return 32;
    
    uint32_t Count = 0;
    if ((Value & 0x0000FFFF) == 0) { Count += 16; Value >>= 16; }
    if ((Value & 0x000000FF) == 0) { Count += 8; Value >>= 8; }
    if ((Value & 0x0000000F) == 0) { Count += 4; Value >>= 4; }
    if ((Value & 0x00000003) == 0) { Count += 2; Value >>= 2; }
    if ((Value & 0x00000001) == 0) { Count += 1; }
    
    return Count;
}

uint32_t NMath::CountBits(uint32_t Value)
{
    uint32_t Count = 0;
    while (Value)
    {
        Count++;
        Value &= Value - 1; // 清除最低位的1
    }
    return Count;
}

// === 噪声函数 ===

float NMath::PerlinNoise(float X)
{
    // 简化的一维Perlin噪声
    int32_t I = static_cast<int32_t>(Floor(X)) & 255;
    X -= Floor(X);
    
    float U = X * X * (3.0f - 2.0f * X); // smoothstep
    
    // 使用简单的哈希函数生成伪随机梯度
    auto Hash = [](int32_t I) -> float {
        I = (I << 13) ^ I;
        return ((I * (I * I * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
    };
    
    return Lerp(Hash(I), Hash(I + 1), U);
}

float NMath::PerlinNoise(float X, float Y)
{
    // 简化的二维Perlin噪声
    int32_t IX = static_cast<int32_t>(Floor(X)) & 255;
    int32_t IY = static_cast<int32_t>(Floor(Y)) & 255;
    
    X -= Floor(X);
    Y -= Floor(Y);
    
    float U = X * X * (3.0f - 2.0f * X);
    float V = Y * Y * (3.0f - 2.0f * Y);
    
    auto Hash = [](int32_t X, int32_t Y) -> float {
        int32_t N = X + Y * 57;
        N = (N << 13) ^ N;
        return ((N * (N * N * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
    };
    
    float A = Hash(IX, IY);
    float B = Hash(IX + 1, IY);
    float C = Hash(IX, IY + 1);
    float D = Hash(IX + 1, IY + 1);
    
    float I1 = Lerp(A, B, U);
    float I2 = Lerp(C, D, U);
    
    return Lerp(I1, I2, V);
}

// === 几何计算辅助函数 ===

float NMath::DistanceSquared(const NVector2& A, const NVector2& B)
{
    float DX = B.X - A.X;
    float DY = B.Y - A.Y;
    return DX * DX + DY * DY;
}

float NMath::Distance(const NVector2& A, const NVector2& B)
{
    return Sqrt(DistanceSquared(A, B));
}

float NMath::DistanceSquared(const NVector3& A, const NVector3& B)
{
    float DX = B.X - A.X;
    float DY = B.Y - A.Y;
    float DZ = B.Z - A.Z;
    return DX * DX + DY * DY + DZ * DZ;
}

float NMath::Distance(const NVector3& A, const NVector3& B)
{
    return Sqrt(DistanceSquared(A, B));
}

} // namespace NLib