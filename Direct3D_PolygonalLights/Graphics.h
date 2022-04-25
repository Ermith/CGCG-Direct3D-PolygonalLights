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


#define LIGHT_BUFFER_SIZE 10

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

	float nx;
	float ny;
	float nz;
};

struct PointLight {
	dx::XMFLOAT4 Position;
	dx::XMFLOAT4 Color;
};

struct DirLight {
	dx::XMFLOAT4 Direction;
	dx::XMFLOAT4 Color;
};

struct SpotLight {
	dx::XMFLOAT4 Position;
	dx::XMFLOAT4 Direction;
	dx::XMFLOAT4 Color;
	dx::XMFLOAT4 InnerCone;
	dx::XMFLOAT4 OuterCone;
};

class Graphics {
public:
	Graphics(HWND hWnd, FLOAT width, FLOAT height);
	void Clear(const FLOAT colorRGBA[4]);
	void SwapBuffers();

	template<class V>
	void DrawTriangles(const vector<V>& vBuffer, const vector<unsigned short>& iBuffer, dx::XMFLOAT3 cameraPos, dx::XMFLOAT3 cameraDir);

	template<class V>
	void FillTriangle(vector<V>& vBuffer, vector<unsigned short>& iBuffer);

	template<class V>
	void FillCubeShared(vector<V>& vBuffer, vector<unsigned short>& iBuffer);

	template<class V>
	void FillCube(vector<V>& vBuffer, vector<unsigned short>& iBuffer);
private:
	struct VSConstantBuffer {
		dx::XMMATRIX modelToWorld;
		dx::XMMATRIX worldToView;
		dx::XMMATRIX projection;
		dx::XMMATRIX normalTransform;
	};

	struct PSConstantBuffer {
		dx::XMFLOAT4 viewPos;
		dx::XMINT4 lightCounts = { 0, 0, 0, 0 };
		PointLight pointLights[LIGHT_BUFFER_SIZE];
		SpotLight spotLights[LIGHT_BUFFER_SIZE];
		DirLight dirLights[LIGHT_BUFFER_SIZE];
	};


	ComPtr<ID3D11Device> _pDevice;
	ComPtr<ID3D11DeviceContext> _pContext;
	ComPtr<IDXGISwapChain> _pSwapChain;
	ComPtr<ID3D11RenderTargetView> _pRTView;
	ComPtr<ID3D11Buffer> _pVSConstantBuffer;
	ComPtr<ID3D11Buffer> _pPSConstantBuffer;
	PSConstantBuffer psConstantBuffer;
	VSConstantBuffer vsConstantBuffer;
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
};


#pragma region PublicMethods

Graphics::Graphics(HWND hWnd, FLOAT width, FLOAT height)
	: _width(width), _height(height)
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

