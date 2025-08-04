#pragma once

/**
 * @file Math.h
 * @brief NLib数学库主头文件
 *
 * 提供完整的游戏开发数学功能：
 * - 基础数学函数和常量
 * - 2D/3D向量运算
 * - 四元数旋转
 * - 矩阵变换（未来实现）
 * - 插值和曲线函数
 */

// 基础数学类型和函数
#include "MathTypes.h"

// 向量类型
#include "Vector2.h"
#include "Vector3.h"

// 旋转表示
#include "Quaternion.h"

// 命名空间别名，便于使用
namespace NLib
{
// === 常用数学函数别名 ===
using Math = CMath;

// === 常用类型别名 ===
using Vec2 = SVector2;
using Vec3 = SVector3;
using Quat = SQuaternion;

// === 常用常量别名 ===
namespace Math
{
using namespace MathConstants;
}

/**
 * @brief 数学工具类 - 提供高级数学运算
 */
class CMathUtils
{
public:
	// === 范围和映射函数 ===

	/**
	 * @brief 将值从一个范围映射到另一个范围
	 */
	template <typename T>
	static T MapRange(T Value, T FromMin, T FromMax, T ToMin, T ToMax)
	{
		if (CMath::IsNearlyEqual(static_cast<float>(FromMax), static_cast<float>(FromMin)))
		{
			return ToMin;
		}

		T Alpha = (Value - FromMin) / (FromMax - FromMin);
		return CMath::Lerp(ToMin, ToMax, static_cast<float>(Alpha));
	}

	/**
	 * @brief 将值映射到0-1范围
	 */
	template <typename T>
	static float Normalize(T Value, T Min, T Max)
	{
		return MapRange(Value, Min, Max, static_cast<T>(0), static_cast<T>(1));
	}

public:
	// === 缓动函数 ===

	/**
	 * @brief 缓入函数（二次方）
	 */
	static float EaseIn(float t)
	{
		return t * t;
	}

	/**
	 * @brief 缓出函数（二次方）
	 */
	static float EaseOut(float t)
	{
		return 1.0f - CMath::Square(1.0f - t);
	}

	/**
	 * @brief 缓入缓出函数（二次方）
	 */
	static float EaseInOut(float t)
	{
		return t < 0.5f ? 2.0f * t * t : 1.0f - CMath::Square(-2.0f * t + 2.0f) * 0.5f;
	}

	/**
	 * @brief 弹性缓动
	 */
	static float EaseElastic(float t)
	{
		if (CMath::IsNearlyZero(t))
			return 0.0f;
		if (CMath::IsNearlyEqual(t, 1.0f))
			return 1.0f;

		float p = 0.3f;
		float s = p * 0.25f;
		return CMath::Pow(2.0f, -10.0f * t) * sinf((t - s) * MathConstants::TWO_PI / p) + 1.0f;
	}

public:
	// === 噪声和随机函数 ===

	/**
	 * @brief 简单的伪随机数生成器（线性同余）
	 */
	static uint32_t Random(uint32_t& Seed)
	{
		Seed = Seed * 1664525u + 1013904223u;
		return Seed;
	}

	/**
	 * @brief 基于种子的随机浮点数 [0, 1)
	 */
	static float RandomFloat(uint32_t& Seed)
	{
		return static_cast<float>(Random(Seed)) / static_cast<float>(UINT32_MAX);
	}

	/**
	 * @brief 简单的1D噪声函数
	 */
	static float Noise1D(float x)
	{
		int32_t i = CMath::FloorToInt(x);
		float f = x - static_cast<float>(i);

		uint32_t seed1 = static_cast<uint32_t>(i);
		uint32_t seed2 = static_cast<uint32_t>(i + 1);

		float a = RandomFloat(seed1);
		float b = RandomFloat(seed2);

		// 使用平滑插值
		f = f * f * (3.0f - 2.0f * f);
		return CMath::Lerp(a, b, f);
	}

public:
	// === 几何计算 ===

