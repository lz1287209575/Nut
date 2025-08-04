#pragma once

#include "Containers/TString.h"
#include "MathTypes.h"

namespace NLib
{
/**
 * @brief 二维向量类
 *
 * 提供完整的2D向量运算功能，包括：
 * - 基础算术运算
 * - 向量数学运算（点积、叉积等）
 * - 长度和距离计算
 * - 标准化和旋转
 */
struct SVector2
{
public:
	// === 成员变量 ===
	float X;
	float Y;

public:
	// === 构造函数 ===

	/**
	 * @brief 默认构造函数（零向量）
	 */
	constexpr SVector2()
	    : X(0.0f),
	      Y(0.0f)
	{}

	/**
	 * @brief 构造函数
	 * @param InX X分量
	 * @param InY Y分量
	 */
	constexpr SVector2(float InX, float InY)
	    : X(InX),
	      Y(InY)
	{}

	/**
	 * @brief 单值构造函数
	 * @param Value 两个分量的值
	 */
	explicit constexpr SVector2(float Value)
	    : X(Value),
	      Y(Value)
	{}

public:
	// === 常用向量常量 ===

	static const SVector2 Zero;  // (0, 0)
	static const SVector2 One;   // (1, 1)
	static const SVector2 UnitX; // (1, 0)
	static const SVector2 UnitY; // (0, 1)
	static const SVector2 Up;    // (0, 1)
	static const SVector2 Down;  // (0, -1)
	static const SVector2 Left;  // (-1, 0)
	static const SVector2 Right; // (1, 0)

public:
	// === 访问器 ===

	/**
	 * @brief 下标访问
	 */
	float& operator[](int32_t Index)
	{
		return (&X)[Index];
	}

	const float& operator[](int32_t Index) const
	{
		return (&X)[Index];
	}

public:
	// === 赋值运算符 ===

	SVector2& operator=(const SVector2& Other) = default;

	SVector2& operator+=(const SVector2& Other)
	{
		X += Other.X;
		Y += Other.Y;
		return *this;
	}

	SVector2& operator-=(const SVector2& Other)
	{
		X -= Other.X;
		Y -= Other.Y;
		return *this;
	}

	SVector2& operator*=(float Scale)
	{
		X *= Scale;
		Y *= Scale;
		return *this;
	}

	SVector2& operator*=(const SVector2& Other)
	{
		X *= Other.X;
		Y *= Other.Y;
		return *this;
	}

	SVector2& operator/=(float Scale)
	{
		if (!CMath::IsNearlyZero(Scale))
		{
			float InvScale = 1.0f / Scale;
			X *= InvScale;
			Y *= InvScale;
		}
		return *this;
	}

	SVector2& operator/=(const SVector2& Other)
	{
		if (!CMath::IsNearlyZero(Other.X))
			X /= Other.X;
		if (!CMath::IsNearlyZero(Other.Y))
			Y /= Other.Y;
		return *this;
	}

public:
	// === 算术运算符 ===

	SVector2 operator+(const SVector2& Other) const
	{
		return SVector2(X + Other.X, Y + Other.Y);
	}

	SVector2 operator-(const SVector2& Other) const
	{
		return SVector2(X - Other.X, Y - Other.Y);
	}

	SVector2 operator*(float Scale) const
	{
		return SVector2(X * Scale, Y * Scale);
	}

	SVector2 operator*(const SVector2& Other) const
	{
		return SVector2(X * Other.X, Y * Other.Y);
	}

	SVector2 operator/(float Scale) const
	{
		if (CMath::IsNearlyZero(Scale))
		{
			return SVector2::Zero;
		}
		float InvScale = 1.0f / Scale;
		return SVector2(X * InvScale, Y * InvScale);
	}

	SVector2 operator/(const SVector2& Other) const
	{
		return SVector2(CMath::IsNearlyZero(Other.X) ? 0.0f : X / Other.X,
		                CMath::IsNearlyZero(Other.Y) ? 0.0f : Y / Other.Y);
	}

