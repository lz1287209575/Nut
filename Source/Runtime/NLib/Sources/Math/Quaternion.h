#pragma once

#include "Containers/TString.h"
#include "MathTypes.h"
#include "Vector3.h"

namespace NLib
{
/**
 * @brief 四元数类
 *
 * 用于表示3D旋转，提供：
 * - 旋转表示和计算
 * - 四元数运算（乘法、逆、共轭等）
 * - 插值和平滑旋转
 * - 与欧拉角的转换
 */
struct SQuaternion
{
public:
	// === 成员变量 ===
	float X; // i 分量
	float Y; // j 分量
	float Z; // k 分量
	float W; // 实数分量

public:
	// === 构造函数 ===

	/**
	 * @brief 默认构造函数（单位四元数）
	 */
	constexpr SQuaternion()
	    : X(0.0f),
	      Y(0.0f),
	      Z(0.0f),
	      W(1.0f)
	{}

	/**
	 * @brief 构造函数
	 */
	constexpr SQuaternion(float InX, float InY, float InZ, float InW)
	    : X(InX),
	      Y(InY),
	      Z(InZ),
	      W(InW)
	{}

	/**
	 * @brief 从向量和标量构造
	 */
	SQuaternion(const SVector3& Vector, float InW)
	    : X(Vector.X),
	      Y(Vector.Y),
	      Z(Vector.Z),
	      W(InW)
	{}

public:
	// === 常用四元数常量 ===

	static const SQuaternion Identity; // 单位四元数 (0, 0, 0, 1)

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
	 * @brief 获取虚部（向量部分）
	 */
	SVector3 GetVector() const
	{
		return SVector3(X, Y, Z);
	}

	/**
	 * @brief 设置虚部
	 */
	void SetVector(const SVector3& Vector)
	{
		X = Vector.X;
		Y = Vector.Y;
		Z = Vector.Z;
	}

public:
	// === 赋值运算符 ===

	SQuaternion& operator=(const SQuaternion& Other) = default;

	SQuaternion& operator+=(const SQuaternion& Other)
	{
		X += Other.X;
		Y += Other.Y;
		Z += Other.Z;
		W += Other.W;
		return *this;
	}

	SQuaternion& operator-=(const SQuaternion& Other)
	{
		X -= Other.X;
		Y -= Other.Y;
		Z -= Other.Z;
		W -= Other.W;
		return *this;
	}

	SQuaternion& operator*=(float Scale)
	{
		X *= Scale;
		Y *= Scale;
		Z *= Scale;
		W *= Scale;
		return *this;
	}

	SQuaternion& operator*=(const SQuaternion& Other)
	{
		*this = *this * Other;
		return *this;
	}

public:
	// === 算术运算符 ===

	SQuaternion operator+(const SQuaternion& Other) const
	{
		return SQuaternion(X + Other.X, Y + Other.Y, Z + Other.Z, W + Other.W);
	}

	SQuaternion operator-(const SQuaternion& Other) const
	{
		return SQuaternion(X - Other.X, Y - Other.Y, Z - Other.Z, W - Other.W);
	}

	SQuaternion operator*(float Scale) const
	{
		return SQuaternion(X * Scale, Y * Scale, Z * Scale, W * Scale);
	}

	/**
	 * @brief 四元数乘法（旋转组合）
	 */
	SQuaternion operator*(const SQuaternion& Other) const
	{
		return SQuaternion(W * Other.X + X * Other.W + Y * Other.Z - Z * Other.Y,
		                   W * Other.Y + Y * Other.W + Z * Other.X - X * Other.Z,
		                   W * Other.Z + Z * Other.W + X * Other.Y - Y * Other.X,
		                   W * Other.W - X * Other.X - Y * Other.Y - Z * Other.Z);
	}

	SQuaternion operator-() const
	{
		return SQuaternion(-X, -Y, -Z, -W);
	}

public:
	// === 比较运算符 ===

	bool operator==(const SQuaternion& Other) const
	{
		return CMath::IsNearlyEqual(X, Other.X) && CMath::IsNearlyEqual(Y, Other.Y) &&
		       CMath::IsNearlyEqual(Z, Other.Z) && CMath::IsNearlyEqual(W, Other.W);
	}

	bool operator!=(const SQuaternion& Other) const
	{
		return !(*this == Other);
	}

public:
	// === 四元数运算 ===

	/**
	 * @brief 计算四元数长度的平方
	 */
	float SizeSquared() const
	{
		return X * X + Y * Y + Z * Z + W * W;
	}

	/**
	 * @brief 计算四元数长度
	 */
	float Size() const
	{
		return CMath::Sqrt(SizeSquared());
	}

	/**
	 * @brief 检查是否为零四元数
	 */
	bool IsZero() const
	{
		return CMath::IsNearlyZero(X) && CMath::IsNearlyZero(Y) && CMath::IsNearlyZero(Z) && CMath::IsNearlyZero(W);
	}