template<class V>
void Graphics::DrawTriangles(const vector<V>& vBuffer, const vector<unsigned short>& iBuffer, dx::XMFLOAT3 cameraPos, dx::XMFLOAT3 cameraDir) {

	// Bind Vertex Buffer
	//========================================
	{
		ComPtr<ID3D11Buffer> pVertexBuffer;

		D3D11_BUFFER_DESC bd = {};
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.CPUAccessFlags = 0u;
		bd.MiscFlags = 0u;
		bd.ByteWidth = sizeof(V) * vBuffer.size();
		bd.StructureByteStride = sizeof(V);

		D3D11_SUBRESOURCE_DATA sd = {};
		sd.pSysMem = vBuffer.data();

		_pDevice->CreateBuffer(&bd, &sd, &pVertexBuffer);
		const UINT stride = sizeof(V);
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
		bd.ByteWidth = sizeof(unsigned short) * iBuffer.size();
		bd.StructureByteStride = sizeof(unsigned short);

		D3D11_SUBRESOURCE_DATA sd = {};
		sd.pSysMem = iBuffer.data();

		_pDevice->CreateBuffer(&bd, &sd, &pIndexBuffer);
		_pContext->IASetIndexBuffer(pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0u);
	}

	// Update VS Constant Buffer
	//========================================
	{
		vsConstantBuffer.modelToWorld = dx::XMMatrixTranspose(dx::XMMatrixTranslation(0, 0, 4));
		vsConstantBuffer.worldToView = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(dx::XMLoadFloat3(&cameraPos), dx::XMLoadFloat3(&cameraDir), { 0,1,0 }));
		vsConstantBuffer.projection = dx::XMMatrixTranspose(dx::XMMatrixPerspectiveLH(1.0f, _height / _width, 0.5f, 500.0f));
		vsConstantBuffer.normalTransform = dx::XMMatrixTranspose(dx::XMMatrixInverse(nullptr, vsConstantBuffer.modelToWorld));

		// Update the constant buffer.
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		_pContext->Map(
			_pVSConstantBuffer.Get(),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		memcpy(mappedResource.pData, &vsConstantBuffer,
			sizeof(vsConstantBuffer));
		_pContext->Unmap(_pVSConstantBuffer.Get(), 0);
	}

	// Update PS Constant Buffer
	{
		psConstantBuffer.viewPos = dx::XMFLOAT4{ cameraPos.x, cameraPos.y, cameraPos.z, 1 };

		// Update the constant buffer.
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		_pContext->Map(
			_pPSConstantBuffer.Get(),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);

		memcpy(mappedResource.pData, &psConstantBuffer,
			sizeof(psConstantBuffer));
		_pContext->Unmap(_pPSConstantBuffer.Get(), 0);
	}

	_pContext->DrawIndexed(iBuffer.size(), 0u, 0u);
}

template<>
void Graphics::FillTriangle(vector<Vertex>& vBuffer, vector<unsigned short>& iBuffer) {

	unsigned short offset = vBuffer.size();

	vBuffer.push_back({ 0.0f,0.5f, 0.0f , 1.0f, 0.0f, 0.0f });
	vBuffer.push_back({ 0.5f,-0.5f, 0.0f ,0.0f, 1.0f, 0.0f });
	vBuffer.push_back({ -0.5f,-0.5f, 0.0f ,0.0f, 0.0f, 1.0f });

	iBuffer.push_back(offset + 0u);
	iBuffer.push_back(offset + 1u);
	iBuffer.push_back(offset + 2u);
}

template<>
void Graphics::FillCubeShared(vector<Vertex>& vBuffer, vector<unsigned short>& iBuffer) {
	unsigned short offset = vBuffer.size();

	vBuffer.push_back({ -1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f });
	vBuffer.push_back({ 1.0f,-1.0f, -1.0f,	0.0f, 1.0f, 0.0f });
	vBuffer.push_back({ -1.0f,1.0f, -1.0f,  0.0f, 0.0f, 1.0f });
	vBuffer.push_back({ 1.0f,1.0f, -1.0f,	1.0f, 1.0f, 0.0f });
	vBuffer.push_back({ -1.0f,-1.0f, 1.0f,	1.0f, 0.0f, 1.0f });
	vBuffer.push_back({ 1.0f,-1.0f, 1.0f,  0.0f, 1.0f, 1.0f });
	vBuffer.push_back({ -1.0f,1.0f, 1.0f,	0.0f, 0.0f, 0.0f });
	vBuffer.push_back({ 1.0f,1.0f, 1.0f,	1.0f, 1.0f, 1.0f });

	const unsigned short indices[] = {
		0,2,1, 2,3,1,
		1,3,5, 3,7,5,
		2,6,3, 3,6,7,
		4,5,7, 4,7,6,
		0,4,2, 2,4,6,
		0,1,4, 1,5,4
	};

	for (unsigned short index : indices)
		iBuffer.push_back(offset + index);
}

