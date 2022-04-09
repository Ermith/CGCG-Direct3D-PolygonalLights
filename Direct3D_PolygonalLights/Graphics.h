#pragma once

#include <d3d11_1.h>
#include <dxgiformat.h>
#include <directxcolors.h>
#include <wrl.h>
#include <d3dcompiler.h>
#include <exception>
#include <Windows.h>
#include <vector>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib")

#define CHECKED(expr, message) if(FAILED(expr)) throw graphicsException(message);

using Microsoft::WRL::ComPtr;
using std::exception;
using std::vector;


struct Vertex {
	float x;
	float y;
	float z;

	float r;
	float g;
	float b;
};

class Graphics {
public:
	Graphics(HWND hWnd, FLOAT width, FLOAT height);
	void Clear(const FLOAT colorRGBA[4]);
	void SwapBuffers();
	void DrawTriangles(const vector<Vertex>& vBuffer) {
		const Vertex* vertices = vBuffer.data();

		ComPtr<ID3D11Buffer> pVertexBuffer;
		D3D11_BUFFER_DESC bd = {};
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.CPUAccessFlags = 0u;
		bd.MiscFlags = 0u;
		bd.ByteWidth = sizeof(Vertex) * vBuffer.size();
		bd.StructureByteStride = sizeof(Vertex);
		D3D11_SUBRESOURCE_DATA sd = {};
		sd.pSysMem = vertices;
		_pDevice->CreateBuffer(&bd, &sd, &pVertexBuffer);

		// Bind vertex buffer to pipeline
		const UINT stride = sizeof(Vertex);
		const UINT offset = 0u;
		_pContext->IASetVertexBuffers(0u, 1u, pVertexBuffer.GetAddressOf(), &stride, &offset);


		_pContext->Draw(3u, 0u);
	}
	void FillTriangle(vector<Vertex>& vBuffer) {
		vBuffer.push_back({ 0.0f,0.5f, 0.0f,  1.0f, 0.0f, 0.0f});
		vBuffer.push_back({ 0.5f,-0.5f, 0.0f,  0.0f, 1.0f, 0.0f});
		vBuffer.push_back({ -0.5f,-0.5f, 0.0f,  0.0f, 0.0f, 1.0f});
	}
private:
	ComPtr<ID3D11Device> _pDevice;
	ComPtr<ID3D11DeviceContext> _pContext;
	ComPtr<IDXGISwapChain> _pSwapChain;
	ComPtr<ID3D11RenderTargetView> _pRTView;
	FLOAT _width;
	FLOAT _height;

	class graphicsException : public exception {
	private:
		const char* _message;
	public:
		graphicsException(const char* message) : _message(message) {}
		const char* what() const throw () { return _message; }
	};

	void CreateDeviceAndSwapChain(HWND hWnd) {
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
		sd.OutputWindow = hWnd;
		sd.Windowed = TRUE;
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		sd.Flags = 0;

		if (FAILED(D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			0,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			&sd,
			&_pSwapChain,
			&_pDevice,
			nullptr,
			&_pContext
		)))
			throw graphicsException("Device and Swapchain fucked up");
	}
	void CreateRenderTargetView() {
		ComPtr<ID3D11Resource> pBackBuffer;
		if (FAILED(_pSwapChain->GetBuffer(0, __uuidof(ID3D11Resource), &pBackBuffer)))
			throw graphicsException("Backbuffer fucked up");

		if (FAILED(_pDevice->CreateRenderTargetView(
			pBackBuffer.Get(),
			nullptr,
			&_pRTView
		)))
			throw graphicsException("RenderTargetView fucked up");

		// bind render target
		_pContext->OMSetRenderTargets(1u, _pRTView.GetAddressOf(), nullptr);
	}
	void LoadShaders(ComPtr<ID3DBlob>& blobBuffer) {
		ComPtr<ID3D11PixelShader> pPixelShader;
		CHECKED(D3DReadFileToBlob(L"PixelShader.cso", &blobBuffer), "Reading PShader fucked up");
		CHECKED(_pDevice->CreatePixelShader(blobBuffer->GetBufferPointer(), blobBuffer->GetBufferSize(), nullptr, &pPixelShader), "PShader creation fucked up");
		_pContext->PSSetShader(pPixelShader.Get(), nullptr, 0u);

		// create vertex shader
		ComPtr<ID3D11VertexShader> pVertexShader;
		CHECKED(D3DReadFileToBlob(L"VertexShader.cso", &blobBuffer), "Reading VSHader fucked up");
		CHECKED(_pDevice->CreateVertexShader(blobBuffer->GetBufferPointer(), blobBuffer->GetBufferSize(), nullptr, &pVertexShader), "VShader creation fucked up");
		_pContext->VSSetShader(pVertexShader.Get(), nullptr, 0u);
	}
	void CreateLayoutAndTopology(ComPtr<ID3DBlob> blobBuffer) {
		// input (vertex) layout (2d position only)
		ComPtr<ID3D11InputLayout> pInputLayout;
		const D3D11_INPUT_ELEMENT_DESC ied[] =
		{
			{ "Position",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0 },
			{ "Color",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12u,D3D11_INPUT_PER_VERTEX_DATA,0 },
		};

		CHECKED(_pDevice->CreateInputLayout(
			ied, (UINT)(sizeof(ied) / sizeof(ied[0])),
			blobBuffer->GetBufferPointer(),
			blobBuffer->GetBufferSize(),
			&pInputLayout
		), "Create Input Layout fucked up");

		// bind vertex layout
		_pContext->IASetInputLayout(pInputLayout.Get());
		// Set primitive topology to triangle list (groups of 3 vertices)
		_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}
	void SetViewPort() {
		D3D11_VIEWPORT vp;
		vp.Width = _width;
		vp.Height = _height;
		vp.MinDepth = 0;
		vp.MaxDepth = 1;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		_pContext->RSSetViewports(1u, &vp);
	}
};

Graphics::Graphics(HWND hWnd, FLOAT width, FLOAT height)
: _width(width), _height(height)
{
	CreateDeviceAndSwapChain(hWnd);
	CreateRenderTargetView();
	ComPtr<ID3DBlob> vertexShader;
	LoadShaders(vertexShader);
	CreateLayoutAndTopology(vertexShader);
	SetViewPort();
}

void Graphics::Clear(const FLOAT colorRGBA[4]) {
	_pContext->ClearRenderTargetView(_pRTView.Get(), colorRGBA);
}

void Graphics::SwapBuffers() {
	_pSwapChain->Present(1u, 0u);
}