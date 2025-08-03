#pragma once

#include "Math/NMath.h"
#include "Core/CObject.h"
#include "Containers/CString.h"

namespace NLib
{

class CMatrix3;
class CMatrix4;

/**
 * @brief 四元数 - 用于表示3D旋转
 */
class LIBNLIB_API CQuaternion : public CObject
{
    NCLASS()
class CQuaternion : public CObject
{
    GENERATED_BODY()

public:
    // 四元数分量 (X, Y, Z, W) where W是实部
    float X, Y, Z, W;
    
    // 构造函数
    CQuaternion();
    CQuaternion(float InX, float InY, float InZ, float InW);
    explicit CQuaternion(const NVector3& Axis, float AngleRadians);
    explicit CQuaternion(const NVector3& EulerAngles); // Pitch, Yaw, Roll (弧度)
    CQuaternion(const CQuaternion& Other);
    CQuaternion& operator=(const CQuaternion& Other);
    
    // 访问器
    float& operator[](int32_t Index);
    const float& operator[](int32_t Index) const;
    
    // 基础运算
    CQuaternion operator+(const CQuaternion& Other) const;
    CQuaternion operator-(const CQuaternion& Other) const;
    CQuaternion operator*(const CQuaternion& Other) const; // 四元数乘法（旋转组合）
    CQuaternion operator*(float Scalar) const;
    CQuaternion operator/(float Scalar) const;
    CQuaternion operator-() const; // 取负（等同于共轭）
    
    CQuaternion& operator+=(const CQuaternion& Other);
    CQuaternion& operator-=(const CQuaternion& Other);
    CQuaternion& operator*=(const CQuaternion& Other);
    CQuaternion& operator*=(float Scalar);
    CQuaternion& operator/=(float Scalar);
    
    // 比较运算
    bool operator==(const CQuaternion& Other) const;
    bool operator!=(const CQuaternion& Other) const;
    
    // 四元数运算
    float Dot(const CQuaternion& Other) const;
    float Length() const;
    float LengthSquared() const;
    
    CQuaternion Normalize() const;
    CQuaternion& NormalizeSelf();
    bool IsNormalized(float Tolerance = NMath::EPSILON) const;
    bool IsIdentity(float Tolerance = NMath::EPSILON) const;
    bool IsNearlyZero(float Tolerance = NMath::SMALL_NUMBER) const;
    
    // 四元数特殊运算
    CQuaternion Conjugate() const; // 共轭四元数
    CQuaternion Inverse() const;   // 逆四元数
    
    // 向量旋转
    NVector3 RotateVector(const NVector3& Vector) const;
    NVector3 UnrotateVector(const NVector3& Vector) const; // 逆旋转
    
    // 角度和轴
    void ToAxisAngle(NVector3& OutAxis, float& OutAngle) const;
    float GetAngle() const; // 获取旋转角度
    NVector3 GetAxis() const; // 获取旋转轴
    NVector3 GetRotationAxis() const; // 获取归一化的旋转轴
    
    // 欧拉角转换
    NVector3 ToEulerAngles() const; // 转换为欧拉角 (Pitch, Yaw, Roll)
    void ToEulerAngles(float& OutPitch, float& OutYaw, float& OutRoll) const;
    
    // 矩阵转换
    CMatrix3 ToMatrix3() const;
    CMatrix4 ToMatrix4() const;
    
    // 方向向量
    NVector3 GetForwardVector() const;  // 前方向
    NVector3 GetRightVector() const;    // 右方向  
    NVector3 GetUpVector() const;       // 上方向
    
    // 插值
    static CQuaternion Lerp(const CQuaternion& A, const CQuaternion& B, float Alpha);
    static CQuaternion Slerp(const CQuaternion& A, const CQuaternion& B, float Alpha);
    static CQuaternion SlerpFullPath(const CQuaternion& A, const CQuaternion& B, float Alpha); // 总是选择长路径
    static CQuaternion Squad(const CQuaternion& Q1, const CQuaternion& Q2, const CQuaternion& Q3, const CQuaternion& Q4, float Alpha);
    
