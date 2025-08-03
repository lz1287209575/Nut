#pragma once

#include "Math/NMath.h"
#include "Core/CObject.h"
#include "Containers/CString.h"

namespace NLib
{

class CQuaternion; // 前向声明

/**
 * @brief 3x3矩阵 - 主要用于2D变换和3D旋转
 */
class LIBNLIB_API CMatrix3 : public CObject
{
    NCLASS()
class CMatrix3 : public CObject
{
    GENERATED_BODY()

public:
    // 矩阵数据 - 行主序存储
    float M[3][3];
    
    // 构造函数
    CMatrix3();
    CMatrix3(float M00, float M01, float M02,
             float M10, float M11, float M12,
             float M20, float M21, float M22);
    explicit CMatrix3(const float Matrix[9]);
    CMatrix3(const CMatrix3& Other);
    CMatrix3& operator=(const CMatrix3& Other);
    
    // 访问器
    float* operator[](int32_t Row);
    const float* operator[](int32_t Row) const;
    float& operator()(int32_t Row, int32_t Col);
    const float& operator()(int32_t Row, int32_t Col) const;
    
    // 矩阵运算
    CMatrix3 operator+(const CMatrix3& Other) const;
    CMatrix3 operator-(const CMatrix3& Other) const;
    CMatrix3 operator*(const CMatrix3& Other) const;
    CMatrix3 operator*(float Scalar) const;
    CMatrix3 operator/(float Scalar) const;
    
    CMatrix3& operator+=(const CMatrix3& Other);
    CMatrix3& operator-=(const CMatrix3& Other);
    CMatrix3& operator*=(const CMatrix3& Other);
    CMatrix3& operator*=(float Scalar);
    CMatrix3& operator/=(float Scalar);
    
    // 比较运算
    bool operator==(const CMatrix3& Other) const;
    bool operator!=(const CMatrix3& Other) const;
    
    // 向量变换
    NVector3 TransformVector(const NVector3& Vector) const;
    NVector2 TransformPoint2D(const NVector2& Point) const;
    
    // 矩阵属性
    float Determinant() const;
    CMatrix3 Transpose() const;
    CMatrix3& TransposeSelf();
    CMatrix3 Inverse() const;
    bool TryInverse(CMatrix3& OutInverse) const;
    
    // 矩阵分解
    bool IsIdentity(float Tolerance = NMath::EPSILON) const;
    bool IsOrthogonal(float Tolerance = NMath::EPSILON) const;
    float GetScale() const;
    NVector2 GetScale2D() const;
    float GetRotation2D() const; // 返回2D旋转角度（弧度）
    
    // 静态构造方法
    static CMatrix3 Identity();
    static CMatrix3 Zero();
    static CMatrix3 Scale(float ScaleX, float ScaleY, float ScaleZ = 1.0f);
    static CMatrix3 Scale(const NVector3& Scale);
    static CMatrix3 Scale2D(float ScaleX, float ScaleY);
    static CMatrix3 Scale2D(const NVector2& Scale);
    
    static CMatrix3 Rotation2D(float AngleRadians);
    static CMatrix3 RotationX(float AngleRadians);
    static CMatrix3 RotationY(float AngleRadians);
    static CMatrix3 RotationZ(float AngleRadians);
    static CMatrix3 RotationAxis(const NVector3& Axis, float AngleRadians);
    
    static CMatrix3 Translation2D(float X, float Y);
    static CMatrix3 Translation2D(const NVector2& Translation);
    
    static CMatrix3 FromColumns(const NVector3& Col0, const NVector3& Col1, const NVector3& Col2);
    static CMatrix3 FromRows(const NVector3& Row0, const NVector3& Row1, const NVector3& Row2);
    
    // 获取行和列
    NVector3 GetRow(int32_t RowIndex) const;
    NVector3 GetColumn(int32_t ColIndex) const;
    void SetRow(int32_t RowIndex, const NVector3& Row);
    void SetColumn(int32_t ColIndex, const NVector3& Column);
    
    virtual CString ToString() const override;
};

/**
 * @brief 4x4矩阵 - 用于3D变换
 */
class LIBNLIB_API CMatrix4 : public CObject
{
    NCLASS()
class CMatrix4 : public CObject
{
    GENERATED_BODY()

public:
    // 矩阵数据 - 行主序存储
    float M[4][4];
    
    // 构造函数
    CMatrix4();
    CMatrix4(float M00, float M01, float M02, float M03,
             float M10, float M11, float M12, float M13,
             float M20, float M21, float M22, float M23,
             float M30, float M31, float M32, float M33);
    explicit CMatrix4(const float Matrix[16]);
    explicit CMatrix4(const CMatrix3& Matrix3); // 扩展3x3矩阵到4x4
    CMatrix4(const CMatrix4& Other);
    CMatrix4& operator=(const CMatrix4& Other);
    
    // 访问器
    float* operator[](int32_t Row);
    const float* operator[](int32_t Row) const;
    float& operator()(int32_t Row, int32_t Col);
    const float& operator()(int32_t Row, int32_t Col) const;
    
    // 矩阵运算
    CMatrix4 operator+(const CMatrix4& Other) const;
    CMatrix4 operator-(const CMatrix4& Other) const;
    CMatrix4 operator*(const CMatrix4& Other) const;
    CMatrix4 operator*(float Scalar) const;
    CMatrix4 operator/(float Scalar) const;
    
