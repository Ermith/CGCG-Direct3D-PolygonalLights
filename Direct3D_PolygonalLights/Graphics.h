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
#include "DDSTextureLoader2.h"
#include <fstream>
#include <limits>
#include <cmath>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib")

#define CHECKED(expr, message) if(FAILED(expr)) throw graphicsException(message);


#define LIGHT_BUFFER_SIZE 10
#define MAX_OBJECTS 100

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

	unsigned int index;
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
	dx::XMFLOAT4 Cone;
};

struct RectLight {
	dx::XMFLOAT4 Position;
	dx::XMFLOAT4 Params; // Width, Height, RotY, RotZ
	dx::XMFLOAT4 Color;
};

class Graphics {
private:
	class PSConstantBuffer;
	class VSConstantBuffer;

public:
	Graphics(HWND hWnd, FLOAT width, FLOAT height);
	void Clear(const FLOAT colorRGBA[4]);
	void SwapBuffers();

	template<class V>
	void DrawTriangles(vector<V>& vBuffer, vector<unsigned short>& iBuffer, dx::XMFLOAT3 cameraPos, dx::XMFLOAT3 cameraDir);

	template<class V>
	void FillTriangle(vector<V>& vBuffer, vector<unsigned short>& iBuffer);

	template<class V>
	void FillCubeShared(vector<V>& vBuffer, vector<unsigned short>& iBuffer);

	template<class V>
	void FillCube(vector<V>& vBuffer, vector<unsigned short>& iBuffer, dx::XMMATRIX transform);

	template<class V>
	void FillFloor(vector<V>& vBuffer, vector<unsigned short>& iBuffer, dx::XMMATRIX transform);

	template<class V>
	void FillQuadLight(vector<V>& vBuffer, vector<unsigned short>& iBuffer, RectLight light);

	void AddPointLight(dx::XMFLOAT3 position, dx::XMFLOAT3 color, float intensity) {
		if (psConstantBuffer.lightCounts.x >= LIGHT_BUFFER_SIZE)
			return;

		int index = psConstantBuffer.lightCounts.x++;
		PointLight& light = psConstantBuffer.pointLights[index];
		light.Position = {
			position.x,
			position.y,
			position.z,
			1
		};
		light.Color = {
			color.x,
			color.y,
			color.z,
			intensity
		};
	}

	void AddSpotLight(dx::XMFLOAT3 position, dx::XMFLOAT3 color, dx::XMFLOAT3 direction, float intensity, float innerCone, float outerCone) {
		if (psConstantBuffer.lightCounts.y >= LIGHT_BUFFER_SIZE)
			return;

		int index = psConstantBuffer.lightCounts.y++;
		SpotLight& light = psConstantBuffer.spotLights[index];

		light.Position = {
			position.x,
			position.y,
			position.z,
			1
		};

		light.Color = {
			color.x,
			color.y,
			color.z,
			intensity
		};

		light.Cone = {
			innerCone,
			outerCone,
			0, 0
		};

		light.Direction = {
			direction.x,
			direction.y,
			direction.z,
			1
		};
	}

	void AddDirLight(dx::XMFLOAT3 color, dx::XMFLOAT3 direction, float intensity) {
		if (psConstantBuffer.lightCounts.z >= LIGHT_BUFFER_SIZE)
			return;

		int index = psConstantBuffer.lightCounts.z++;
		DirLight& light = psConstantBuffer.dirLights[index];

		light.Color = {
			color.x,
			color.y,
			color.z,
			intensity
		};

		light.Direction = {
			direction.x,
			direction.y,
			direction.z,
			1
		};
	}

	void AddRectLight(dx::XMFLOAT3 color, dx::XMFLOAT3 position, float width, float height, float rotationX, float rotationY, float intensity) {
		if (psConstantBuffer.lightCounts.w >= LIGHT_BUFFER_SIZE)
			return;

		int index = psConstantBuffer.lightCounts.w++;
		RectLight& light = psConstantBuffer.rectLights[index];

		light.Color = {
			color.x,
			color.y,
			color.z,
			intensity
		};

		light.Position = {
			position.x,
			position.y,
			position.z,
			1
		};

		light.Params = {
			width,
			height,
			rotationX,
			rotationY
		};
	}

	RectLight* GetRectLight(int index) {
		if (index >= psConstantBuffer.lightCounts.w)
			return nullptr;
		
		return &psConstantBuffer.rectLights[index];
	}

private:
	struct VSConstantBuffer {
		dx::XMMATRIX modelToWorld[MAX_OBJECTS];
		dx::XMMATRIX worldToView;
		dx::XMMATRIX projection;
		dx::XMMATRIX normalTransform[MAX_OBJECTS];
		unsigned int objects;
	};

