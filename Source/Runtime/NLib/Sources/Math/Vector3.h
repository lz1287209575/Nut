#pragma once

#include "Containers/TString.h"
#include "MathTypes.h"
#include "Vector2.h"

namespace NLib
{
/**
 * @brief 三维向量类
 *
 * 提供完整的3D向量运算功能，包括：
 * - 基础算术运算
 * - 向量数学运算（点积、叉积等）
 * - 长度和距离计算
 * - 标准化和旋转
 */
struct SVector3
{
public:
	// === 成员变量 ===
	float X;
	float Y;
	float Z;

public:
	// === 构造函数 ===

	/**
	 * @brief 默认构造函数（零向量）
	 */
	constexpr SVector3()
	    : X(0.0f),
	      Y(0.0f),
	      Z(0.0f)
	{}

	/**
	 * @brief 构造函数
	 */
	constexpr SVector3(float InX, float InY, float InZ)
	    : X(InX),
	      Y(InY),
	      Z(InZ)
	{}

	/**
	 * @brief 单值构造函数
	 */
	explicit constexpr SVector3(float Value)
	    : X(Value),
	      Y(Value),
	      Z(Value)
	{}

	/**
	 * @brief 从2D向量构造
	 */
	explicit SVector3(const SVector2& Vector2D, float InZ = 0.0f)
	    : X(Vector2D.X),
	      Y(Vector2D.Y),
	      Z(InZ)
	{}

public:
	// === 常用向量常量 ===

	static const SVector3 Zero;     // (0, 0, 0)
	static const SVector3 One;      // (1, 1, 1)
	static const SVector3 UnitX;    // (1, 0, 0)
	static const SVector3 UnitY;    // (0, 1, 0)
	static const SVector3 UnitZ;    // (0, 0, 1)
	static const SVector3 Forward;  // (0, 0, 1)
	static const SVector3 Backward; // (0, 0, -1)
	static const SVector3 Up;       // (0, 1, 0)
	static const SVector3 Down;     // (0, -1, 0)
	static const SVector3 Left;     // (-1, 0, 0)
	static const SVector3 Right;    // (1, 0, 0)

public:
	// === 访问器 ===

	float& operator[](int32_t Index)
	{
		return (&X)[Index];
	}

	const float& operator[](int32_t Index) const
	{
		return (&X)[Index];
	}

	/**
	 * @brief 获取XY分量作为2D向量
	 */
	SVector2 GetXY() const
	{
		return SVector2(X, Y);
	}

	/**
	 * @brief 获取XZ分量作为2D向量
	 */
	SVector2 GetXZ() const
	{
		return SVector2(X, Z);
	}

	/**
	 * @brief 获取YZ分量作为2D向量
	 */
	SVector2 GetYZ() const
	{
		return SVector2(Y, Z);
	}

public:
	// === 赋值运算符 ===

	SVector3& operator=(const SVector3& Other) = default;

	SVector3& operator+=(const SVector3& Other)
	{
		X += Other.X;
		Y += Other.Y;
		Z += Other.Z;
		return *this;
	}

	SVector3& operator-=(const SVector3& Other)
	{
		X -= Other.X;
		Y -= Other.Y;
		Z -= Other.Z;
		return *this;
	}

	SVector3& operator*=(float Scale)
	{
		X *= Scale;
		Y *= Scale;
		Z *= Scale;
		return *this;
	}

	SVector3& operator*=(const SVector3& Other)
	{
		X *= Other.X;
		Y *= Other.Y;
		Z *= Other.Z;
		return *this;
	}

	SVector3& operator/=(float Scale)
	{
		if (!CMath::IsNearlyZero(Scale))
		{
			float InvScale = 1.0f / Scale;
			X *= InvScale;
			Y *= InvScale;
			Z *= InvScale;
		}
		return *this;
	}

	SVector3& operator/=(const SVector3& Other)
	{
		if (!CMath::IsNearlyZero(Other.X))
			X /= Other.X;
		if (!CMath::IsNearlyZero(Other.Y))
			Y /= Other.Y;
		if (!CMath::IsNearlyZero(Other.Z))
			Z /= Other.Z;
		return *this;
	}

public:
	// === 算术运算符 ===

	SVector3 operator+(const SVector3& Other) const
	{
		return SVector3(X + Other.X, Y + Other.Y, Z + Other.Z);
	}

	SVector3 operator-(const SVector3& Other) const
	{
		return SVector3(X - Other.X, Y - Other.Y, Z - Other.Z);
	}

	SVector3 operator*(float Scale) const
	{
		return SVector3(X * Scale, Y * Scale, Z * Scale);
	}

	SVector3 operator*(const SVector3& Other) const
	{
		return SVector3(X * Other.X, Y * Other.Y, Z * Other.Z);
	}

	SVector3 operator/(float Scale) const
	{
		if (CMath::IsNearlyZero(Scale))
		{
			return SVector3::Zero;
		}
		float InvScale = 1.0f / Scale;
		return SVector3(X * InvScale, Y * InvScale, Z * InvScale);
	}

	SVector3 operator/(const SVector3& Other) const
	{
		return SVector3(CMath::IsNearlyZero(Other.X) ? 0.0f : X / Other.X,
		                CMath::IsNearlyZero(Other.Y) ? 0.0f : Y / Other.Y,
		                CMath::IsNearlyZero(Other.Z) ? 0.0f : Z / Other.Z);
	}

	SVector3 operator-() const
	{
		return SVector3(-X, -Y, -Z);
	}

public:
	// === 比较运算符 ===

