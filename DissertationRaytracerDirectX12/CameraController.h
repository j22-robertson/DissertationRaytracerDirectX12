#pragma once
#include <DirectXMath.h>
#include <vector>
//#include <WinUser.h>

class CameraController
{
public:
	void UpdateCamera() {

		std::vector<DirectX::XMMATRIX> data = std::vector<DirectX::XMMATRIX>();
		//data.reserve(4);
		

		//camRotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(camPitch, camYaw, 0);
		



		//At = DirectX::XMVector3TransformCoord(DefaultForward, camRotationMatrix);
		//At = DirectX::XMVector3Normalize(At);

		//DirectX::XMMATRIX tempRotationMatrixY = DirectX::XMMatrixRotationY(camYaw);


		//Right = DirectX::XMVector3TransformCoord(DefaultRight, tempRotationMatrixY);

		//Up = DirectX::XMVector3TransformCoord(Up, tempRotationMatrixY);

		//Forward = DirectX::XMVector3TransformCoord(DefaultForward, tempRotationMatrixY);

		//auto camRight = DirectX::XMVectorScale(Right, moveLeftRight);
//
		//camPosition = DirectX::XMVectorAdd(camPosition, camRight);
		//camPosition = DirectX::XMVectorAdd(camPosition, Forward);

		//moveLeftRight = 0.0;
		//moveBackForward = 0.0;

		//At = DirectX::XMVectorAdd(At, camPosition);





	}

	void MoveForward()
	{
		camPosition.m128_f32[2] += 0.2f;
	}

	void MoveLeft()
	{
		camPosition.m128_f32[1] -= 0.2f;
	}
	void MoveRight()
	{
		camPosition.m128_f32[1] += 0.2f;
	}
	void moveBack()
	{
		camPosition.m128_f32[2] -= 0.05;
	}
	void MoveUp()
	{}


	std::vector<DirectX::XMMATRIX> GetCameraData(int width, int height);


	///DirectX::XMMATRIX* getData();

private:
	/// <summary>
	/// Contains [0]View [1]Projection [2]Inverse View and [3]Inverse Proj 
	/// </summary>
	std::vector<DirectX::XMMATRIX> cameraUploadData = std::vector<DirectX::XMMATRIX>(4);


	DirectX::XMVECTOR DefaultForward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	DirectX::XMVECTOR DefaultRight = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);



	DirectX::XMMATRIX view;

	DirectX::XMVECTOR Up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	DirectX::XMVECTOR lookAt = DirectX::XMVectorSet(0.0,0.0,0.0,0.0); // Look at/ Target Direction
	DirectX::XMVECTOR Forward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	DirectX::XMVECTOR Right = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

	DirectX::XMVECTOR camPosition = DirectX::XMVectorSet(1.5f, 1.5f, 1.5f, 0.0);




	float camYaw = 0.0f;
	float camPitch = 0.0f;
	float moveLeftRight = 0.0f;
	float moveBackForward = 0.0f;

	DirectX::XMMATRIX camRotationMatrix;
};

