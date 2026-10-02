#include "DebugCamera.h"
#include <windows.h>
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2)
{
	Matrix4x4 result{};

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			for (int k = 0; k < 4; k++) {
				result.m[i][j] += m1.m[i][k] * m2.m[k][j];
			}
		}
	}
	return result;
}
Matrix4x4 MakeRotateXMatrix(float radian)
{
	Matrix4x4 result =
	{
		1.0f,         0.0f,         0.0f, 0.0f,
		0.0f, cosf(radian), sinf(radian), 0.0f,
		0.0f,-sinf(radian), cosf(radian), 0.0f,
		0.0f,         0.0f,         0.0f, 1.0f
	};

	return result;
}

Matrix4x4 MakeRotateYMatrix(float radian)
{
	Matrix4x4 result =
	{
		 cosf(radian), 0.0f, -sinf(radian), 0.0f,
		 0.0f,         1.0f,  0.0f,         0.0f,
		 sinf(radian), 0.0f,  cosf(radian), 0.0f,
		 0.0f,         0.0f,  0.0f,         1.0f
	};

	return result;
}

Matrix4x4 MakeRotateZMatrix(float radian)
{
	Matrix4x4 result =
	{
		 cosf(radian),  sinf(radian), 0.0f, 0.0f,
		-sinf(radian),  cosf(radian), 0.0f, 0.0f,
		 0.0f,          0.0f,         1.0f, 0.0f,
		 0.0f,          0.0f,         0.0f, 1.0f
	};

	return result;
}

Matrix4x4 MakeTranslateMatrix(const Vector3& translate)
{
	Matrix4x4 result = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		translate.x, translate.y, translate.z, 1.0f
	};
	return result;
}

Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate)
{
	// スケーリング行列
	Matrix4x4 scaleMatrix = {
	scale.x,0.0f,0.0f,0.f,
	0.0f,scale.y,0.0f,0.0f,
	0.0f,0.0f,scale.z,0.0f,
	0.0f,0.0f,0.0f,1.0f
	};



	// 回転行列

	Matrix4x4 rotateX = {

	1.0f,0.0f,0.0f,0.0f,
	0.0f,cosf(rotate.x),sinf(rotate.x),0.0f,
	0.0f,-sinf(rotate.x),cosf(rotate.x),0.0f,
	0.0f,0.0f,0.0f,1.0f

	};

	///////////////////////////////////////////////////////////////////

	Matrix4x4 rotateY = {

		cosf(rotate.y),0.0f,-sinf(rotate.y),0.0f,
		0.0f,1.0f,0.0f,0.0f,
		sinf(rotate.y),0.0f,cosf(rotate.y),0.0f,
		0.0f,0.0f,0.0f,1.0f

	};

	//////////////////////////////////////////////////////////////////

	Matrix4x4 rotateZ = {

		cosf(rotate.z),sinf(rotate.z),0.0f,0.0f,
		-sinf(rotate.z),cosf(rotate.z),0.0f,0.0f,
		0.0f,0.0f,1.0f,0.0f,
		0.0f,0.0f,0.0f,1.0f

	};

	/////////////////////////////////////////////////////////////////

	// 平行移動行列
	Matrix4x4 translateMatrix = {
	1.0f,0.0f,0.0f,0.0f,
	0.0f,1.0f,0.0f,0.0f,
	0.0f,0.0f,1.0f,0.0f,
	translate.x,translate.y,translate.z,1.0f
	};

	Matrix4x4 rotateXYZ = Multiply(Multiply(rotateX, rotateY), rotateZ);

	// アフィン変換行列の合成
	Matrix4x4 result = Multiply(Multiply(scaleMatrix, rotateXYZ), translateMatrix);

	return result;
}

