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
#include "DDSTextureLoader.h"
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

	float u;
	float v;

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
public:
	Graphics(HWND hWnd, FLOAT width, FLOAT height);
	void Clear(const FLOAT colorRGBA[4]);
	void SwapBuffers();
	void DrawTriangles(vector<Vertex>& vBuffer, vector<unsigned short>& iBuffer, dx::XMFLOAT3 cameraPos, dx::XMFLOAT3 cameraRotation);
	void FillTriangle(vector<Vertex>& vBuffer, vector<unsigned short>& iBuffer);
	void FillCubeShared(vector<Vertex>& vBuffer, vector<unsigned short>& iBuffer);
	void FillCube(vector<Vertex>& vBuffer, vector<unsigned short>& iBuffer, dx::XMMATRIX transform);
	void FillFloor(vector<Vertex>& vBuffer, vector<unsigned short>& iBuffer, dx::XMMATRIX transform);
	void FillQuadLight(vector<Vertex>& vBuffer, vector<unsigned short>& iBuffer, RectLight light);
	void AddPointLight(dx::XMFLOAT3 position, dx::XMFLOAT3 color, float intensity = 1.0f);
	void AddSpotLight(dx::XMFLOAT3 position, dx::XMFLOAT3 color, dx::XMFLOAT3 direction, float intensity = 10.0f, float innerCone = 0.7f, float outerCone = .75f);
	void AddDirLight(dx::XMFLOAT3 color, dx::XMFLOAT3 direction, float intensity = 1.0f);
	void AddRectLight(dx::XMFLOAT3 position, dx::XMFLOAT3 color, float intensity = 1.0f, float width = 1.0f, float height = 1.0f, float rotationX = .0f, float rotationY = .0f);

	PointLight* GetPointLight(int index);
	SpotLight* GetSpotLight(int index);
	DirLight* GetDirLight(int index);
	RectLight* GetRectLight(int index);

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
	
	PSConstantBuffer _psConstantBuffer;
	VSConstantBuffer _vsConstantBuffer;
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