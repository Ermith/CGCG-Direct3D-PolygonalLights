#include <Windows.h>
#include <d3d11_1.h>
#include <directxcolors.h>
#include <dxgiformat.h>
#include <wrl.h>

namespace wrl = Microsoft::WRL;
using wrl::ComPtr;

HINSTANCE g_hInstance;
HWND g_hWnd;
ComPtr<ID3D11Device> g_pDevice;
ComPtr<ID3D11DeviceContext> g_pContext;
ComPtr<IDXGISwapChain> g_pSwapChain;
ComPtr<ID3D11RenderTargetView> g_pRenderTarget;

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
	DXGI_SWAP_CHAIN_DESC sd = { 0 };
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 0;
	sd.BufferDesc.RefreshRate.Denominator = 0;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;

	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 1;
	sd.OutputWindow = g_hWnd;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = 0;

	HRESULT result = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0,
		nullptr,
		0,
		D3D11_SDK_VERSION,
		&sd,
		&g_pSwapChain,
		&g_pDevice,
		nullptr,
		&g_pContext
	);

	ComPtr<ID3D11Resource> pBackBuffer;
	result = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Resource), &pBackBuffer);
	result = g_pDevice->CreateRenderTargetView(
		pBackBuffer.Get(),
		nullptr,
		&g_pRenderTarget
	);

	return result;
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
		g_pContext->ClearRenderTargetView(g_pRenderTarget.Get(), DirectX::Colors::Blue);


		// Swap buffers
		g_pSwapChain->Present(1u, 0u);
	}
	return (int)msgBuffer.wParam;
}