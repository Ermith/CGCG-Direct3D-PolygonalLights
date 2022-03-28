#include <Windows.h>
#include <d3d11_1.h>
#include <directxcolors.h>

HINSTANCE g_hInstance;
HWND g_hWnd;

LRESULT CALLBACK window_callback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg == WM_DESTROY) {
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam); // default procedure
}

HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow) {
	// Create and register window class

	WNDCLASSEX wc;
	LPCWSTR className = L"DXClass";

	wc.lpszClassName = className;
	wc.hInstance = hInstance;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = window_callback;
	wc.style = CS_HREDRAW | CS_VREDRAW;

	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = 0;
	wc.hCursor = 0;
	wc.hIcon = 0;
	wc.hIconSm = 0;
	wc.lpszMenuName = 0;

	if (!RegisterClassEx(&wc))
		return E_FAIL;

	// Create and show the window
	DWORD dwStyle = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	g_hInstance = hInstance;
	RECT rc = { 0, 0, 800, 600 };
	AdjustWindowRect(&rc, dwStyle, false);
	g_hWnd = CreateWindow(className, L"DXWindow",
		dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT,
		rc.right - rc.left, rc.bottom - rc.top,
		nullptr, nullptr,
		hInstance,
		nullptr);

	if (!g_hWnd)
		return E_FAIL;

	ShowWindow(g_hWnd, nCmdShow);

	return S_OK;
}

HRESULT InitDirect3D() {

}

bool PollEvents(MSG* msg) {
	while (PeekMessage(msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(msg);
		DispatchMessage(msg);

		if (msg->message == WM_QUIT)
			return false;
	}

	return true;
}

int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow
) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (FAILED(InitWindow(hInstance, nCmdShow)))
		return 0;

	if (FAILED(InitDirect3D()))
		return 0;


	MSG msgBuffer = {0};
	while (PollEvents(&msgBuffer)) {

		// DO STUFF

	}

	return (int)msgBuffer.wParam;
}