Matrix4x4 Inverse(const Matrix4x4& m)
{
	Matrix4x4 result{};
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	result.m[0][0] = m.m[1][1] * m.m[2][2] * m.m[3][3] - m.m[1][1] * m.m[2][3] * m.m[3][2] - m.m[2][1] * m.m[1][2] * m.m[3][3] + m.m[2][1] * m.m[1][3] * m.m[3][2] + m.m[3][1] * m.m[1][2] * m.m[2][3] - m.m[3][1] * m.m[1][3] * m.m[2][2];

	result.m[0][1] = -m.m[0][1] * m.m[2][2] * m.m[3][3] + m.m[0][1] * m.m[2][3] * m.m[3][2] + m.m[2][1] * m.m[0][2] * m.m[3][3] - m.m[2][1] * m.m[0][3] * m.m[3][2] - m.m[3][1] * m.m[0][2] * m.m[2][3] + m.m[3][1] * m.m[0][3] * m.m[2][2];

	result.m[0][2] = m.m[0][1] * m.m[1][2] * m.m[3][3] - m.m[0][1] * m.m[1][3] * m.m[3][2] - m.m[1][1] * m.m[0][2] * m.m[3][3] + m.m[1][1] * m.m[0][3] * m.m[3][2] + m.m[3][1] * m.m[0][2] * m.m[1][3] - m.m[3][1] * m.m[0][3] * m.m[1][2];

	result.m[0][3] = -m.m[0][1] * m.m[1][2] * m.m[2][3] + m.m[0][1] * m.m[1][3] * m.m[2][2] + m.m[1][1] * m.m[0][2] * m.m[2][3] - m.m[1][1] * m.m[0][3] * m.m[2][2] - m.m[2][1] * m.m[0][2] * m.m[1][3] + m.m[2][1] * m.m[0][3] * m.m[1][2];
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	result.m[1][0] = -m.m[1][0] * m.m[2][2] * m.m[3][3] + m.m[1][0] * m.m[2][3] * m.m[3][2] + m.m[2][0] * m.m[1][2] * m.m[3][3] - m.m[2][0] * m.m[1][3] * m.m[3][2] - m.m[3][0] * m.m[1][2] * m.m[2][3] + m.m[3][0] * m.m[1][3] * m.m[2][2];

	result.m[1][1] = m.m[0][0] * m.m[2][2] * m.m[3][3] - m.m[0][0] * m.m[2][3] * m.m[3][2] - m.m[2][0] * m.m[0][2] * m.m[3][3] + m.m[2][0] * m.m[0][3] * m.m[3][2] + m.m[3][0] * m.m[0][2] * m.m[2][3] - m.m[3][0] * m.m[0][3] * m.m[2][2];

	result.m[1][2] = -m.m[0][0] * m.m[1][2] * m.m[3][3] + m.m[0][0] * m.m[1][3] * m.m[3][2] + m.m[1][0] * m.m[0][2] * m.m[3][3] - m.m[1][0] * m.m[0][3] * m.m[3][2] - m.m[3][0] * m.m[0][2] * m.m[1][3] + m.m[3][0] * m.m[0][3] * m.m[1][2];

	result.m[1][3] = m.m[0][0] * m.m[1][2] * m.m[2][3] - m.m[0][0] * m.m[1][3] * m.m[2][2] - m.m[1][0] * m.m[0][2] * m.m[2][3] + m.m[1][0] * m.m[0][3] * m.m[2][2] + m.m[2][0] * m.m[0][2] * m.m[1][3] - m.m[2][0] * m.m[0][3] * m.m[1][2];
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	result.m[2][0] = m.m[1][0] * m.m[2][1] * m.m[3][3] - m.m[1][0] * m.m[2][3] * m.m[3][1] - m.m[2][0] * m.m[1][1] * m.m[3][3] + m.m[2][0] * m.m[1][3] * m.m[3][1] + m.m[3][0] * m.m[1][1] * m.m[2][3] - m.m[3][0] * m.m[1][3] * m.m[2][1];

	result.m[2][1] = -m.m[0][0] * m.m[2][1] * m.m[3][3] + m.m[0][0] * m.m[2][3] * m.m[3][1] + m.m[2][0] * m.m[0][1] * m.m[3][3] - m.m[2][0] * m.m[0][3] * m.m[3][1] - m.m[3][0] * m.m[0][1] * m.m[2][3] + m.m[3][0] * m.m[0][3] * m.m[2][1];

	result.m[2][2] = m.m[0][0] * m.m[1][1] * m.m[3][3] - m.m[0][0] * m.m[1][3] * m.m[3][1] - m.m[1][0] * m.m[0][1] * m.m[3][3] + m.m[1][0] * m.m[0][3] * m.m[3][1] + m.m[3][0] * m.m[0][1] * m.m[1][3] - m.m[3][0] * m.m[0][3] * m.m[1][1];

	result.m[2][3] = -m.m[0][0] * m.m[1][1] * m.m[2][3] + m.m[0][0] * m.m[1][3] * m.m[2][1] + m.m[1][0] * m.m[0][1] * m.m[2][3] - m.m[1][0] * m.m[0][3] * m.m[2][1] - m.m[2][0] * m.m[0][1] * m.m[1][3] + m.m[2][0] * m.m[0][3] * m.m[1][1];
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	result.m[3][0] = -m.m[1][0] * m.m[2][1] * m.m[3][2] + m.m[1][0] * m.m[2][2] * m.m[3][1] + m.m[2][0] * m.m[1][1] * m.m[3][2] - m.m[2][0] * m.m[1][2] * m.m[3][1] - m.m[3][0] * m.m[1][1] * m.m[2][2] + m.m[3][0] * m.m[1][2] * m.m[2][1];

	result.m[3][1] = m.m[0][0] * m.m[2][1] * m.m[3][2] - m.m[0][0] * m.m[2][2] * m.m[3][1] - m.m[2][0] * m.m[0][1] * m.m[3][2] + m.m[2][0] * m.m[0][2] * m.m[3][1] + m.m[3][0] * m.m[0][1] * m.m[2][2] - m.m[3][0] * m.m[0][2] * m.m[2][1];

	result.m[3][2] = -m.m[0][0] * m.m[1][1] * m.m[3][2] + m.m[0][0] * m.m[1][2] * m.m[3][1] + m.m[1][0] * m.m[0][1] * m.m[3][2] - m.m[1][0] * m.m[0][2] * m.m[3][1] - m.m[3][0] * m.m[0][1] * m.m[1][2] + m.m[3][0] * m.m[0][2] * m.m[1][1];

	result.m[3][3] = m.m[0][0] * m.m[1][1] * m.m[2][2] - m.m[0][0] * m.m[1][2] * m.m[2][1] - m.m[1][0] * m.m[0][1] * m.m[2][2] + m.m[1][0] * m.m[0][2] * m.m[2][1] + m.m[2][0] * m.m[0][1] * m.m[1][2] - m.m[2][0] * m.m[0][2] * m.m[1][1];
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// 行列式
	float det =
		m.m[0][0] * result.m[0][0] +
		m.m[0][1] * result.m[1][0] +
		m.m[0][2] * result.m[2][0] +
		m.m[0][3] * result.m[3][0];

	// 行列式で割る
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result.m[i][j] /= det;
		}
	}

	return result;
}
Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix)
{
	Vector3 result{};



	float w =
		vector.x * matrix.m[0][3] +
		vector.y * matrix.m[1][3] +
		vector.z * matrix.m[2][3] +
		matrix.m[3][3];

	result.x =
		vector.x * matrix.m[0][0] +
		vector.y * matrix.m[1][0] +
		vector.z * matrix.m[2][0] +
		matrix.m[3][0];

	result.y =
		vector.x * matrix.m[0][1] +
		vector.y * matrix.m[1][1] +
		vector.z * matrix.m[2][1] +
		matrix.m[3][1];

	result.z =
		vector.x * matrix.m[0][2] +
		vector.y * matrix.m[1][2] +
		vector.z * matrix.m[2][2] +
		matrix.m[3][2];

	if (w != 0.0f) {
		result.x /= w;
		result.y /= w;
		result.z /= w;
	}



	return result;
}