	SVector2 operator-() const
	{
		return SVector2(-X, -Y);
	}

public:
	// === 比较运算符 ===

	bool operator==(const SVector2& Other) const
	{
		return CMath::IsNearlyEqual(X, Other.X) && CMath::IsNearlyEqual(Y, Other.Y);
	}

	bool operator!=(const SVector2& Other) const
	{
		return !(*this == Other);
	}

public:
	// === 向量运算 ===

	/**
	 * @brief 计算向量长度的平方
	 */
	float SizeSquared() const
	{
		return X * X + Y * Y;
	}

	/**
	 * @brief 计算向量长度
	 */
	float Size() const
	{
		return CMath::Sqrt(SizeSquared());
	}

	/**
	 * @brief 检查是否为零向量
	 */
	bool IsZero() const
	{
		return CMath::IsNearlyZero(X) && CMath::IsNearlyZero(Y);
	}

	/**
	 * @brief 检查是否为单位向量
	 */
	bool IsUnit() const
	{
		return CMath::IsNearlyEqual(SizeSquared(), 1.0f);
	}

	/**
	 * @brief 标准化向量（修改自身）
	 */
	void Normalize()
	{
		float Length = Size();
		if (!CMath::IsNearlyZero(Length))
		{
			float InvLength = 1.0f / Length;
			X *= InvLength;
			Y *= InvLength;
		}
	}

	/**
	 * @brief 获取标准化后的向量（不修改自身）
	 */
	SVector2 GetNormalized() const
	{
		SVector2 Result = *this;
		Result.Normalize();
		return Result;
	}

	/**
	 * @brief 安全标准化（返回是否成功）
	 */
	bool SafeNormalize(float Tolerance = MathConstants::EPSILON)
	{
		float Length = Size();
		if (Length > Tolerance)
		{
			float InvLength = 1.0f / Length;
			X *= InvLength;
			Y *= InvLength;
			return true;
		}
		*this = SVector2::Zero;
		return false;
	}

public:
	// === 点积和叉积 ===

	/**
	 * @brief 计算点积
	 */
	float Dot(const SVector2& Other) const
	{
		return X * Other.X + Y * Other.Y;
	}

	/**
	 * @brief 计算2D叉积（标量值）
	 */
	float Cross(const SVector2& Other) const
	{
		return X * Other.Y - Y * Other.X;
	}

public:
	// === 距离计算 ===

	/**
	 * @brief 计算到另一点的距离平方
	 */
	float DistanceSquaredTo(const SVector2& Other) const
	{
		return (Other - *this).SizeSquared();
	}

	/**
	 * @brief 计算到另一点的距离
	 */
	float DistanceTo(const SVector2& Other) const
	{
		return CMath::Sqrt(DistanceSquaredTo(Other));
	}

public:
	// === 角度和旋转 ===

	/**
	 * @brief 获取向量的角度（弧度）
	 */
	float GetAngle() const
	{
		return atan2f(Y, X);
	}

	/**
	 * @brief 获取向量的角度（度）
	 */
	float GetAngleDegrees() const
	{
		return CMath::RadiansToDegrees(GetAngle());
	}

	/**
	 * @brief 旋转向量
	 * @param AngleRadians 旋转角度（弧度）
	 */
	SVector2 Rotated(float AngleRadians) const
	{
		float CosAngle = cosf(AngleRadians);
		float SinAngle = sinf(AngleRadians);
		return SVector2(X * CosAngle - Y * SinAngle, X * SinAngle + Y * CosAngle);
	}

	/**
	 * @brief 旋转向量（度）
	 */
	SVector2 RotatedDegrees(float AngleDegrees) const
	{
		return Rotated(CMath::DegreesToRadians(AngleDegrees));
	}

public:
	// === 投影和反射 ===

