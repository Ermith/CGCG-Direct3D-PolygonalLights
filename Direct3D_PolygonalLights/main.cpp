#include "Window.h"
#include "Graphics.h"
#include <Windows.h>

#define WIDTH 800
#define HEIGHT 600

int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow
) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	try {
		Window wnd(hInstance, nCmdShow, WIDTH, HEIGHT);
		Graphics gr(wnd.GetHandle(), WIDTH, HEIGHT);


		while (true) {

			if (auto wParam = wnd.PollEvents())
				return (int)wParam.value();

			// Do stuff
			gr.Clear(DirectX::Colors::Gray);

			vector<Vertex> vBuffer;
			gr.FillTriangle(vBuffer);
			gr.DrawTriangles(vBuffer);
			vBuffer.clear();

			gr.SwapBuffers();
		}
	}
	catch (...) {
		return -1;
	}

	return 0;
}