	/**
	 * @brief 计算点到线段的最短距离
	 */
	static float PointToLineSegmentDistance(const SVector3& Point, const SVector3& LineStart, const SVector3& LineEnd)
	{
		SVector3 LineVec = LineEnd - LineStart;
		SVector3 PointVec = Point - LineStart;

		float LineLengthSq = LineVec.SizeSquared();
		if (CMath::IsNearlyZero(LineLengthSq))
		{
			return Point.DistanceTo(LineStart);
		}

		float t = CMath::Clamp(PointVec.Dot(LineVec) / LineLengthSq, 0.0f, 1.0f);
		SVector3 Projection = LineStart + LineVec * t;
		return Point.DistanceTo(Projection);
	}

	/**
	 * @brief 计算两个球体是否相交
	 */
	static bool SpheresIntersect(const SVector3& Center1, float Radius1, const SVector3& Center2, float Radius2)
	{
		float DistanceSq = Center1.DistanceSquaredTo(Center2);
		float RadiusSum = Radius1 + Radius2;
		return DistanceSq <= RadiusSum * RadiusSum;
	}

	/**
	 * @brief 计算三角形的重心坐标
	 */
	static SVector3 CalculateBarycentric(const SVector3& Point, const SVector3& A, const SVector3& B, const SVector3& C)
	{
		SVector3 v0 = C - A;
		SVector3 v1 = B - A;
		SVector3 v2 = Point - A;

		float dot00 = v0.Dot(v0);
		float dot01 = v0.Dot(v1);
		float dot02 = v0.Dot(v2);
		float dot11 = v1.Dot(v1);
		float dot12 = v1.Dot(v2);

		float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
		float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
		float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

		return SVector3(1.0f - u - v, v, u); // (A权重, B权重, C权重)
	}

public:
	// === 曲线和样条 ===

	/**
	 * @brief 贝塞尔曲线插值（二次）
	 */
	static SVector3 BezierQuadratic(const SVector3& P0, const SVector3& P1, const SVector3& P2, float t)
	{
		float oneMinusT = 1.0f - t;
		return P0 * (oneMinusT * oneMinusT) + P1 * (2.0f * oneMinusT * t) + P2 * (t * t);
	}

	/**
	 * @brief 贝塞尔曲线插值（三次）
	 */
	static SVector3 BezierCubic(const SVector3& P0, const SVector3& P1, const SVector3& P2, const SVector3& P3, float t)
	{
		float oneMinusT = 1.0f - t;
		float oneMinusTSq = oneMinusT * oneMinusT;
		float tSq = t * t;

		return P0 * (oneMinusTSq * oneMinusT) + P1 * (3.0f * oneMinusTSq * t) + P2 * (3.0f * oneMinusT * tSq) +
		       P3 * (tSq * t);
	}

	/**
	 * @brief Catmull-Rom样条插值
	 */
	static SVector3 CatmullRom(const SVector3& P0, const SVector3& P1, const SVector3& P2, const SVector3& P3, float t)
	{
		float tSq = t * t;
		float tCube = tSq * t;

		return P1 + (P2 - P0) * (0.5f * t) + (P0 * 2.0f - P1 * 5.0f + P2 * 4.0f - P3) * (0.5f * tSq) +
		       (P1 * 3.0f - P0 - P2 * 3.0f + P3) * (0.5f * tCube);
	}

public:
	// === 角度工具 ===

	/**
	 * @brief 计算两个角度之间的最短差值
	 */
	static float AngleDifference(float Angle1, float Angle2)
	{
		float Diff = Angle2 - Angle1;
		Diff = CMath::NormalizeAngleSigned(Diff);
		return Diff;
	}

	/**
	 * @brief 角度的平滑插值
	 */
	static float LerpAngle(float A, float B, float Alpha)
	{
		return A + AngleDifference(A, B) * Alpha;
	}

	/**
	 * @brief 将向量转换为2D角度
	 */
	static float VectorToAngle(const SVector2& Vector)
	{
		return atan2f(Vector.Y, Vector.X);
	}

	/**
	 * @brief 将2D角度转换为单位向量
	 */
	static SVector2 AngleToVector(float Angle)
	{
		return SVector2(cosf(Angle), sinf(Angle));
	}
};

} // namespace NLib