	/**
	 * @brief 投影到另一个向量上
	 */
	SVector2 ProjectOnto(const SVector2& Other) const
	{
		float OtherLengthSq = Other.SizeSquared();
		if (CMath::IsNearlyZero(OtherLengthSq))
		{
			return SVector2::Zero;
		}
		return Other * (Dot(Other) / OtherLengthSq);
	}

	/**
	 * @brief 沿法向量反射
	 */
	SVector2 Reflect(const SVector2& Normal) const
	{
		return *this - Normal * (2.0f * Dot(Normal));
	}

	/**
	 * @brief 获取垂直向量（逆时针90度）
	 */
	SVector2 GetPerpendicular() const
	{
		return SVector2(-Y, X);
	}

public:
	// === 限制和约束 ===

	/**
	 * @brief 限制向量长度
	 */
	SVector2 ClampSize(float MaxSize) const
	{
		float CurrentSize = Size();
		if (CurrentSize > MaxSize && !CMath::IsNearlyZero(CurrentSize))
		{
			return (*this) * (MaxSize / CurrentSize);
		}
		return *this;
	}

	/**
	 * @brief 限制向量分量
	 */
	SVector2 ClampComponents(const SVector2& Min, const SVector2& Max) const
	{
		return SVector2(CMath::Clamp(X, Min.X, Max.X), CMath::Clamp(Y, Min.Y, Max.Y));
	}

public:
	// === 插值 ===

	/**
	 * @brief 线性插值
	 */
	static SVector2 Lerp(const SVector2& A, const SVector2& B, float Alpha)
	{
		return A + (B - A) * Alpha;
	}

	/**
	 * @brief 球面线性插值
	 */
	static SVector2 Slerp(const SVector2& A, const SVector2& B, float Alpha)
	{
		float DotProduct = A.Dot(B);
		DotProduct = CMath::Clamp(DotProduct, -1.0f, 1.0f);

		float Theta = acosf(DotProduct);
		if (CMath::IsNearlyZero(Theta))
		{
			return Lerp(A, B, Alpha);
		}

		float SinTheta = sinf(Theta);
		float WeightA = sinf((1.0f - Alpha) * Theta) / SinTheta;
		float WeightB = sinf(Alpha * Theta) / SinTheta;

		return A * WeightA + B * WeightB;
	}

public:
	// === 实用函数 ===

	/**
	 * @brief 转换为字符串
	 */
	CString ToString() const
	{
		char Buffer[128];
		snprintf(Buffer, sizeof(Buffer), "Vector2(%.3f, %.3f)", X, Y);
		return CString(Buffer);
	}

	/**
	 * @brief 获取分量的最小值
	 */
	float GetMin() const
	{
		return CMath::Min(X, Y);
	}

	/**
	 * @brief 获取分量的最大值
	 */
	float GetMax() const
	{
		return CMath::Max(X, Y);
	}

	/**
	 * @brief 获取分量的绝对值向量
	 */
	SVector2 GetAbs() const
	{
		return SVector2(CMath::Abs(X), CMath::Abs(Y));
	}
};

// === 常量定义 ===
inline const SVector2 SVector2::Zero(0.0f, 0.0f);
inline const SVector2 SVector2::One(1.0f, 1.0f);
inline const SVector2 SVector2::UnitX(1.0f, 0.0f);
inline const SVector2 SVector2::UnitY(0.0f, 1.0f);
inline const SVector2 SVector2::Up(0.0f, 1.0f);
inline const SVector2 SVector2::Down(0.0f, -1.0f);
inline const SVector2 SVector2::Left(-1.0f, 0.0f);
inline const SVector2 SVector2::Right(1.0f, 0.0f);

// === 全局运算符 ===

/**
 * @brief 标量 * 向量
 */
inline SVector2 operator*(float Scale, const SVector2& Vector)
{
	return Vector * Scale;
}

// === 类型别名 ===
using FVector2D = SVector2;
using FPoint2D = SVector2;

} // namespace NLib