// 6. 単位行列の作成
Matrix4x4 MakeIdentity4x4()
{
	Matrix4x4 result{};
	result.m[0][0] = 1.0f;
	result.m[0][1] = 0.0f;
	result.m[0][2] = 0.0f;
	result.m[0][3] = 0.0f;

	result.m[1][0] = 0.0f;
	result.m[1][1] = 1.0f;
	result.m[1][2] = 0.0f;
	result.m[1][3] = 0.0f;

	result.m[2][0] = 0.0f;
	result.m[2][1] = 0.0f;
	result.m[2][2] = 1.0f;
	result.m[2][3] = 0.0f;

	result.m[3][0] = 0.0f;
	result.m[3][1] = 0.0f;
	result.m[3][2] = 0.0f;
	result.m[3][3] = 1.0f;

	return result;
}

Matrix4x4 DebugCamera::MakeViewMatrix(
	const Vector3& scale,
	const Vector3& rotate,
	const Vector3& translate)
{
	// カメラのワールド行列を作る
	Matrix4x4 cameraMatrix = MakeAffineMatrix(scale, rotate, translate);

	// 逆行列を取るとビュー行列になる
	Matrix4x4 viewMatrix = Inverse(cameraMatrix);

	return viewMatrix;
}
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip)
{
	Matrix4x4 result = {};
	float f = 1.0f / tanf(fovY / 2.0f);
	result.m[0][0] = f / aspectRatio;
	result.m[1][1] = f;
	result.m[2][2] = farClip / (farClip - nearClip);
	result.m[2][3] = 1.0f;
	result.m[3][2] = (-nearClip * farClip) / (farClip - nearClip);
	result.m[3][3] = 0.0f;
	return result;
}

Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip)
{
	Matrix4x4 result = {};
	result.m[0][0] = 2.0f / (right - left);
	result.m[1][1] = 2.0f / (top - bottom);
	result.m[2][2] = 1.0f / (farClip - nearClip);
	result.m[3][0] = (left + right) / (left - right);
	result.m[3][1] = (top + bottom) / (bottom - top);
	result.m[3][2] = (nearClip) / (nearClip - farClip);
	result.m[3][3] = 1.0f;
	return result;
}