    CMatrix4& operator+=(const CMatrix4& Other);
    CMatrix4& operator-=(const CMatrix4& Other);
    CMatrix4& operator*=(const CMatrix4& Other);
    CMatrix4& operator*=(float Scalar);
    CMatrix4& operator/=(float Scalar);
    
    // 比较运算
    bool operator==(const CMatrix4& Other) const;
    bool operator!=(const CMatrix4& Other) const;
    
    // 向量变换
    NVector4 TransformVector(const NVector4& Vector) const;
    NVector3 TransformPosition(const NVector3& Position) const; // 齐次坐标点变换
    NVector3 TransformDirection(const NVector3& Direction) const; // 方向向量变换（忽略平移）
    NVector3 TransformNormal(const NVector3& Normal) const; // 法向量变换（使用逆转置）
    
    // 矩阵属性
    float Determinant() const;
    CMatrix4 Transpose() const;
    CMatrix4& TransposeSelf();
    CMatrix4 Inverse() const;
    bool TryInverse(CMatrix4& OutInverse) const;
    
    // 矩阵分解
    bool IsIdentity(float Tolerance = NMath::EPSILON) const;
    bool IsOrthogonal(float Tolerance = NMath::EPSILON) const;
    bool DecomposeTransform(NVector3& OutTranslation, CQuaternion& OutRotation, NVector3& OutScale) const;
    bool DecomposeTransform(NVector3& OutTranslation, CMatrix3& OutRotation, NVector3& OutScale) const;
    
    // 获取变换分量
    NVector3 GetTranslation() const;
    CMatrix3 GetRotationMatrix() const;
    NVector3 GetScale() const;
    NVector3 GetRight() const;   // 第一列的XYZ分量
    NVector3 GetUp() const;      // 第二列的XYZ分量
    NVector3 GetForward() const; // 第三列的XYZ分量的负值（右手坐标系）
    
    void SetTranslation(const NVector3& Translation);
    void SetRotation(const CMatrix3& Rotation);
    void SetRotation(const CQuaternion& Rotation);
    void SetScale(const NVector3& Scale);
    
    // 静态构造方法
    static CMatrix4 Identity();
    static CMatrix4 Zero();
    
    // 变换矩阵
    static CMatrix4 Translation(const NVector3& Translation);
    static CMatrix4 Translation(float X, float Y, float Z);
    
    static CMatrix4 Scale(const NVector3& Scale);
    static CMatrix4 Scale(float ScaleX, float ScaleY, float ScaleZ);
    static CMatrix4 Scale(float UniformScale);
    
    static CMatrix4 RotationX(float AngleRadians);
    static CMatrix4 RotationY(float AngleRadians);
    static CMatrix4 RotationZ(float AngleRadians);
    static CMatrix4 RotationAxis(const NVector3& Axis, float AngleRadians);
    static CMatrix4 RotationEuler(float Pitch, float Yaw, float Roll);
    static CMatrix4 RotationEuler(const NVector3& EulerAngles);
    static CMatrix4 RotationQuaternion(const CQuaternion& Quaternion);
    
    // 组合变换
    static CMatrix4 TRS(const NVector3& Translation, const CQuaternion& Rotation, const NVector3& Scale);
    static CMatrix4 TRS(const NVector3& Translation, const CMatrix3& Rotation, const NVector3& Scale);
    
    // 视图矩阵
    static CMatrix4 LookAt(const NVector3& Eye, const NVector3& Target, const NVector3& Up);
    static CMatrix4 LookTo(const NVector3& Eye, const NVector3& Direction, const NVector3& Up);
    
    // 投影矩阵
    static CMatrix4 Perspective(float FovY, float AspectRatio, float NearPlane, float FarPlane);
    static CMatrix4 PerspectiveOffCenter(float Left, float Right, float Bottom, float Top, float NearPlane, float FarPlane);
    static CMatrix4 Orthographic(float Width, float Height, float NearPlane, float FarPlane);
    static CMatrix4 OrthographicOffCenter(float Left, float Right, float Bottom, float Top, float NearPlane, float FarPlane);
    
    // 其他矩阵
    static CMatrix4 Reflection(const NVector4& Plane); // 平面反射矩阵
    static CMatrix4 Shadow(const NVector4& Plane, const NVector3& LightDirection); // 阴影投影矩阵
    
    static CMatrix4 FromColumns(const NVector4& Col0, const NVector4& Col1, const NVector4& Col2, const NVector4& Col3);
    static CMatrix4 FromRows(const NVector4& Row0, const NVector4& Row1, const NVector4& Row2, const NVector4& Row3);
    
    // 获取行和列
    NVector4 GetRow(int32_t RowIndex) const;
    NVector4 GetColumn(int32_t ColIndex) const;
    void SetRow(int32_t RowIndex, const NVector4& Row);
    void SetColumn(int32_t ColIndex, const NVector4& Column);
    
    // 转换为其他格式
    CMatrix3 ToMatrix3() const; // 提取左上角3x3矩阵
    void ToFloatArray(float OutArray[16]) const; // 转换为float数组（列主序，用于OpenGL）
    void ToFloatArrayRowMajor(float OutArray[16]) const; // 转换为float数组（行主序）
    
    virtual CString ToString() const override;
};

// 全局运算符重载
LIBNLIB_API CMatrix3 operator*(float Scalar, const CMatrix3& Matrix);
LIBNLIB_API CMatrix4 operator*(float Scalar, const CMatrix4& Matrix);

} // namespace NLib