	bool operator==(const SVector3& Other) const
	{
		return CMath::IsNearlyEqual(X, Other.X) && CMath::IsNearlyEqual(Y, Other.Y) && CMath::IsNearlyEqual(Z, Other.Z);
	}

	bool operator!=(const SVector3& Other) const
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
		return X * X + Y * Y + Z * Z;
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
		return CMath::IsNearlyZero(X) && CMath::IsNearlyZero(Y) && CMath::IsNearlyZero(Z);
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
			Z *= InvLength;
		}
	}

	/**
	 * @brief 获取标准化后的向量（不修改自身）
	 */
	SVector3 GetNormalized() const
	{
		SVector3 Result = *this;
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
			Z *= InvLength;
			return true;
		}
		*this = SVector3::Zero;
		return false;
	}

public:
	// === 点积和叉积 ===

	/**
	 * @brief 计算点积
	 */
	float Dot(const SVector3& Other) const
	{
		return X * Other.X + Y * Other.Y + Z * Other.Z;
	}

	/**
	 * @brief 计算叉积
	 */
	SVector3 Cross(const SVector3& Other) const
	{
		return SVector3(Y * Other.Z - Z * Other.Y, Z * Other.X - X * Other.Z, X * Other.Y - Y * Other.X);
	}

public:
	// === 距离计算 ===

	/**
	 * @brief 计算到另一点的距离平方
	 */
	float DistanceSquaredTo(const SVector3& Other) const
	{
		return (Other - *this).SizeSquared();
	}

	/**
	 * @brief 计算到另一点的距离
	 */
	float DistanceTo(const SVector3& Other) const
	{
		return CMath::Sqrt(DistanceSquaredTo(Other));
	}

public:
	// === 投影和反射 ===

	/**
	 * @brief 投影到另一个向量上
	 */
	SVector3 ProjectOnto(const SVector3& Other) const
	{
		float OtherLengthSq = Other.SizeSquared();
		if (CMath::IsNearlyZero(OtherLengthSq))
		{
			return SVector3::Zero;
		}
		return Other * (Dot(Other) / OtherLengthSq);
	}

	/**
	 * @brief 投影到平面上
	 */
	SVector3 ProjectOntoPlane(const SVector3& PlaneNormal) const
	{
		return *this - ProjectOnto(PlaneNormal);
	}

	/**
	 * @brief 沿法向量反射
	 */
	SVector3 Reflect(const SVector3& Normal) const
	{
		return *this - Normal * (2.0f * Dot(Normal));
	}

public:
	// === 限制和约束 ===

	/**
	 * @brief 限制向量长度
	 */
	SVector3 ClampSize(float MaxSize) const
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
	SVector3 ClampComponents(const SVector3& Min, const SVector3& Max) const
	{
		return SVector3(CMath::Clamp(X, Min.X, Max.X), CMath::Clamp(Y, Min.Y, Max.Y), CMath::Clamp(Z, Min.Z, Max.Z));
	}

public:
	// === 插值 ===

	/**
	 * @brief 线性插值
	 */
	static SVector3 Lerp(const SVector3& A, const SVector3& B, float Alpha)
	{
		return A + (B - A) * Alpha;
	}

	/**
	 * @brief 球面线性插值
	 */
	static SVector3 Slerp(const SVector3& A, const SVector3& B, float Alpha)
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
		snprintf(Buffer, sizeof(Buffer), "Vector3(%.3f, %.3f, %.3f)", X, Y, Z);
		return CString(Buffer);
	}

	/**
	 * @brief 获取分量的最小值
	 */
	float GetMin() const
	{
		return CMath::Min(CMath::Min(X, Y), Z);
	}

	/**
	 * @brief 获取分量的最大值
	 */
	float GetMax() const
	{
		return CMath::Max(CMath::Max(X, Y), Z);
	}

	/**
	 * @brief 获取分量的绝对值向量
	 */
	SVector3 GetAbs() const
	{
		return SVector3(CMath::Abs(X), CMath::Abs(Y), CMath::Abs(Z));
	}

	/**
	 * @brief 根据最大分量创建正交向量
	 */
	SVector3 GetOrthogonal() const
	{
		if (CMath::Abs(X) >= CMath::Abs(Y) && CMath::Abs(X) >= CMath::Abs(Z))
		{
			return SVector3(-Z, 0.0f, X).GetNormalized();
		}
		else
		{
			return SVector3(0.0f, Z, -Y).GetNormalized();
		}
	}
};

// === 常量定义 ===
inline const SVector3 SVector3::Zero(0.0f, 0.0f, 0.0f);
inline const SVector3 SVector3::One(1.0f, 1.0f, 1.0f);
inline const SVector3 SVector3::UnitX(1.0f, 0.0f, 0.0f);
inline const SVector3 SVector3::UnitY(0.0f, 1.0f, 0.0f);
inline const SVector3 SVector3::UnitZ(0.0f, 0.0f, 1.0f);
inline const SVector3 SVector3::Forward(0.0f, 0.0f, 1.0f);
inline const SVector3 SVector3::Backward(0.0f, 0.0f, -1.0f);
inline const SVector3 SVector3::Up(0.0f, 1.0f, 0.0f);
inline const SVector3 SVector3::Down(0.0f, -1.0f, 0.0f);
inline const SVector3 SVector3::Left(-1.0f, 0.0f, 0.0f);
inline const SVector3 SVector3::Right(1.0f, 0.0f, 0.0f);

// === 全局运算符 ===

/**
 * @brief 标量 * 向量
 */
inline SVector3 operator*(float Scale, const SVector3& Vector)
{
	return Vector * Scale;
}

// === 类型别名 ===
using FVector = SVector3;
using FVector3D = SVector3;
using FPoint3D = SVector3;

} // namespace NLib