    // 静态构造方法
    static CQuaternion Identity();
    static CQuaternion FromAxisAngle(const NVector3& Axis, float AngleRadians);
    static CQuaternion FromEulerAngles(float Pitch, float Yaw, float Roll);
    static CQuaternion FromEulerAngles(const NVector3& EulerAngles);
    static CQuaternion FromEulerDegrees(float PitchDeg, float YawDeg, float RollDeg);
    static CQuaternion FromRotationMatrix(const CMatrix3& Matrix);
    static CQuaternion FromRotationMatrix(const CMatrix4& Matrix);
    
    // 从向量到向量的旋转
    static CQuaternion FromToRotation(const NVector3& FromDirection, const NVector3& ToDirection);
    static CQuaternion LookRotation(const NVector3& Forward, const NVector3& Up = NVector3::Up);
    
    // 角度操作
    static float AngularDistance(const CQuaternion& A, const CQuaternion& B);
    static CQuaternion AngleAxis(float AngleRadians, const NVector3& Axis);
    
    // 随机四元数
    static CQuaternion Random();
    static CQuaternion RandomRotation();
    
    // 工具方法
    bool IsFinite() const;
    CQuaternion GetEquivalent() const; // 获取等价四元数（W为正）
    
    virtual CString ToString() const override;
    
private:
    // 内部辅助方法
    static CQuaternion SlerpInternal(const CQuaternion& A, const CQuaternion& B, float Alpha, bool bShortestPath);
};

// 全局运算符重载
LIBNLIB_API CQuaternion operator*(float Scalar, const CQuaternion& Quaternion);

/**
 * @brief 双四元数 - 用于表示刚体变换（旋转+平移）
 */
class LIBNLIB_API NDualQuaternion : public CObject
{
    NCLASS()
class NDualQuaternion : public NObject
{
    GENERATED_BODY()

public:
    CQuaternion Real;      // 实部四元数（旋转）
    CQuaternion Dual;      // 对偶部四元数（平移）
    
    // 构造函数
    NDualQuaternion();
    NDualQuaternion(const CQuaternion& InReal, const CQuaternion& InDual);
    NDualQuaternion(const CQuaternion& Rotation, const NVector3& Translation);
    NDualQuaternion(const CMatrix4& Transform);
    NDualQuaternion(const NDualQuaternion& Other);
    NDualQuaternion& operator=(const NDualQuaternion& Other);
    
    // 基础运算
    NDualQuaternion operator+(const NDualQuaternion& Other) const;
    NDualQuaternion operator-(const NDualQuaternion& Other) const;
    NDualQuaternion operator*(const NDualQuaternion& Other) const;
    NDualQuaternion operator*(float Scalar) const;
    NDualQuaternion operator/(float Scalar) const;
    
    NDualQuaternion& operator+=(const NDualQuaternion& Other);
    NDualQuaternion& operator-=(const NDualQuaternion& Other);
    NDualQuaternion& operator*=(const NDualQuaternion& Other);
    NDualQuaternion& operator*=(float Scalar);
    NDualQuaternion& operator/=(float Scalar);
    
    // 比较运算
    bool operator==(const NDualQuaternion& Other) const;
    bool operator!=(const NDualQuaternion& Other) const;
    
    // 双四元数运算
    float Length() const;
    float LengthSquared() const;
    
    NDualQuaternion Normalize() const;
    NDualQuaternion& NormalizeSelf();
    bool IsNormalized(float Tolerance = NMath::EPSILON) const;
    
    NDualQuaternion Conjugate() const;
    NDualQuaternion Inverse() const;
    
    // 变换操作
    NVector3 TransformPosition(const NVector3& Position) const;
    NVector3 TransformDirection(const NVector3& Direction) const;
    
    // 分解
    CQuaternion GetRotation() const;
    NVector3 GetTranslation() const;
    void GetTransform(CQuaternion& OutRotation, NVector3& OutTranslation) const;
    CMatrix4 ToMatrix4() const;
    
    // 插值
    static NDualQuaternion Lerp(const NDualQuaternion& A, const NDualQuaternion& B, float Alpha);
    static NDualQuaternion Slerp(const NDualQuaternion& A, const NDualQuaternion& B, float Alpha);
    
    // 静态构造方法
    static NDualQuaternion Identity();
    static NDualQuaternion FromTransform(const CQuaternion& Rotation, const NVector3& Translation);
    static NDualQuaternion FromMatrix(const CMatrix4& Transform);
    
    virtual CString ToString() const override;
};

} // namespace NLib