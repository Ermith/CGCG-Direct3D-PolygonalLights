#include "Window.h"
#include "Graphics.h"
#include <Windows.h>

#define WIDTH 800
#define HEIGHT 600


class Keyboard {
private:
	static bool keys[255];
public:
	static void PressKey(char key) { keys[key] = true; }
	static void ReleaseKey(char key) { keys[key] = false; }
	static bool IsPressed(char key) { return keys[key]; }
};

bool Keyboard::keys[255];


struct Camera {
	dx::XMFLOAT3 Position = { -4, 1, -4 };
	dx::XMFLOAT3 Rotation = { 0, 0, 0 };

	void Update() {
		dx::XMFLOAT3 cameraMove = { 0, 0, 0 };
		if (Keyboard::IsPressed('W')) cameraMove.z += 0.1f;
		if (Keyboard::IsPressed('A')) cameraMove.x -= 0.1f;
		if (Keyboard::IsPressed('S')) cameraMove.z -= 0.1f;
		if (Keyboard::IsPressed('D')) cameraMove.x += 0.1f;
		if (Keyboard::IsPressed('F')) cameraMove.y -= 0.1f;
		if (Keyboard::IsPressed('R')) cameraMove.y += 0.1f;
		if (Keyboard::IsPressed('I')) Rotation.x -= 0.01f;
		if (Keyboard::IsPressed('K')) Rotation.x += 0.01f;
		if (Keyboard::IsPressed('J')) Rotation.y -= 0.01f;
		if (Keyboard::IsPressed('L')) Rotation.y += 0.01f;

		dx::XMMATRIX translation = dx::XMMatrixTranslation(cameraMove.x, cameraMove.y, cameraMove.z);
		dx::XMMATRIX rotation = dx::XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z);
		dx::XMVECTOR movement = dx::XMVector3Transform(
			dx::XMLoadFloat3(&cameraMove),
			translation * rotation
		);

		dx::XMStoreFloat3(&cameraMove, movement);
		Position.x += cameraMove.x;
		Position.y += cameraMove.y;
		Position.z += cameraMove.z;
	}
};


LRESULT CALLBACK window_callback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg == WM_KEYDOWN) Keyboard::PressKey(wParam);
	if (uMsg == WM_KEYUP) Keyboard::ReleaseKey(wParam);

	if (uMsg == WM_DESTROY) {
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam); // default procedure
}



int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow
) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	try {
		Window wnd(hInstance, nCmdShow, WIDTH, HEIGHT, window_callback);
		Graphics gr(wnd.GetHandle(), WIDTH, HEIGHT);
		Camera camera;

		// LIGHTS
		{
			/*/
			gr.AddDirLight(
				{ 1, 1, 1 },   // color
				{ 1, -1, -2 }, // direction
				1			   // intensity
			);
			//*/

			/*/
			gr.AddSpotLight(
				{ 4, 2, 7 },  // position
				{ 0, 1, 0 },  // color
				{-2, -1, -1}, // direction
				20,			  // intensity
				0.7f, 0.75f	  // inner/outer cone
			);
			gr.AddSpotLight(
				{ -4, 2, 7 },	// position
				{ 0, 0, 1 },	// color
				{ 2, -1, -1 },	// direction
				20,				// intensity
				0.7f, 0.75f		// inner/outer cone
			);
			//*/

			/*/
			gr.AddPointLight(
				{ 4, 2, 7 }, // position
				{0, 1, 1},	 // color
				4			 // intensity
			);
			//*/

			//*/
			gr.AddRectLight(
				{ 4, 0.3, 5 }, // position
				{ 1, 1, 1 },   // color
				4,			   // intensity
				1, 1,          // width, height
				0.0, 0.5	   // rotationX, rotationY
			);
			//*/
		}

		dx::XMFLOAT3 cubeLocation = { 0, 0, 4 };
		dx::XMFLOAT3 cubeRotation = { 0, 0, 0 };
		dx::XMMATRIX cubeTransform = dx::XMMatrixIdentity();

		while (true) {

			if (auto wParam = wnd.PollEvents())
				return (int)wParam.value();
			
			// Update
			{
				camera.Update();
				//gr.GetRectLight(0)->Params.w += 0.001;
				gr.GetRectLight(0)->Params.z += 0.001;
				cubeRotation.y += 0.01;
				cubeTransform =
					dx::XMMatrixRotationRollPitchYaw(cubeRotation.x, cubeRotation.y, cubeRotation.z)
					* dx::XMMatrixTranslation(cubeLocation.x, cubeLocation.y, cubeLocation.z);
			}

			// Render
			{
				gr.Clear(DirectX::Colors::Gray);

				vector<Vertex> vBuffer;
				vector<unsigned short> iBuffer;
				gr.FillFloor(vBuffer, iBuffer, dx::XMMatrixIdentity());
				gr.FillCube(vBuffer, iBuffer, cubeTransform);
				gr.DrawTriangles(vBuffer, iBuffer, camera.Position, camera.Rotation);

				vBuffer.clear();
				iBuffer.clear();
				gr.SwapBuffers();
			}
		}
	}
	catch (...) {
		return -1;
	}

	return 0;
}