template<>
void Graphics::FillCube(vector<Vertex>& vBuffer, vector<unsigned short>& iBuffer) {
	unsigned short offset = vBuffer.size();

	float red[3] = { 1.0f, 0.0f, 0.0f };

	// BACK - green
	vBuffer.push_back({ -1.0f,-1.0f, 1.0f,	0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 1.0f });
	vBuffer.push_back({ 1.0f,-1.0f, 1.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 1.0f });
	vBuffer.push_back({ -1.0f,1.0f, 1.0f,	0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 1.0f });
	vBuffer.push_back({ 1.0f,1.0f, 1.0f,	0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 1.0f });

	// LEFT - magenta
	vBuffer.push_back({ -1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 1.0f,   -1.0f, 0.0f, 0.0f });
	vBuffer.push_back({ -1.0f,1.0f, -1.0f,  1.0f, 0.0f, 1.0f,   -1.0f, 0.0f, 0.0f });
	vBuffer.push_back({ -1.0f,-1.0f, 1.0f,	1.0f, 0.0f, 1.0f,   -1.0f, 0.0f, 0.0f });
	vBuffer.push_back({ -1.0f,1.0f, 1.0f,	1.0f, 0.0f, 1.0f,   -1.0f, 0.0f, 0.0f });

	// RIGHT - cyan
	vBuffer.push_back({ 1.0f,-1.0f, -1.0f,	0.0f, 1.0f, 1.0f,   1.0f, 0.0f, 0.0f });
	vBuffer.push_back({ 1.0f,1.0f, -1.0f,	0.0f, 1.0f, 1.0f,   1.0f, 0.0f, 0.0f });
	vBuffer.push_back({ 1.0f,-1.0f, 1.0f,  0.0f, 1.0f, 1.0f,   1.0f, 0.0f, 0.0f });
	vBuffer.push_back({ 1.0f,1.0f, 1.0f,	0.0f, 1.0f, 1.0f,   1.0f, 0.0f, 0.0f });

	// TOP - blue
	vBuffer.push_back({ -1.0f,1.0f, -1.0f,  0.0f, 0.0f, 1.0f,   0.0f, 1.0f, 0.0f });
	vBuffer.push_back({ 1.0f,1.0f, -1.0f,	0.0f, 0.0f, 1.0f,   0.0f, 1.0f, 0.0f });
	vBuffer.push_back({ -1.0f,1.0f, 1.0f,	0.0f, 0.0f, 1.0f,   0.0f, 1.0f, 0.0f });
	vBuffer.push_back({ 1.0f,1.0f, 1.0f,	0.0f, 0.0f, 1.0f,   0.0f, 1.0f, 0.0f });

	// BOTTOM - yellow
	vBuffer.push_back({ -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 0.0f,   0.0f, -1.0f, 0.0f });
	vBuffer.push_back({ 1.0f,-1.0f, -1.0f,	1.0f, 1.0f, 0.0f,   0.0f, -1.0f, 0.0f });
	vBuffer.push_back({ -1.0f,-1.0f, 1.0f,	1.0f, 1.0f, 0.0f,   0.0f, -1.0f, 0.0f });
	vBuffer.push_back({ 1.0f,-1.0f, 1.0f,  1.0f, 1.0f, 0.0f,   0.0f, -1.0f, 0.0f });

	// FRONT - red
	vBuffer.push_back({ -1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f,  0.0f, 0.0f, -1.0f });
	vBuffer.push_back({ 1.0f,-1.0f, -1.0f,	1.0f, 0.0f, 0.0f,  0.0f, 0.0f, -1.0f });
	vBuffer.push_back({ -1.0f,1.0f, -1.0f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f, -1.0f });
	vBuffer.push_back({ 1.0f,1.0f, -1.0f,	1.0f, 0.0f, 0.0f,  0.0f, 0.0f, -1.0f });


	const unsigned short indices[] = {
		0,1,2,    2,1,3,	
		4,6,5,	  6,7,5,
		8,9,10,   10,9,11,	
		12,14,13, 14,15,13, 
		16,17,18, 18,17,19, 
		20,22,21, 22,23,21  
	};

	for (unsigned short index : indices)
		iBuffer.push_back(offset + index);
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
	{
		ComPtr<ID3D11PixelShader> pPixelShader;
		CHECKED(D3DReadFileToBlob(L"PixelShader.cso", &blobBuffer), "Reading PShader fucked up");
		CHECKED(_pDevice->CreatePixelShader(blobBuffer->GetBufferPointer(), blobBuffer->GetBufferSize(), nullptr, &pPixelShader), "PShader creation fucked up");
		_pContext->PSSetShader(pPixelShader.Get(), nullptr, 0u);

		// bind view position to pixel shader
		psConstantBuffer.viewPos = {0, 0, 0, 1};
		psConstantBuffer.lightCounts.y = 1;
		psConstantBuffer.spotLights[0].Color = dx::XMFLOAT4{1, 1, 1, 1};
		psConstantBuffer.spotLights[0].Position = dx::XMFLOAT4{ 5, 5, 0, 1 };
		psConstantBuffer.spotLights[0].Direction = dx::XMFLOAT4{ -1, -1, 1, 1};
		psConstantBuffer.spotLights[0].InnerCone = dx::XMFLOAT4(cos(dx::XMConvertToRadians(30.0f)), 0.0f, 0.0f, 0.0f);
		psConstantBuffer.spotLights[0].OuterCone = dx::XMFLOAT4(cos(dx::XMConvertToRadians(34.0f)), 0.0f, 0.0f, 0.0f);

		//psConstantBuffer.lightCounts.z = 1;
		//psConstantBuffer.dirLights[0].Direction = dx::XMFLOAT4(1, 1, -1, 1);
		//psConstantBuffer.dirLights[0].Color = dx::XMFLOAT4(1, 1, 1, 1);
		
		//psConstantBuffer.lightCounts.x = 1;
		//psConstantBuffer.pointLights[0].Position = dx::XMFLOAT4{ 2, 4, 0, 1 };
		//psConstantBuffer.pointLights[0].Color = dx::XMFLOAT4{ 1, 1, 1, 1 };
		//psConstantBuffer.pointLights[1].Position = dx::XMFLOAT4{ -2, 0, 0, 1 };
		//psConstantBuffer.pointLights[1].Color = dx::XMFLOAT4{ 1, 1, 1, 1 };

		D3D11_BUFFER_DESC bd = {};
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; //| D3D11_CPU_ACCESS_READ;
		bd.MiscFlags = 0u;
		bd.ByteWidth = sizeof(psConstantBuffer);
		bd.StructureByteStride = 0u;

		D3D11_SUBRESOURCE_DATA sd = {};
		sd.pSysMem = &psConstantBuffer;

		HRESULT hr = _pDevice->CreateBuffer(&bd, &sd, &_pPSConstantBuffer);
		_pContext->PSSetConstantBuffers(0u, 1u, _pPSConstantBuffer.GetAddressOf());
	}

	// create vertex shader
	{
		ComPtr<ID3D11VertexShader> pVertexShader;
		CHECKED(D3DReadFileToBlob(L"VertexShader.cso", &blobBuffer), "Reading VSHader fucked up");
		CHECKED(_pDevice->CreateVertexShader(blobBuffer->GetBufferPointer(), blobBuffer->GetBufferSize(), nullptr, &pVertexShader), "VShader creation fucked up");
		_pContext->VSSetShader(pVertexShader.Get(), nullptr, 0u);

		// bind transformation matrix to vertex shader
		vsConstantBuffer.modelToWorld = dx::XMMatrixTranspose(
			dx::XMMatrixScaling(_width / (float)_height, 1.0f, 1.0f)
		);

		D3D11_BUFFER_DESC bd = {};
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0u;
		bd.ByteWidth = sizeof(VSConstantBuffer);
		bd.StructureByteStride = 0u;

		D3D11_SUBRESOURCE_DATA sd = {};
		sd.pSysMem = &vsConstantBuffer;

		_pDevice->CreateBuffer(&bd, &sd, &_pVSConstantBuffer);
		_pContext->VSSetConstantBuffers(0u, 1u, _pVSConstantBuffer.GetAddressOf());
	}

}

void Graphics::CreateLayoutAndTopology(ComPtr<ID3DBlob> blobBuffer) {
	// input (vertex) layout (2d position only)
	ComPtr<ID3D11InputLayout> pInputLayout;
	const D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{ "Position",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0 },
		{ "Color",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12u,D3D11_INPUT_PER_VERTEX_DATA,0 },
		{ "Normal",0,DXGI_FORMAT_R32G32B32_FLOAT,0,24u,D3D11_INPUT_PER_VERTEX_DATA,0 }
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
