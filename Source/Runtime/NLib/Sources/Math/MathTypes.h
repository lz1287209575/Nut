#pragma once

#include <cmath>
#include <cstdint>
#include <limits>

namespace NLib
{
/**
 * @brief 数学常量定义
 */
namespace MathConstants
{
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float HALF_PI = PI * 0.5f;
constexpr float QUARTER_PI = PI * 0.25f;
constexpr float E = 2.71828182845904523536f;
constexpr float SQRT_2 = 1.41421356237309504880f;
constexpr float SQRT_3 = 1.73205080756887729352f;
constexpr float INV_PI = 1.0f / PI;
constexpr float INV_TWO_PI = 1.0f / TWO_PI;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

constexpr float EPSILON = 1e-6f;
constexpr float SMALL_EPSILON = 1e-8f;
constexpr float KINDA_SMALL_NUMBER = 1e-4f;
constexpr float BIG_NUMBER = 3.4e+38f;

constexpr double PI_DOUBLE = 3.1415926535897932384626433832795;
constexpr double E_DOUBLE = 2.7182818284590452353602874713527;
constexpr double EPSILON_DOUBLE = 1e-15;
} // namespace MathConstants

/**
 * @brief 基础数学函数
 */
class CMath
{
public:
	// === 浮点数比较 ===

	/**
	 * @brief 计算绝对值（自实现）
	 */
	template <typename T>
	static constexpr T Abs(T Value)
	{
		return Value < 0 ? -Value : Value;
	}

	/**
	 * @brief 检查两个浮点数是否近似相等
	 */
	static bool IsNearlyEqual(float A, float B, float Tolerance = MathConstants::EPSILON)
	{
		return Abs(A - B) <= Tolerance;
	}

	static bool IsNearlyEqual(double A, double B, double Tolerance = MathConstants::EPSILON_DOUBLE)
	{
		return Abs(A - B) <= Tolerance;
	}

	/**
	 * @brief 检查浮点数是否近似为零
	 */
	static bool IsNearlyZero(float Value, float Tolerance = MathConstants::EPSILON)
	{
		return Abs(Value) <= Tolerance;
	}

	static bool IsNearlyZero(double Value, double Tolerance = MathConstants::EPSILON_DOUBLE)
	{
		return Abs(Value) <= Tolerance;
	}

public:
	// === 基础数学运算 ===

	/**
	 * @brief 计算平方
	 */
	template <typename T>
	static constexpr T Square(T Value)
	{
		return Value * Value;
	}

	/**
	 * @brief 安全的平方根计算
	 */
	static float Sqrt(float Value)
	{
		return Value <= 0.0f ? 0.0f : sqrtf(Value);
	}

	static double Sqrt(double Value)
	{
		return Value <= 0.0 ? 0.0 : sqrt(Value);
	}

	/**
	 * @brief 反平方根（快速计算）
	 */
	static float InvSqrt(float Value)
	{
		if (Value <= 0.0f)
			return 0.0f;

		// 使用快速反平方根算法（Quake III算法的现代版本）
		float HalfValue = Value * 0.5f;
		uint32_t BitPattern = *(uint32_t*)&Value;
		BitPattern = 0x5f3759df - (BitPattern >> 1);
		Value = *(float*)&BitPattern;
		Value = Value * (1.5f - HalfValue * Value * Value); // 牛顿迭代
		return Value;
	}

public:
	// === 限制和范围函数 ===

	/**
	 * @brief 将值限制在指定范围内
	 */
	template <typename T>
	static constexpr T Clamp(T Value, T Min, T Max)
	{
		return Value < Min ? Min : (Value > Max ? Max : Value);
	}

	/**
	 * @brief 将值限制在0-1范围内
	 */
	template <typename T>
	static constexpr T Saturate(T Value)
	{
		return Clamp(Value, static_cast<T>(0), static_cast<T>(1));
	}

	/**
	 * @brief 获取最小值
	 */
	template <typename T>
	static constexpr T Min(T A, T B)
	{
		return A < B ? A : B;
	}

	/**
	 * @brief 获取最大值
	 */
	template <typename T>
	static constexpr T Max(T A, T B)
	{
		return A > B ? A : B;
	}

public:
	// === 符号和取整函数 ===

	/**
	 * @brief 获取数值的符号
	 * @return -1, 0, or 1
	 */
	template <typename T>
	static constexpr T Sign(T Value)
	{
		return Value > 0 ? static_cast<T>(1) : (Value < 0 ? static_cast<T>(-1) : static_cast<T>(0));
	}

	/**
	 * @brief 向下取整
	 */
	static int32_t FloorToInt(float Value)
	{
		return static_cast<int32_t>(floorf(Value));
	}

	/**
	 * @brief 向上取整
	 */
	static int32_t CeilToInt(float Value)
	{
		return static_cast<int32_t>(ceilf(Value));
	}

	/**
	 * @brief 四舍五入
	 */
	static int32_t RoundToInt(float Value)
	{
		return static_cast<int32_t>(Value + (Value >= 0.0f ? 0.5f : -0.5f));
	}

