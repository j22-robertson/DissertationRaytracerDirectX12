#include "CameraController.h"

std::vector<DirectX::XMMATRIX> CameraController::GetCameraData(int width, int height)
{

	view = DirectX::XMMatrixLookAtRH(camPosition, lookAt, Up);



	cameraUploadData[0] = view;

	float fovAngleY = 45.0f * DirectX::XM_PI / 180.0f;

	float aspectRatio = static_cast<float>(width) / static_cast<float>(height);


	cameraUploadData[1] = DirectX::XMMatrixPerspectiveFovRH(fovAngleY, aspectRatio, 0.1f, 1000.0f);

	DirectX::XMVECTOR det;

	cameraUploadData[2] = DirectX::XMMatrixInverse(&det, cameraUploadData[0]);
	cameraUploadData[3] = DirectX::XMMatrixInverse(&det, cameraUploadData[1]);

	return cameraUploadData;
}