	/**
	 * @brief 检查是否为单位四元数
	 */
	bool IsUnit() const
	{
		return CMath::IsNearlyEqual(SizeSquared(), 1.0f);
	}

	/**
	 * @brief 标准化四元数（修改自身）
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
			W *= InvLength;
		}
		else
		{
			*this = Identity;
		}
	}

	/**
	 * @brief 获取标准化后的四元数（不修改自身）
	 */
	SQuaternion GetNormalized() const
	{
		SQuaternion Result = *this;
		Result.Normalize();
		return Result;
	}

	/**
	 * @brief 获取共轭四元数
	 */
	SQuaternion GetConjugate() const
	{
		return SQuaternion(-X, -Y, -Z, W);
	}

	/**
	 * @brief 获取逆四元数
	 */
	SQuaternion GetInverse() const
	{
		float LengthSq = SizeSquared();
		if (CMath::IsNearlyZero(LengthSq))
		{
			return Identity;
		}

		SQuaternion Conjugate = GetConjugate();
		return Conjugate * (1.0f / LengthSq);
	}

public:
	// === 旋转操作 ===

	/**
	 * @brief 旋转向量
	 */
	SVector3 RotateVector(const SVector3& Vector) const
	{
		// q * (0, v) * q^(-1)
		SQuaternion VectorQuat(Vector.X, Vector.Y, Vector.Z, 0.0f);
		SQuaternion Result = (*this) * VectorQuat * GetConjugate();
		return SVector3(Result.X, Result.Y, Result.Z);
	}

	/**
	 * @brief 获取旋转轴
	 */
	SVector3 GetAxis() const
	{
		if (CMath::IsNearlyEqual(W, 1.0f))
		{
			return SVector3::UnitX; // 无旋转时返回默认轴
		}

		float SinHalfAngle = CMath::Sqrt(1.0f - W * W);
		if (CMath::IsNearlyZero(SinHalfAngle))
		{
			return SVector3::UnitX;
		}

		float InvSinHalfAngle = 1.0f / SinHalfAngle;
		return SVector3(X * InvSinHalfAngle, Y * InvSinHalfAngle, Z * InvSinHalfAngle);
	}

	/**
	 * @brief 获取旋转角度（弧度）
	 */
	float GetAngle() const
	{
		return 2.0f * acosf(CMath::Clamp(CMath::Abs(W), 0.0f, 1.0f));
	}

public:
	// === 工厂函数 ===

	/**
	 * @brief 从轴角创建四元数
	 * @param Axis 旋转轴（必须是单位向量）
	 * @param AngleRadians 旋转角度（弧度）
	 */
	static SQuaternion FromAxisAngle(const SVector3& Axis, float AngleRadians)
	{
		float HalfAngle = AngleRadians * 0.5f;
		float SinHalfAngle = sinf(HalfAngle);
		float CosHalfAngle = cosf(HalfAngle);

		return SQuaternion(Axis.X * SinHalfAngle, Axis.Y * SinHalfAngle, Axis.Z * SinHalfAngle, CosHalfAngle);
	}

	/**
	 * @brief 从欧拉角创建四元数（度）
	 * @param Pitch X轴旋转（俯仰）
	 * @param Yaw Y轴旋转（偏航）
	 * @param Roll Z轴旋转（翻滚）
	 */
	static SQuaternion FromEulerDegrees(float Pitch, float Yaw, float Roll)
	{
		return FromEulerRadians(
		    CMath::DegreesToRadians(Pitch), CMath::DegreesToRadians(Yaw), CMath::DegreesToRadians(Roll));
	}

	/**
	 * @brief 从欧拉角创建四元数（弧度）
	 */
	static SQuaternion FromEulerRadians(float Pitch, float Yaw, float Roll)
	{
		float HalfPitch = Pitch * 0.5f;
		float HalfYaw = Yaw * 0.5f;
		float HalfRoll = Roll * 0.5f;

		float CosPitch = cosf(HalfPitch);
		float SinPitch = sinf(HalfPitch);
		float CosYaw = cosf(HalfYaw);
		float SinYaw = sinf(HalfYaw);
		float CosRoll = cosf(HalfRoll);
		float SinRoll = sinf(HalfRoll);

		return SQuaternion(SinPitch * CosYaw * CosRoll - CosPitch * SinYaw * SinRoll,
		                   CosPitch * SinYaw * CosRoll + SinPitch * CosYaw * SinRoll,
		                   CosPitch * CosYaw * SinRoll - SinPitch * SinYaw * CosRoll,
		                   CosPitch * CosYaw * CosRoll + SinPitch * SinYaw * SinRoll);
	}