	/**
	 * @brief 截断取整
	 */
	static int32_t TruncToInt(float Value)
	{
		return static_cast<int32_t>(Value);
	}

public:
	// === 三角函数 ===

	/**
	 * @brief 角度转弧度
	 */
	static constexpr float DegreesToRadians(float Degrees)
	{
		return Degrees * MathConstants::DEG_TO_RAD;
	}

	/**
	 * @brief 弧度转角度
	 */
	static constexpr float RadiansToDegrees(float Radians)
	{
		return Radians * MathConstants::RAD_TO_DEG;
	}

	/**
	 * @brief 安全的反余弦函数
	 */
	static float Acos(float Value)
	{
		return acosf(Clamp(Value, -1.0f, 1.0f));
	}

	/**
	 * @brief 安全的反正弦函数
	 */
	static float Asin(float Value)
	{
		return asinf(Clamp(Value, -1.0f, 1.0f));
	}

public:
	// === 插值函数 ===

	/**
	 * @brief 线性插值
	 */
	template <typename T>
	static T Lerp(T A, T B, float Alpha)
	{
		return A + Alpha * (B - A);
	}

	/**
	 * @brief 反线性插值（获取插值因子）
	 */
	template <typename T>
	static float InverseLerp(T A, T B, T Value)
	{
		if (IsNearlyEqual(static_cast<float>(A), static_cast<float>(B)))
		{
			return 0.0f;
		}
		return static_cast<float>(Value - A) / static_cast<float>(B - A);
	}

	/**
	 * @brief 球面线性插值（用于角度）
	 */
	static float SlerpAngle(float A, float B, float Alpha)
	{
		float Delta = B - A;

		// 标准化到 [-PI, PI] 范围
		while (Delta > MathConstants::PI)
			Delta -= MathConstants::TWO_PI;
		while (Delta < -MathConstants::PI)
			Delta += MathConstants::TWO_PI;

		return A + Alpha * Delta;
	}

	/**
	 * @brief 平滑步进插值
	 */
	static float SmoothStep(float A, float B, float Alpha)
	{
		Alpha = Saturate(Alpha);
		Alpha = Alpha * Alpha * (3.0f - 2.0f * Alpha);
		return Lerp(A, B, Alpha);
	}

public:
	// === 模运算 ===

	/**
	 * @brief 浮点数模运算
	 */
	static float Fmod(float X, float Y)
	{
		return Y != 0.0f ? fmodf(X, Y) : 0.0f;
	}

	/**
	 * @brief 将角度标准化到 [0, 2π) 范围
	 */
	static float NormalizeAngle(float Angle)
	{
		Angle = Fmod(Angle, MathConstants::TWO_PI);
		return Angle < 0.0f ? Angle + MathConstants::TWO_PI : Angle;
	}

	/**
	 * @brief 将角度标准化到 [-π, π] 范围
	 */
	static float NormalizeAngleSigned(float Angle)
	{
		Angle = NormalizeAngle(Angle);
		return Angle > MathConstants::PI ? Angle - MathConstants::TWO_PI : Angle;
	}

public:
	// === 幂运算 ===

	/**
	 * @brief 安全的幂运算
	 */
	static float Pow(float Base, float Exponent)
	{
		return powf(Base, Exponent);
	}

	/**
	 * @brief 快速整数幂运算
	 */
	template <typename T>
	static T IntPow(T Base, int32_t Exponent)
	{
		if (Exponent == 0)
			return static_cast<T>(1);
		if (Exponent == 1)
			return Base;

		T Result = static_cast<T>(1);
		T CurrentBase = Base;
		int32_t CurrentExp = Abs(Exponent);

		while (CurrentExp > 0)
		{
			if (CurrentExp & 1)
			{
				Result *= CurrentBase;
			}
			CurrentBase *= CurrentBase;
			CurrentExp >>= 1;
		}

		return Exponent < 0 ? static_cast<T>(1) / Result : Result;
	}

public:
	// === 随机数相关 ===

	/**
	 * @brief 生成随机浮点数 [0, 1)
	 */
	static float RandomFloat()
	{
		return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	}

	/**
	 * @brief 生成指定范围内的随机浮点数
	 */
	static float RandomFloatInRange(float Min, float Max)
	{
		return Min + RandomFloat() * (Max - Min);
	}

	/**
	 * @brief 生成指定范围内的随机整数
	 */
	static int32_t RandomIntInRange(int32_t Min, int32_t Max)
	{
		return Min + rand() % (Max - Min + 1);
	}

public:
	// === 特殊数值检查 ===

	/**
	 * @brief 检查是否为有限数值
	 */
	static bool IsFinite(float Value)
	{
		return !(IsNaN(Value) || IsInfinite(Value));
	}

	/**
	 * @brief 检查是否为NaN
	 */
	static bool IsNaN(float Value)
	{
		return Value != Value; // NaN不等于自身
	}

	/**
	 * @brief 检查是否为无穷大
	 */
	static bool IsInfinite(float Value)
	{
		return Abs(Value) > 3.4e+38f; // 近似检查
	}
};

} // namespace NLib