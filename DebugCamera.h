#pragma once
#include <cmath>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif 
struct Vector2
{
	float x;
	float y;
};

struct Vector3
{
	float x, y, z;
};
struct Matrix4x4
{
	float m[4][4];
};
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);

Matrix4x4 MakeRotateXMatrix(float radian);
Matrix4x4 MakeRotateYMatrix(float radian);
Matrix4x4 MakeRotateZMatrix(float radian);

Matrix4x4 MakeTranslateMatrix(const Vector3& translate);

Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);
Matrix4x4 Inverse(const Matrix4x4& m);
Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix);
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);
	Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);
	// 6. 単位行列の作成
	Matrix4x4 MakeIdentity4x4();
class DebugCamera
{
private:
	
	Matrix4x4 matRot_;

	Vector3 translatetion_ = { 0,0,-50.0f };

	Matrix4x4 worldMatrix_{};

	Matrix4x4 viewMatrix_{};	

	Matrix4x4 MakeViewMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

public:

	void Initialize();
	void Update();

	Matrix4x4 GetViewMatrix();
};