	struct PSConstantBuffer {
		dx::XMFLOAT4 viewPos = { 0, 0, 0, 1 };
		dx::XMINT4 lightCounts = { 0, 0, 0, 0 }; // point, spot, dir, rect
		PointLight pointLights[LIGHT_BUFFER_SIZE] = {};
		SpotLight spotLights[LIGHT_BUFFER_SIZE] = {};
		DirLight dirLights[LIGHT_BUFFER_SIZE] = {};
		RectLight rectLights[LIGHT_BUFFER_SIZE] = {};
	};


	ComPtr<ID3D11Device> _pDevice;
	ComPtr<ID3D11DeviceContext> _pContext;
	ComPtr<IDXGISwapChain> _pSwapChain;
	ComPtr<ID3D11RenderTargetView> _pRTView;
	ComPtr<ID3D11Buffer> _pVSConstantBuffer;
	ComPtr<ID3D11Buffer> _pPSConstantBuffer;
	ComPtr<ID3D11Resource> _pLTCMatTexture;
	ComPtr<ID3D11ShaderResourceView> _pLTCMatTextureView;
	ComPtr<ID3D11Resource> _pLTCAmpTexture;
	ComPtr<ID3D11ShaderResourceView> _pLTCAmpTextureView;
	ComPtr<ID3D11SamplerState> _pSampler;
	ComPtr<ID3D11DepthStencilView> _pDepthStencilView;
	
	PSConstantBuffer psConstantBuffer;
	VSConstantBuffer vsConstantBuffer;
	FLOAT _width;
	FLOAT _height;
	size_t _objectIndex;


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
	_pContext->ClearDepthStencilView(_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0u);
}

void Graphics::SwapBuffers() {
	_pSwapChain->Present(1u, 0u);
	_objectIndex = 0;
}

template<class V>
void Graphics::DrawTriangles(vector<V>& vBuffer, vector<unsigned short>& iBuffer, dx::XMFLOAT3 cameraPos, dx::XMFLOAT3 cameraDir) {

	for (int i = 0; i < psConstantBuffer.lightCounts.w; i++) {
		RectLight* l = GetRectLight(i);
		FillQuadLight(vBuffer, iBuffer, *GetRectLight(i));
	}


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
		//vsConstantBuffer.modelToWorld = dx::XMMatrixTranspose(dx::XMMatrixTranslation(0, 0, 4));
		//vsConstantBuffer.normalTransform = dx::XMMatrixTranspose(dx::XMMatrixInverse(nullptr, vsConstantBuffer.modelToWorld[]));
		vsConstantBuffer.projection = dx::XMMatrixTranspose(dx::XMMatrixPerspectiveLH(1.0f, _height / _width, 0.5f, 500.0f));
		vsConstantBuffer.worldToView = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(
			dx::XMLoadFloat3(&cameraPos),
			dx::XMVector3Normalize(dx::XMLoadFloat3(&cameraDir)),
			{ 0, 1, 0 }
		));
		vsConstantBuffer.objects = _objectIndex;


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
void Graphics::FillCube(vector<Vertex>& vBuffer, vector<unsigned short>& iBuffer, dx::XMMATRIX transform) {
	unsigned short offset = vBuffer.size();

	float red[3] = { 1.0f, 0.0f, 0.0f };

	// BACK - green
	vBuffer.push_back({ -1.0f,-1.0f, 1.0f,	0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 1.0f, _objectIndex });
	vBuffer.push_back({ 1.0f,-1.0f, 1.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 1.0f, _objectIndex });
	vBuffer.push_back({ -1.0f,1.0f, 1.0f,	0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 1.0f , _objectIndex});
	vBuffer.push_back({ 1.0f,1.0f, 1.0f,	0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 1.0f , _objectIndex});

	// LEFT - magenta
	vBuffer.push_back({ -1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 1.0f,   -1.0f, 0.0f, 0.0f , _objectIndex });
	vBuffer.push_back({ -1.0f,1.0f, -1.0f,  1.0f, 0.0f, 1.0f,   -1.0f, 0.0f, 0.0f , _objectIndex});
	vBuffer.push_back({ -1.0f,-1.0f, 1.0f,	1.0f, 0.0f, 1.0f,   -1.0f, 0.0f, 0.0f , _objectIndex});
	vBuffer.push_back({ -1.0f,1.0f, 1.0f,	1.0f, 0.0f, 1.0f,   -1.0f, 0.0f, 0.0f , _objectIndex});

	// RIGHT - cyan
	vBuffer.push_back({ 1.0f,-1.0f, -1.0f,	0.0f, 1.0f, 1.0f,   1.0f, 0.0f, 0.0f , _objectIndex });
	vBuffer.push_back({ 1.0f,1.0f, -1.0f,	0.0f, 1.0f, 1.0f,   1.0f, 0.0f, 0.0f , _objectIndex });
	vBuffer.push_back({ 1.0f,-1.0f, 1.0f,  0.0f, 1.0f, 1.0f,   1.0f, 0.0f, 0.0f , _objectIndex });
	vBuffer.push_back({ 1.0f,1.0f, 1.0f,	0.0f, 1.0f, 1.0f,   1.0f, 0.0f, 0.0f , _objectIndex });

	// TOP - blue
	vBuffer.push_back({ -1.0f,1.0f, -1.0f,  0.0f, 0.0f, 1.0f,   0.0f, 1.0f, 0.0f , _objectIndex});
	vBuffer.push_back({ 1.0f,1.0f, -1.0f,	0.0f, 0.0f, 1.0f,   0.0f, 1.0f, 0.0f , _objectIndex});
	vBuffer.push_back({ -1.0f,1.0f, 1.0f,	0.0f, 0.0f, 1.0f,   0.0f, 1.0f, 0.0f , _objectIndex});
	vBuffer.push_back({ 1.0f,1.0f, 1.0f,	0.0f, 0.0f, 1.0f,   0.0f, 1.0f, 0.0f , _objectIndex});

	// BOTTOM - yellow
	vBuffer.push_back({ -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 0.0f,   0.0f, -1.0f, 0.0f , _objectIndex });
	vBuffer.push_back({ 1.0f,-1.0f, -1.0f,	1.0f, 1.0f, 0.0f,   0.0f, -1.0f, 0.0f , _objectIndex });
	vBuffer.push_back({ -1.0f,-1.0f, 1.0f,	1.0f, 1.0f, 0.0f,   0.0f, -1.0f, 0.0f , _objectIndex });
	vBuffer.push_back({ 1.0f,-1.0f, 1.0f,  1.0f, 1.0f, 0.0f,   0.0f, -1.0f, 0.0f , _objectIndex });

	// FRONT - red
	vBuffer.push_back({ -1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f,  0.0f, 0.0f, -1.0f , _objectIndex });
	vBuffer.push_back({ 1.0f,-1.0f, -1.0f,	1.0f, 0.0f, 0.0f,  0.0f, 0.0f, -1.0f , _objectIndex });
	vBuffer.push_back({ -1.0f,1.0f, -1.0f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f, -1.0f , _objectIndex });
	vBuffer.push_back({ 1.0f,1.0f, -1.0f,	1.0f, 0.0f, 0.0f,  0.0f, 0.0f, -1.0f , _objectIndex });


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

	vsConstantBuffer.modelToWorld[_objectIndex] = dx::XMMatrixTranspose(transform);
	vsConstantBuffer.normalTransform[_objectIndex] = dx::XMMatrixTranspose(dx::XMMatrixInverse(nullptr, vsConstantBuffer.modelToWorld[_objectIndex]));
	_objectIndex++;
}