Matrix4x4 DebugCamera::GetViewMatrix()
{
	return viewMatrix_;
}


void DebugCamera::Initialize() {

	POINT mousePos;
	GetCursorPos(&mousePos);

	translatetion_ = { 0,0,-50.0f };

	matRot_ = MakeIdentity4x4();
	worldMatrix_ = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0,0,0 }, translatetion_);
}

void DebugCamera::Update(Vector2 mouseDelta) {

	//HWND hwnd = GetActiveWindow();

	//RECT rect;
	//GetClientRect(hwnd, &rect);

	//POINT center;
	//center.x = (rect.right - rect.left) / 2;
	//center.y = (rect.bottom - rect.top) / 2;


	//ClientToScreen(hwnd, &center);

	//POINT mousePos;
	//GetCursorPos(&mousePos);


	//const float rotateSpeed = 0.005f;
	if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {


		if (mouseDelta.y != 0.0f)
		{
			// X軸周りの角度を計算する
			const float rotateSpeed = 0.005f;
			Matrix4x4 matRotDelta = MakeIdentity4x4();

			matRotDelta = Multiply(MakeRotateXMatrix(mouseDelta.x * rotateSpeed), matRotDelta);

			//matRotDelta = Multiply(matRotDelta, MakeRotateYMatrix(mouseDelta.y * rotateSpeed));

			matRot_ = Multiply(matRotDelta, matRot_);
			//rotatetion_.x += mouseDelta.y * rotateSpeed;
		}

		if (mouseDelta.x != 0.0f)
		{
			// Y軸周りの角度を計算する
			const float rotateSpeed = 0.005f;
			Matrix4x4 matRotDelta = MakeIdentity4x4();

			//matRotDelta = Multiply(MakeRotateXMatrix(mouseDelta.x * rotateSpeed), matRotDelta);

			matRotDelta = Multiply(MakeRotateYMatrix(mouseDelta.y * rotateSpeed), matRotDelta);

			matRot_ = Multiply(matRotDelta, matRot_);
			//rotatetion_.y += mouseDelta.x * rotateSpeed;
		}

		//if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
		//{
		//	if (mouseDelta.x != 0.0f)
		//	{
		//		// Z軸周りの角度を計算する
		//		const float rotateSpeed = 0.005f;
		//		Matrix4x4 matRotDelta = MakeIdentity4x4();

		//		matRotDelta = Multiply(matRotDelta, MakeRotateXMatrix(mouseDelta.x * rotateSpeed));

		//		matRotDelta = Multiply(matRotDelta, MakeRotateYMatrix(mouseDelta.y * rotateSpeed));

		//		matRot_ = Multiply(matRotDelta, matRot_);

		//	}
		//}

	}


	//if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
	//{
	//	// X軸
	//	rotatetion_.x += mouseMoveY * rotateSpeed;

	//	// Y軸
	//	rotatetion_.y += mouseMoveX * rotateSpeed;

	//	
	//}



	if (GetAsyncKeyState('Q') & 0x8000) {
		const float speed = 0.5f;

		Vector3 move = { 0,0,speed };
		Vector3 scale_ = { 1.0f, 1.0f, 1.0f };
		Matrix4x4 translationMatrix = MakeAffineMatrix(scale_, { 0,0,0 }, translatetion_);
		Matrix4x4 rotateMatrix = MakeAffineMatrix(scale_, { 0,0,0 }, translatetion_);
		Matrix4x4 scaleMatrix = MakeAffineMatrix(scale_, { 0,0,0 }, translatetion_);


		//worldMatrix_ = MakeAffineMatrix(scale_, { 0,0,0 }, translatetion_);



		translatetion_.x += move.x;
		translatetion_.y += move.y;
		translatetion_.z += move.z;


	}
	if (GetAsyncKeyState('W') & 0x8000) {
		const float speed = 0.5f;

		Vector3 move = { 0,-speed,0 };
		Vector3 scale_ = { 1.0f, 1.0f, 1.0f };
		Matrix4x4 translationMatrix = MakeAffineMatrix(scale_, { 0,0,0 }, translatetion_);
		Matrix4x4 rotateMatrix = MakeAffineMatrix(scale_, { 0,0,0 }, translatetion_);
		Matrix4x4 scaleMatrix = MakeAffineMatrix(scale_, { 0,0,0 }, translatetion_);

	

		//worldMatrix_ = MakeAffineMatrix(scale_, { 0,0,0 }, translatetion_);



		translatetion_.x += move.x;
		translatetion_.y += move.y;
		translatetion_.z += move.z;



	}
	if (GetAsyncKeyState('D') & 0x8000) {
		const float speed = 0.5f;

		Vector3 move = { -speed,0,0 };
		Vector3 scale_ = { 1.0f, 1.0f, 1.0f };
		Matrix4x4 translationMatrix = MakeAffineMatrix(scale_, { 0,0,0 }, translatetion_);
		Matrix4x4 rotateMatrix = MakeAffineMatrix(scale_, { 0,0,0 }, translatetion_);
		Matrix4x4 scaleMatrix = MakeAffineMatrix(scale_, { 0,0,0 }, translatetion_);



		//worldMatrix_ = MakeAffineMatrix(scale_, { 0,0,0 }, translatetion_);



		translatetion_.x += move.x;
		translatetion_.y += move.y;
		translatetion_.z += move.z;



	}
	if (GetAsyncKeyState('A') & 0x8000) {
		const float speed = 0.5f;

		Vector3 move = { speed,0,0 };
		Vector3 scale_ = { 1.0f, 1.0f, 1.0f };
		Matrix4x4 translationMatrix = MakeAffineMatrix(scale_, { 0,0,0 }, translatetion_);
		Matrix4x4 rotateMatrix = MakeAffineMatrix(scale_, { 0,0,0 }, translatetion_);
		Matrix4x4 scaleMatrix = MakeAffineMatrix(scale_, { 0,0,0 }, translatetion_);

	

		//worldMatrix_ = MakeAffineMatrix(scale_, { 0,0,0 }, translatetion_);



		translatetion_.x += move.x;
		translatetion_.y += move.y;
		translatetion_.z += move.z;
	}
	if (GetAsyncKeyState('S') & 0x8000) {
		const float speed = 0.5f;

		Vector3 move = { 0,speed,0 };
		Vector3 scale_ = { 1.0f, 1.0f, 1.0f };
		Matrix4x4 translationMatrix = MakeAffineMatrix(scale_, { 0,0,0 }, translatetion_);
		Matrix4x4 rotateMatrix = MakeAffineMatrix(scale_, { 0,0,0 }, translatetion_);
		Matrix4x4 scaleMatrix = MakeAffineMatrix(scale_, { 0,0,0 }, translatetion_);

		

		//worldMatrix_ = MakeAffineMatrix(scale_, { 0,0,0 }, translatetion_);



		translatetion_.x += move.x;
		translatetion_.y += move.y;
		translatetion_.z += move.z;

	}
	if (GetAsyncKeyState('E') & 0x8000) {
		const float speed = 0.5f;

		Vector3 move = { 0,0,-speed };
		Vector3 scale_ = { 1.0f, 1.0f, 1.0f };
		Matrix4x4 translationMatrix = MakeAffineMatrix(scale_, { 0,0,0 }, translatetion_);
		Matrix4x4 rotateMatrix = MakeAffineMatrix(scale_, { 0,0,0 }, translatetion_);
		Matrix4x4 scaleMatrix = MakeAffineMatrix(scale_, { 0,0,0 }, translatetion_);

	

		 //worldMatrix_ = MakeAffineMatrix(scale_, { 0,0,0 }, translatetion_);

		translatetion_.x += move.x;
		translatetion_.y += move.y;
		translatetion_.z += move.z;
	}

	//座標から平行移動行列を計算する

	//座標から平行移動行列を計算する

	Vector3 scale_ = { 1.0f, 1.0f, 1.0f };
	//Matrix4x4 matRotDelta = MakeIdentity4x4();
	//
	//matRotDelta = Multiply(matRotDelta, MakeRotateXMatrix(mouseDelta.x));
	//
	//matRotDelta = Multiply(matRotDelta, MakeRotateYMatrix(mouseDelta.y));
	//
	//matRot_ = Multiply(matRotDelta, matRot_);

	//Matrix4x4 translationMatrix = MakeAffineMatrix(scale_, rotation_, translatetion_);
	//Matrix4x4 rotateMatrix = MakeAffineMatrix(scale_, rotation_, translatetion_);
	//Matrix4x4 scaleMatrix = MakeAffineMatrix(scale_, rotation_, translatetion_);

	Matrix4x4 translationMatrix = MakeTranslateMatrix(translatetion_);
	worldMatrix_ = Multiply(Multiply(MakeIdentity4x4(), matRot_), translationMatrix);
	viewMatrix_ = Inverse(worldMatrix_);
	Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, 720.0f / 1280.0f, 0.1f, 100.0f);
}

