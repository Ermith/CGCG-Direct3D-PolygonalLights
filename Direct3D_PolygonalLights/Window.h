#pragma once

#include <Windows.h>
#include <exception>
#include <optional>

using std::exception;
using std::optional;
using std::nullopt;

class Window {
public:
	Window(HINSTANCE hInstance, int nCmdShow, LONG width, LONG height, WNDPROC proc, LPCWSTR windowName = L"DXWindow", LPCWSTR className = L"DXWindow");
	optional<WPARAM> PollEvents();
	HWND GetHandle() { return _hWnd; }
private:
	LPCWSTR _className;
	HINSTANCE _hInstance;
	HWND _hWnd;
	LONG _width;
	LONG _height;

	class windowException : public exception {
	private:
		const char* _message;
	public:
		windowException(const char* message) : _message(message) {}
		const char* what() const throw () { return _message; }
	};
};