template<>
void Graphics::FillFloor(vector<Vertex>& vBuffer, vector<unsigned short>& iBuffer, dx::XMMATRIX transform) {
	unsigned short offset = vBuffer.size();

	vBuffer.push_back({ -100.0f, -1.0f, -100.0f,	0.5f, 0.5f, 0.5f,  0.0f, 1.0f, 0.0f, _objectIndex });
	vBuffer.push_back({ 100.0f,  -1.0f, -100.0f,	0.5f, 0.5f, 0.5f,  0.0f, 1.0f, 0.0f, _objectIndex });
	vBuffer.push_back({ 100.0f,  -1.0f, 100.0f,	0.5f, 0.5f, 0.5f,  0.0f, 1.0f, 0.0f, _objectIndex });
	vBuffer.push_back({ -100.0f, -1.0f, 100.0f,	0.5f, 0.5f, 0.5f,  0.0f, 1.0f, 0.0f, _objectIndex });


	const unsigned short indices[] = {
		0,2,1,    0,3,2,
	};

	for (unsigned short index : indices)
		iBuffer.push_back(offset + index);

	vsConstantBuffer.modelToWorld[_objectIndex] = dx::XMMatrixTranspose(transform);
	vsConstantBuffer.normalTransform[_objectIndex] = dx::XMMatrixTranspose(dx::XMMatrixInverse(nullptr, vsConstantBuffer.modelToWorld[_objectIndex]));
	_objectIndex++;
}