	/**
	 * @brief 从两个向量之间的旋转创建四元数
	 */
	static SQuaternion FromTwoVectors(const SVector3& From, const SVector3& To)
	{
		SVector3 FromNorm = From.GetNormalized();
		SVector3 ToNorm = To.GetNormalized();

		float Dot = FromNorm.Dot(ToNorm);

		// 处理平行向量的特殊情况
		if (CMath::IsNearlyEqual(Dot, 1.0f))
		{
			return Identity; // 相同方向
		}
		else if (CMath::IsNearlyEqual(Dot, -1.0f))
		{
			// 相反方向，选择一个垂直轴
			SVector3 Axis = FromNorm.GetOrthogonal();
			return FromAxisAngle(Axis, MathConstants::PI);
		}

		SVector3 Axis = FromNorm.Cross(ToNorm);
		float W = 1.0f + Dot;

		SQuaternion Result(Axis.X, Axis.Y, Axis.Z, W);
		Result.Normalize();
		return Result;
	}

public:
	// === 欧拉角转换 ===

	/**
	 * @brief 转换为欧拉角（度）
	 */
	SVector3 ToEulerDegrees() const
	{
		SVector3 Radians = ToEulerRadians();
		return SVector3(
		    CMath::RadiansToDegrees(Radians.X), CMath::RadiansToDegrees(Radians.Y), CMath::RadiansToDegrees(Radians.Z));
	}

	/**
	 * @brief 转换为欧拉角（弧度）
	 */
	SVector3 ToEulerRadians() const
	{
		// 转换为欧拉角 (Pitch, Yaw, Roll)
		float SinR_CosPitch = 2.0f * (W * X + Y * Z);
		float CosR_CosPitch = 1.0f - 2.0f * (X * X + Y * Y);
		float Roll = atan2f(SinR_CosPitch, CosR_CosPitch);

		float SinPitch = 2.0f * (W * Y - Z * X);
		SinPitch = CMath::Clamp(SinPitch, -1.0f, 1.0f);
		float Pitch = asinf(SinPitch);

		float SinY_CosPitch = 2.0f * (W * Z + X * Y);
		float CosY_CosPitch = 1.0f - 2.0f * (Y * Y + Z * Z);
		float Yaw = atan2f(SinY_CosPitch, CosY_CosPitch);

		return SVector3(Pitch, Yaw, Roll);
	}

public:
	// === 插值 ===

	/**
	 * @brief 线性插值（不推荐用于旋转）
	 */
	static SQuaternion Lerp(const SQuaternion& A, const SQuaternion& B, float Alpha)
	{
		return (A + (B - A) * Alpha).GetNormalized();
	}

	/**
	 * @brief 球面线性插值
	 */
	static SQuaternion Slerp(const SQuaternion& A, const SQuaternion& B, float Alpha)
	{
		float Dot = A.X * B.X + A.Y * B.Y + A.Z * B.Z + A.W * B.W;

		// 选择最短路径
		SQuaternion B_Adjusted = B;
		if (Dot < 0.0f)
		{
			B_Adjusted = -B;
			Dot = -Dot;
		}

		Dot = CMath::Clamp(Dot, -1.0f, 1.0f);

		// 如果四元数非常接近，使用线性插值
		if (Dot > 0.9995f)
		{
			return Lerp(A, B_Adjusted, Alpha);
		}

		float Theta = acosf(Dot);
		float SinTheta = sinf(Theta);
		float WeightA = sinf((1.0f - Alpha) * Theta) / SinTheta;
		float WeightB = sinf(Alpha * Theta) / SinTheta;

		return SQuaternion(WeightA * A.X + WeightB * B_Adjusted.X,
		                   WeightA * A.Y + WeightB * B_Adjusted.Y,
		                   WeightA * A.Z + WeightB * B_Adjusted.Z,
		                   WeightA * A.W + WeightB * B_Adjusted.W);
	}

public:
	// === 实用函数 ===

	/**
	 * @brief 转换为字符串
	 */
	CString ToString() const
	{
		char Buffer[128];
		snprintf(Buffer, sizeof(Buffer), "Quaternion(%.3f, %.3f, %.3f, %.3f)", X, Y, Z, W);
		return CString(Buffer);
	}

	/**
	 * @brief 计算与另一个四元数的点积
	 */
	float Dot(const SQuaternion& Other) const
	{
		return X * Other.X + Y * Other.Y + Z * Other.Z + W * Other.W;
	}
};

// === 常量定义 ===
inline const SQuaternion SQuaternion::Identity(0.0f, 0.0f, 0.0f, 1.0f);

// === 全局运算符 ===

/**
 * @brief 标量 * 四元数
 */
inline SQuaternion operator*(float Scale, const SQuaternion& Quat)
{
	return Quat * Scale;
}

// === 类型别名 ===
using FQuat = SQuaternion;
using FQuaternion = SQuaternion;

} // namespace NLib