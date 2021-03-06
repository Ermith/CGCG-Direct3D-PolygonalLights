#include "Window.h"

Window::Window(
	HINSTANCE hInstance,
	int nCmdShow,
	LONG width, LONG height,
	WNDPROC proc,
	LPCWSTR windowName,
	LPCWSTR className
) :
	_className(className),
	_hInstance(hInstance),
	_width(width),
	_height(height)
{
	WNDCLASSEX wc;
	wc.lpszClassName = className;
	wc.hInstance = hInstance;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = proc;
	wc.style = CS_HREDRAW | CS_VREDRAW;

	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = 0;
	wc.hCursor = 0;
	wc.hIcon = 0;
	wc.hIconSm = 0;
	wc.lpszMenuName = 0;

	if (!RegisterClassEx(&wc)) throw windowException("RegisterClassEx fucked up");

	DWORD dwStyle = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	RECT rc = { 0, 0, width, height };
	AdjustWindowRect(&rc, dwStyle, false);
	_hWnd = CreateWindow(
		className, windowName,
		dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT,
		rc.right - rc.left, rc.bottom - rc.top,
		nullptr, nullptr,
		hInstance,
		nullptr);

	if (!_hWnd) throw windowException("CreateWindow fucked up");

	ShowWindow(_hWnd, nCmdShow);
}

optional<WPARAM> Window::PollEvents() {
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if (msg.message == WM_QUIT)
			return msg.wParam;
	}

	return nullopt;
}