template<>
void Graphics::FillQuadLight(vector<Vertex>& vBuffer, vector<unsigned short>& iBuffer, RectLight light) {
	unsigned short offset = vBuffer.size();

	vBuffer.push_back({ -1.0f, -1.0f, .0f,	light.Color.x, light.Color.y, light.Color.z,  .0f, .0f, -1.0f, _objectIndex});
	vBuffer.push_back({  1.0f, -1.0f, .0f,	light.Color.x, light.Color.y, light.Color.z,  .0f, .0f, -1.0f, _objectIndex });
	vBuffer.push_back({  1.0f,  1.0f, .0f,	light.Color.x, light.Color.y, light.Color.z,  .0f, .0f, -1.0f, _objectIndex });
	vBuffer.push_back({ -1.0f,  1.0f, .0f,	light.Color.x, light.Color.y, light.Color.z,  .0f, .0f, -1.0f, _objectIndex });


	const unsigned short indices[] = {
		0,2,1,    0,3,2,
		0,1,2,    0,2,3
	};

	for (unsigned short index : indices)
		iBuffer.push_back(offset + index);
	
	dx::XMMATRIX transform =
		dx::XMMatrixScaling(light.Params.x, light.Params.y, 0)
		* dx::XMMatrixRotationRollPitchYaw(0, light.Params.z * 2 * 3.14, light.Params.w * 2 * 3.14)
		* dx::XMMatrixTranslation(light.Position.x, light.Position.y, light.Position.z);

	vsConstantBuffer.modelToWorld[_objectIndex] = dx::XMMatrixTranspose(transform);
	vsConstantBuffer.normalTransform[_objectIndex] = dx::XMMatrixTranspose(dx::XMMatrixInverse(nullptr, vsConstantBuffer.modelToWorld[_objectIndex]));
	_objectIndex++;
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

	// create and bind depth stencil state
	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
	ComPtr<ID3D11DepthStencilState> pDSState;
	_pDevice->CreateDepthStencilState(&dsDesc, &pDSState);
	_pContext->OMSetDepthStencilState(pDSState.Get(), 1u);

	// create depth stencil texture
	ComPtr<ID3D11Texture2D> pDepthSencil;
	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = 800u;
	descDepth.Height = 600u;
	descDepth.MipLevels = 1u;
	descDepth.ArraySize = 1u;
	descDepth.Format = DXGI_FORMAT_D32_FLOAT;
	descDepth.SampleDesc.Count = 1u;
	descDepth.SampleDesc.Quality = 0u;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	_pDevice->CreateTexture2D(&descDepth, nullptr, &pDepthSencil);

	// create depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
	descDSV.Format = DXGI_FORMAT_D32_FLOAT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0u;
	_pDevice->CreateDepthStencilView(pDepthSencil.Get(), &descDSV, &_pDepthStencilView);
	_pContext->OMSetRenderTargets(1u, _pRTView.GetAddressOf(), _pDepthStencilView.Get());
}

void Graphics::BindShaders(ComPtr<ID3DBlob>& blobBuffer) {
	// create pixel shader
	{
		ComPtr<ID3D11PixelShader> pPixelShader;
		CHECKED(D3DReadFileToBlob(L"PixelShader.cso", &blobBuffer), "Reading PShader fucked up");
		CHECKED(_pDevice->CreatePixelShader(blobBuffer->GetBufferPointer(), blobBuffer->GetBufferSize(), nullptr, &pPixelShader), "PShader creation fucked up");
		_pContext->PSSetShader(pPixelShader.Get(), nullptr, 0u);

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

		hr = DirectX::CreateDDSTextureFromFile(_pDevice.Get(), L"./ltc_mat.dds", true, &_pLTCMatTexture, &_pLTCMatTextureView);
		_pContext->PSSetShaderResources(0, 1, _pLTCMatTextureView.GetAddressOf());

		hr = DirectX::CreateDDSTextureFromFile(_pDevice.Get(), L"./ltc_amp.dds", true, &_pLTCAmpTexture, &_pLTCAmpTextureView);
		_pContext->PSSetShaderResources(1, 1, _pLTCAmpTextureView.GetAddressOf());

		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

		_pDevice->CreateSamplerState(&samplerDesc, &_pSampler);
		_pContext->PSSetSamplers(0, 1, _pSampler.GetAddressOf());
	}

	// create vertex shader
	{
		ComPtr<ID3D11VertexShader> pVertexShader;
		CHECKED(D3DReadFileToBlob(L"VertexShader.cso", &blobBuffer), "Reading VSHader fucked up");
		CHECKED(_pDevice->CreateVertexShader(blobBuffer->GetBufferPointer(), blobBuffer->GetBufferSize(), nullptr, &pVertexShader), "VShader creation fucked up");
		_pContext->VSSetShader(pVertexShader.Get(), nullptr, 0u);

		// bind transformation matrix to vertex shader
		//vsConstantBuffer.modelToWorld = dx::XMMatrixTranspose(
		//	dx::XMMatrixScaling(_width / (float)_height, 1.0f, 1.0f)
		//);

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
		{ "Normal",0,DXGI_FORMAT_R32G32B32_FLOAT,0,24u,D3D11_INPUT_PER_VERTEX_DATA,0 },
		{ "Index",0,DXGI_FORMAT_R32_UINT,0,36u,D3D11_INPUT_PER_VERTEX_DATA,0 }
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
