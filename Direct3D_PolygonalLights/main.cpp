#include <Windows.h>
#include <d3d11_1.h>
#include <directxcolors.h>
#include <dxgiformat.h>
#include <wrl.h>
#include <d3dcompiler.h>

namespace wrl = Microsoft::WRL;
using wrl::ComPtr;

HINSTANCE g_hInstance;
HWND g_hWnd;
ComPtr<ID3D11Device> g_pDevice;
ComPtr<ID3D11DeviceContext> g_pContext;
ComPtr<IDXGISwapChain> g_pSwapChain;
ComPtr<ID3D11RenderTargetView> g_pRenderTarget;


#pragma region Windows

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

bool PollEvents(MSG* msg) {
	while (PeekMessage(msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(msg);
		DispatchMessage(msg);

		if (msg->message == WM_QUIT)
			return false;
	}

	return true;
}

#pragma endregion


#pragma region Direct3D

HRESULT CreateDeviceAndSwapChain() {
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

	return D3D11CreateDeviceAndSwapChain(
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
}

HRESULT CreateRenderTargetView() {

	HRESULT result = S_OK;

	ComPtr<ID3D11Resource> pBackBuffer;
	result = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Resource), &pBackBuffer);

	if (FAILED(result))
		return result;

	result = g_pDevice->CreateRenderTargetView(
		pBackBuffer.Get(),
		nullptr,
		&g_pRenderTarget
	);

	return result;
}


HRESULT InitDirect3D() {
	HRESULT result = S_OK;

	result = CreateDeviceAndSwapChain();
	
	if (FAILED(result))
		return result;

	result = CreateRenderTargetView();

	return result;
}

void DrawTriangle() {
	HRESULT hr;

	struct Vertex {
		float x;
		float y;
	};

	// create vertex buffer (1 2d triangle at center of screen)
	const Vertex vertices[] =
	{
		{ 0.0f,0.5f },
		{ 0.5f,-0.5f },
		{ -0.5f,-0.5f },
	};

	ComPtr<ID3D11Buffer> pVertexBuffer;
	D3D11_BUFFER_DESC bd = {};
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.CPUAccessFlags = 0u;
	bd.MiscFlags = 0u;
	bd.ByteWidth = sizeof(vertices);
	bd.StructureByteStride = sizeof(Vertex);
	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem = vertices;
	g_pDevice->CreateBuffer(&bd, &sd, &pVertexBuffer);

	// Bind vertex buffer to pipeline
	const UINT stride = sizeof(Vertex);
	const UINT offset = 0u;
	g_pContext->IASetVertexBuffers(0u, 1u, pVertexBuffer.GetAddressOf(), &stride, &offset);

	// create pixel shader
	ComPtr<ID3D11PixelShader> pPixelShader;
	ComPtr<ID3DBlob> pBlob;
	D3DReadFileToBlob(L"PixelShader.cso", &pBlob);
	g_pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pPixelShader);

	// bind pixel shader
	g_pContext->PSSetShader(pPixelShader.Get(), nullptr, 0u);



	// create vertex shader
	wrl::ComPtr<ID3D11VertexShader> pVertexShader;
	D3DReadFileToBlob(L"VertexShader.cso", &pBlob);
	g_pDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pVertexShader);

	// bind vertex shader
	g_pContext->VSSetShader(pVertexShader.Get(), nullptr, 0u);

	// input (vertex) layout (2d position only)
	ComPtr<ID3D11InputLayout> pInputLayout;
	const D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{ "Position",0,DXGI_FORMAT_R32G32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0 },
	};
	g_pDevice->CreateInputLayout(
		ied, (UINT)(sizeof(ied) / sizeof(ied[0])),
		pBlob->GetBufferPointer(),
		pBlob->GetBufferSize(),
		&pInputLayout
	);

	// bind vertex layout
	g_pContext->IASetInputLayout(pInputLayout.Get());


	// bind render target
	g_pContext->OMSetRenderTargets(1u, g_pRenderTarget.GetAddressOf(), nullptr);


	// Set primitive topology to triangle list (groups of 3 vertices)
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// configure viewport
	D3D11_VIEWPORT vp;
	vp.Width = 800;
	vp.Height = 600;
	vp.MinDepth = 0;
	vp.MaxDepth = 1;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pContext->RSSetViewports(1u, &vp);
	g_pContext->Draw(3u, 0u);
}

#pragma endregion



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


	MSG msgBuffer = { 0 };
	while (PollEvents(&msgBuffer)) {
		// DO STUFF
		g_pContext->ClearRenderTargetView(g_pRenderTarget.Get(), DirectX::Colors::Blue);

		DrawTriangle();

		// Swap buffers
		g_pSwapChain->Present(1u, 0u);
	}
	return (int)msgBuffer.wParam;
}
