#include "Window.h"
#include "Graphics.h"
#include <Windows.h>

#define WIDTH 800
#define HEIGHT 600

dx::XMFLOAT3 cameraPosition = { -4, 1, -4 };
dx::XMFLOAT3 cameraDir = { 0, -1, 1 };


class Keyboard {
private:
	static bool keys[255];
public:
	static void PressKey(char key) { keys[key] = true; }
	static void ReleaseKey(char key) { keys[key] = false; }
	static bool IsPressed(char key) { return keys[key]; }
};

bool Keyboard::keys[255];

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
		//gr.AddDirLight({ 0, 1, 0 }, { 0, -1, 1 }, 2);
		//gr.AddSpotLight({ 2, 1, 0 }, { 1, 1, 0 }, {-1, -1, 0}, 1, 0.5f, 0.5f);
		//gr.AddPointLight({ 2, 1, 1 }, {1, 1, 0}, 1);
		gr.AddRectLight(
			{ 0, 1, 1 }, { 2.5, 1, 5.5 }, 1, 1, 0.0, 0.0, 4);

		dx::XMFLOAT3 cubeLocation = { 0, -1, 4 };
		dx::XMFLOAT3 cubeRotation = { 0, 0, 0 };
		dx::XMMATRIX cubeTransform = dx::XMMatrixIdentity();

		while (true) {

			if (auto wParam = wnd.PollEvents())
				return (int)wParam.value();

			// Update
			{
				if (Keyboard::IsPressed('W')) cameraPosition.y += 0.1f;
				if (Keyboard::IsPressed('A')) cameraPosition.x -= 0.1f;
				if (Keyboard::IsPressed('S')) cameraPosition.y -= 0.1f;
				if (Keyboard::IsPressed('D')) cameraPosition.x += 0.1f;

				gr.GetRectLight(0)->Params.z += 0.001f;
				//cubeLocation.z += 0.01;
				cubeRotation.y += 0.01;
				//cubeRotation.y = .5f*2*3.14;
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
				gr.DrawTriangles(vBuffer, iBuffer, cameraPosition, cameraDir);
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
