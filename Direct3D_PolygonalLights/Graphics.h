#pragma once

#include <d3d11_1.h>
#include <dxgiformat.h>
#include <d3dcompiler.h>
#include <directxcolors.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <exception>
#include <Windows.h>
#include <vector>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib")

#define CHECKED(expr, message) if(FAILED(expr)) throw graphicsException(message);

namespace dx = DirectX;
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
	void DrawTriangles(const vector<Vertex>& vBuffer, const vector<unsigned short>& iBuffer);
	void FillTriangle(vector<Vertex>& vBuffer, vector<unsigned short>& iBuffer);
	void FillCube(vector<Vertex>& vBuffer, vector<unsigned short>& iBuffer);
private:
	ComPtr<ID3D11Device> _pDevice;
	ComPtr<ID3D11DeviceContext> _pContext;
	ComPtr<IDXGISwapChain> _pSwapChain;
	ComPtr<ID3D11RenderTargetView> _pRTView;
	ComPtr<ID3D11Buffer> _pTransform;
	FLOAT angle;
	FLOAT _width;
	FLOAT _height;

	void CreateDeviceAndSwapChain(HWND hWnd);
	void CreateRenderTargetView();
	void BindShaders(ComPtr<ID3DBlob>& blobBuffer);
	void CreateLayoutAndTopology(ComPtr<ID3DBlob> blobBuffer);
	void SetViewPort();

	class graphicsException : public exception {
	private:
		const char* _message;
	public:
		graphicsException(const char* message) : _message(message) {}
		const char* what() const throw () { return _message; }
	};

	struct ConstantBuffer {
		dx::XMMATRIX transform;
	};
};


#pragma region PublicMethods

Graphics::Graphics(HWND hWnd, FLOAT width, FLOAT height)
	: _width(width), _height(height), angle(0.0f)
{
	CreateDeviceAndSwapChain(hWnd);
	CreateRenderTargetView();
	ComPtr<ID3DBlob> vertexShader;
	BindShaders(vertexShader);
	CreateLayoutAndTopology(vertexShader);
	SetViewPort();
}

void Graphics::Clear(const FLOAT colorRGBA[4]) {
	_pContext->ClearRenderTargetView(_pRTView.Get(), colorRGBA);
}

void Graphics::SwapBuffers() {
	_pSwapChain->Present(1u, 0u);
}

void Graphics::DrawTriangles(const vector<Vertex>& vBuffer, const vector<unsigned short>& iBuffer) {

	// Bind Vertex Buffer
	//========================================
	{
		ComPtr<ID3D11Buffer> pVertexBuffer;

		D3D11_BUFFER_DESC bd = {};
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.CPUAccessFlags = 0u;
		bd.MiscFlags = 0u;
		bd.ByteWidth = sizeof(Vertex) * vBuffer.size();
		bd.StructureByteStride = sizeof(Vertex);

		D3D11_SUBRESOURCE_DATA sd = {};
		sd.pSysMem = vBuffer.data();

		_pDevice->CreateBuffer(&bd, &sd, &pVertexBuffer);
		const UINT stride = sizeof(Vertex);
		const UINT offset = 0u;
		_pContext->IASetVertexBuffers(0u, 1u, pVertexBuffer.GetAddressOf(), &stride, &offset);
	}

	// Bind Index Buffer
	//========================================
	{
		ComPtr<ID3D11Buffer> pIndexBuffer;

		D3D11_BUFFER_DESC bd = {};
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.CPUAccessFlags = 0u;
		bd.MiscFlags = 0u;
		bd.ByteWidth = sizeof(unsigned short) * vBuffer.size();
		bd.StructureByteStride = sizeof(unsigned short);

		D3D11_SUBRESOURCE_DATA sd = {};
		sd.pSysMem = iBuffer.data();

		_pDevice->CreateBuffer(&bd, &sd, &pIndexBuffer);
		_pContext->IASetIndexBuffer(pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0u);
	}

	// Update Constant Buffer
	//========================================
	{
		angle += 0.01f;
		const ConstantBuffer cb = {
			dx::XMMatrixTranspose(
				dx::XMMatrixRotationZ(angle) *
				dx::XMMatrixScaling(_height / _width, 1.0f, 1.0f)
			)
		};

		// Update the constant buffer.
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		_pContext->Map(
			_pTransform.Get(),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		memcpy(mappedResource.pData, &cb,
			sizeof(cb));
		_pContext->Unmap(_pTransform.Get(), 0);
	}

	_pContext->DrawIndexed(iBuffer.size(), 0u, 0u);
}

void Graphics::FillTriangle(vector<Vertex>& vBuffer, vector<unsigned short>& iBuffer) {

	unsigned short offset = vBuffer.size();

	vBuffer.push_back({ 0.0f,0.5f, 0.0f,  1.0f, 0.0f, 0.0f });
	vBuffer.push_back({ 0.5f,-0.5f, 0.0f,  0.0f, 1.0f, 0.0f });
	vBuffer.push_back({ -0.5f,-0.5f, 0.0f,  0.0f, 0.0f, 1.0f });


	iBuffer.push_back(offset + 0u);
	iBuffer.push_back(offset + 1u);
	iBuffer.push_back(offset + 2u);
}

void Graphics::FillCube(vector<Vertex>& vBuffer, vector<unsigned short>& iBuffer) {

}

#pragma endregion

#pragma region PrivateMethods
void Graphics::CreateDeviceAndSwapChain(HWND hWnd) {
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

void Graphics::CreateRenderTargetView() {
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

void Graphics::BindShaders(ComPtr<ID3DBlob>& blobBuffer) {
	// create pixel shader
	ComPtr<ID3D11PixelShader> pPixelShader;
	CHECKED(D3DReadFileToBlob(L"PixelShader.cso", &blobBuffer), "Reading PShader fucked up");
	CHECKED(_pDevice->CreatePixelShader(blobBuffer->GetBufferPointer(), blobBuffer->GetBufferSize(), nullptr, &pPixelShader), "PShader creation fucked up");
	_pContext->PSSetShader(pPixelShader.Get(), nullptr, 0u);

	// create vertex shader
	ComPtr<ID3D11VertexShader> pVertexShader;
	CHECKED(D3DReadFileToBlob(L"VertexShader.cso", &blobBuffer), "Reading VSHader fucked up");
	CHECKED(_pDevice->CreateVertexShader(blobBuffer->GetBufferPointer(), blobBuffer->GetBufferSize(), nullptr, &pVertexShader), "VShader creation fucked up");
	_pContext->VSSetShader(pVertexShader.Get(), nullptr, 0u);

	// bind transformation matrix to vertex shader
	//===============================================
	const ConstantBuffer cb = {
		dx::XMMatrixTranspose(
			dx::XMMatrixRotationZ(angle) *
			dx::XMMatrixScaling(_width / (float)_height, 1.0f, 1.0f)
		)
	};

	D3D11_BUFFER_DESC bd = {};
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.MiscFlags = 0u;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.StructureByteStride = 0u;

	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem = &cb;

	_pDevice->CreateBuffer(&bd, &sd, &_pTransform);
	_pContext->VSSetConstantBuffers(0u, 1u, _pTransform.GetAddressOf());
}

void Graphics::CreateLayoutAndTopology(ComPtr<ID3DBlob> blobBuffer) {
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

void Graphics::SetViewPort() {
	D3D11_VIEWPORT vp;
	vp.Width = _width;
	vp.Height = _height;
	vp.MinDepth = 0;
	vp.MaxDepth = 1;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	_pContext->RSSetViewports(1u, &vp);
}
#pragma endregion
