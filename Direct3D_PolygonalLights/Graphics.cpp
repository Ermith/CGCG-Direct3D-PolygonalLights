#include "Graphics.h"

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

void Graphics::DrawTriangles(vector<Vertex>& vBuffer, vector<unsigned short>& iBuffer, dx::XMFLOAT3 cameraPos, dx::XMFLOAT3 cameraRotation) {

	for (int i = 0; i < _psConstantBuffer.lightCounts.w; i++) {
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
		_vsConstantBuffer.projection = dx::XMMatrixTranspose(dx::XMMatrixPerspectiveLH(1.0f, _height / _width, 0.5f, 500.0f));
		dx::XMFLOAT3 forward(0, 0, 1);
		dx::XMFLOAT3 pos(0, 0, 0);
		dx::XMMATRIX translation;

		dx::XMVECTOR cameraDir = dx::XMVector3Transform(
			dx::XMLoadFloat3(&forward),
			dx::XMMatrixRotationRollPitchYaw(cameraRotation.x, cameraRotation.y, cameraRotation.z)
		);
		_vsConstantBuffer.worldToView = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(
			dx::XMLoadFloat3(&cameraPos),
			dx::XMVector3Normalize(cameraDir),
			{ 0, 1, 0 }
		));
		_vsConstantBuffer.objects = _objectIndex;


		// Update the constant buffer.
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		_pContext->Map(
			_pVSConstantBuffer.Get(),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		memcpy(mappedResource.pData, &_vsConstantBuffer,
			sizeof(_vsConstantBuffer));
		_pContext->Unmap(_pVSConstantBuffer.Get(), 0);
	}

	// Update PS Constant Buffer
	{
		_psConstantBuffer.viewPos = dx::XMFLOAT4{ cameraPos.x, cameraPos.y, cameraPos.z, 1 };

		// Update the constant buffer.
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		_pContext->Map(
			_pPSConstantBuffer.Get(),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);

		memcpy(mappedResource.pData, &_psConstantBuffer,
			sizeof(_psConstantBuffer));
		_pContext->Unmap(_pPSConstantBuffer.Get(), 0);
	}

	_pContext->DrawIndexed(iBuffer.size(), 0u, 0u);
}

void Graphics::FillTriangle(vector<Vertex>& vBuffer, vector<unsigned short>& iBuffer) {

	unsigned short offset = vBuffer.size();

	vBuffer.push_back({ 0.0f,0.5f, 0.0f , 1.0f, 0.0f, 0.0f });
	vBuffer.push_back({ 0.5f,-0.5f, 0.0f ,0.0f, 1.0f, 0.0f });
	vBuffer.push_back({ -0.5f,-0.5f, 0.0f ,0.0f, 0.0f, 1.0f });

	iBuffer.push_back(offset + 0u);
	iBuffer.push_back(offset + 1u);
	iBuffer.push_back(offset + 2u);
}

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

void Graphics::FillCube(vector<Vertex>& vBuffer, vector<unsigned short>& iBuffer, dx::XMMATRIX transform) {
	unsigned short offset = vBuffer.size();

	float red[3] = { 1.0f, 0.0f, 0.0f };

	// BACK - green
	vBuffer.push_back({ -1.0f,-1.0f, 1.0f,	0.0f, 1.0f, 0.0f, 0,0, 0.0f, 0.0f, 1.0f, _objectIndex });
	vBuffer.push_back({ 1.0f,-1.0f, 1.0f,  0.0f, 1.0f, 0.0f,  0,0, 0.0f, 0.0f, 1.0f, _objectIndex });
	vBuffer.push_back({ -1.0f,1.0f, 1.0f,	0.0f, 1.0f, 0.0f, 0,0, 0.0f, 0.0f, 1.0f , _objectIndex });
	vBuffer.push_back({ 1.0f,1.0f, 1.0f,	0.0f, 1.0f, 0.0f, 0,0, 0.0f, 0.0f, 1.0f , _objectIndex });

	// LEFT - magenta
	vBuffer.push_back({ -1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 0,0,   -1.0f, 0.0f, 0.0f , _objectIndex });
	vBuffer.push_back({ -1.0f,1.0f, -1.0f,  1.0f, 0.0f, 1.0f,  0,0,  -1.0f, 0.0f, 0.0f , _objectIndex });
	vBuffer.push_back({ -1.0f,-1.0f, 1.0f,	1.0f, 0.0f, 1.0f,  0,0,  -1.0f, 0.0f, 0.0f , _objectIndex });
	vBuffer.push_back({ -1.0f,1.0f, 1.0f,	1.0f, 0.0f, 1.0f,  0,0,  -1.0f, 0.0f, 0.0f , _objectIndex });

	// RIGHT - cyan
	vBuffer.push_back({ 1.0f,-1.0f, -1.0f,	0.0f, 1.0f, 1.0f, 0,0,  1.0f, 0.0f, 0.0f , _objectIndex });
	vBuffer.push_back({ 1.0f,1.0f, -1.0f,	0.0f, 1.0f, 1.0f, 0,0,  1.0f, 0.0f, 0.0f , _objectIndex });
	vBuffer.push_back({ 1.0f,-1.0f, 1.0f,  0.0f, 1.0f, 1.0f,  0,0, 1.0f, 0.0f, 0.0f , _objectIndex });
	vBuffer.push_back({ 1.0f,1.0f, 1.0f,	0.0f, 1.0f, 1.0f, 0,0,  1.0f, 0.0f, 0.0f , _objectIndex });

	// TOP - blue
	vBuffer.push_back({ -1.0f,1.0f, -1.0f,  0.0f, 0.0f, 1.0f, 0,0,  0.0f, 1.0f, 0.0f , _objectIndex });
	vBuffer.push_back({ 1.0f,1.0f, -1.0f,	0.0f, 0.0f, 1.0f, 0,0,  0.0f, 1.0f, 0.0f , _objectIndex });
	vBuffer.push_back({ -1.0f,1.0f, 1.0f,	0.0f, 0.0f, 1.0f, 0,0,  0.0f, 1.0f, 0.0f , _objectIndex });
	vBuffer.push_back({ 1.0f,1.0f, 1.0f,	0.0f, 0.0f, 1.0f, 0,0,  0.0f, 1.0f, 0.0f , _objectIndex });

	// BOTTOM - yellow
	vBuffer.push_back({ -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 0.0f,0,0,   0.0f, -1.0f, 0.0f , _objectIndex });
	vBuffer.push_back({ 1.0f,-1.0f, -1.0f,	1.0f, 1.0f, 0.0f, 0,0,  0.0f, -1.0f, 0.0f , _objectIndex });
	vBuffer.push_back({ -1.0f,-1.0f, 1.0f,	1.0f, 1.0f, 0.0f, 0,0,  0.0f, -1.0f, 0.0f , _objectIndex });
	vBuffer.push_back({ 1.0f,-1.0f, 1.0f,  1.0f, 1.0f, 0.0f,  0,0, 0.0f, -1.0f, 0.0f , _objectIndex });

	// FRONT - red
	vBuffer.push_back({ -1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f,0,0,  0.0f, 0.0f, -1.0f , _objectIndex });
	vBuffer.push_back({ 1.0f,-1.0f, -1.0f,	1.0f, 0.0f, 0.0f, 0,0, 0.0f, 0.0f, -1.0f , _objectIndex });
	vBuffer.push_back({ -1.0f,1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 0,0, 0.0f, 0.0f, -1.0f , _objectIndex });
	vBuffer.push_back({ 1.0f,1.0f, -1.0f,	1.0f, 0.0f, 0.0f, 0,0, 0.0f, 0.0f, -1.0f , _objectIndex });


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

	_vsConstantBuffer.modelToWorld[_objectIndex] = dx::XMMatrixTranspose(transform);
	_vsConstantBuffer.normalTransform[_objectIndex] = dx::XMMatrixTranspose(dx::XMMatrixInverse(nullptr, _vsConstantBuffer.modelToWorld[_objectIndex]));
	_objectIndex++;
}

void Graphics::FillFloor(vector<Vertex>& vBuffer, vector<unsigned short>& iBuffer, dx::XMMATRIX transform) {
	unsigned short offset = vBuffer.size();

	vBuffer.push_back({ -100.0f, -1.0f, -100.0f,	0.5f, 0.5f, 0.5f, 0,0, 0.0f, 1.0f, 0.0f, _objectIndex });
	vBuffer.push_back({ 100.0f,  -1.0f, -100.0f,	0.5f, 0.5f, 0.5f, 0,0, 0.0f, 1.0f, 0.0f, _objectIndex });
	vBuffer.push_back({ 100.0f,  -1.0f, 100.0f,		0.5f, 0.5f, 0.5f, 0,0, 0.0f, 1.0f, 0.0f, _objectIndex });
	vBuffer.push_back({ -100.0f, -1.0f, 100.0f,		0.5f, 0.5f, 0.5f, 0,0, 0.0f, 1.0f, 0.0f, _objectIndex });


	const unsigned short indices[] = {
		0,2,1,    0,3,2,
	};

	for (unsigned short index : indices)
		iBuffer.push_back(offset + index);

	_vsConstantBuffer.modelToWorld[_objectIndex] = dx::XMMatrixTranspose(transform);
	_vsConstantBuffer.normalTransform[_objectIndex] = dx::XMMatrixTranspose(dx::XMMatrixInverse(nullptr, _vsConstantBuffer.modelToWorld[_objectIndex]));
	_objectIndex++;
}

void Graphics::FillQuadLight(vector<Vertex>& vBuffer, vector<unsigned short>& iBuffer, RectLight light) {
	unsigned short offset = vBuffer.size();

	vBuffer.push_back({ -1.0f, -1.0f, .0f,	light.Color.x, light.Color.y, light.Color.z, 0,0, .0f, .0f, -1.0f, _objectIndex });
	vBuffer.push_back({ 1.0f, -1.0f, .0f,	light.Color.x, light.Color.y, light.Color.z, 1,0, .0f, .0f, -1.0f, _objectIndex });
	vBuffer.push_back({ 1.0f,  1.0f, .0f,	light.Color.x, light.Color.y, light.Color.z, 1,1, .0f, .0f, -1.0f, _objectIndex });
	vBuffer.push_back({ -1.0f,  1.0f, .0f,	light.Color.x, light.Color.y, light.Color.z, 0,1, .0f, .0f, -1.0f, _objectIndex });

	const unsigned short indices[] = {
		0,2,1,    0,3,2,
		0,1,2,    0,2,3
	};

	for (unsigned short index : indices)
		iBuffer.push_back(offset + index);

	dx::XMMATRIX transform =
		dx::XMMatrixScaling(light.Params.x, light.Params.y, 0)
		* dx::XMMatrixRotationY(light.Params.z * 2 * 3.14)
		* dx::XMMatrixRotationZ(light.Params.w * 2 * 3.14)
		* dx::XMMatrixTranslation(light.Position.x, light.Position.y, light.Position.z);

	_vsConstantBuffer.modelToWorld[_objectIndex] = dx::XMMatrixTranspose(transform);
	_vsConstantBuffer.normalTransform[_objectIndex] = dx::XMMatrixTranspose(dx::XMMatrixInverse(nullptr, _vsConstantBuffer.modelToWorld[_objectIndex]));
	_objectIndex++;
}

void Graphics::AddPointLight(dx::XMFLOAT3 position, dx::XMFLOAT3 color, float intensity) {
	if (_psConstantBuffer.lightCounts.x >= LIGHT_BUFFER_SIZE)
		return;

	int index = _psConstantBuffer.lightCounts.x++;
	PointLight& light = _psConstantBuffer.pointLights[index];
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

void Graphics::AddSpotLight(dx::XMFLOAT3 position, dx::XMFLOAT3 color, dx::XMFLOAT3 direction, float intensity, float innerCone, float outerCone) {
	if (_psConstantBuffer.lightCounts.y >= LIGHT_BUFFER_SIZE)
		return;

	int index = _psConstantBuffer.lightCounts.y++;
	SpotLight& light = _psConstantBuffer.spotLights[index];

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

void Graphics::AddDirLight(dx::XMFLOAT3 color, dx::XMFLOAT3 direction, float intensity) {
	if (_psConstantBuffer.lightCounts.z >= LIGHT_BUFFER_SIZE)
		return;

	int index = _psConstantBuffer.lightCounts.z++;
	DirLight& light = _psConstantBuffer.dirLights[index];

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

void Graphics::AddRectLight(dx::XMFLOAT3 position, dx::XMFLOAT3 color, float intensity, float width, float height, float rotationX, float rotationY) {
	if (_psConstantBuffer.lightCounts.w >= LIGHT_BUFFER_SIZE)
		return;

	int index = _psConstantBuffer.lightCounts.w++;
	RectLight& light = _psConstantBuffer.rectLights[index];

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

PointLight* Graphics::GetPointLight(int index) {
	if (index >= _psConstantBuffer.lightCounts.x)
		return nullptr;

	return &_psConstantBuffer.pointLights[index];
}

SpotLight* Graphics::GetSpotLight(int index) {
	if (index >= _psConstantBuffer.lightCounts.y)
		return nullptr;

	return &_psConstantBuffer.spotLights[index];
}

DirLight* Graphics::GetDirLight(int index) {
	if (index >= _psConstantBuffer.lightCounts.z)
		return nullptr;

	return &_psConstantBuffer.dirLights[index];
}

RectLight* Graphics::GetRectLight(int index) {
	if (index >= _psConstantBuffer.lightCounts.w)
		return nullptr;

	return &_psConstantBuffer.rectLights[index];
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
	// pixel shader
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
		bd.ByteWidth = sizeof(_psConstantBuffer);
		bd.StructureByteStride = 0u;

		D3D11_SUBRESOURCE_DATA sd = {};
		sd.pSysMem = &_psConstantBuffer;

		HRESULT hr = _pDevice->CreateBuffer(&bd, &sd, &_pPSConstantBuffer);
		_pContext->PSSetConstantBuffers(0u, 1u, _pPSConstantBuffer.GetAddressOf());

		hr = DirectX::CreateDDSTextureFromFile(_pDevice.Get(), L"./ltc_mat.dds", true, &_pLTCMatTexture, &_pLTCMatTextureView);
		_pContext->PSSetShaderResources(0, 1, _pLTCMatTextureView.GetAddressOf());

		hr = DirectX::CreateDDSTextureFromFile(_pDevice.Get(), L"./ltc_amp.dds", true, &_pLTCAmpTexture, &_pLTCAmpTextureView);
		_pContext->PSSetShaderResources(1, 1, _pLTCAmpTextureView.GetAddressOf());

		hr = DirectX::CreateDDSTextureFromFile(_pDevice.Get(), L"./dataFiltered.dds", true, &_pLTCAmpTexture, &_pLTCAmpTextureView);
		_pContext->PSSetShaderResources(2, 1, _pLTCAmpTextureView.GetAddressOf());


		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

		_pDevice->CreateSamplerState(&samplerDesc, &_pSampler);
		_pContext->PSSetSamplers(0, 1, _pSampler.GetAddressOf());
	}

	// vertex shader
	{
		ComPtr<ID3D11VertexShader> pVertexShader;
		CHECKED(D3DReadFileToBlob(L"VertexShader.cso", &blobBuffer), "Reading VSHader fucked up");
		CHECKED(_pDevice->CreateVertexShader(blobBuffer->GetBufferPointer(), blobBuffer->GetBufferSize(), nullptr, &pVertexShader), "VShader creation fucked up");
		_pContext->VSSetShader(pVertexShader.Get(), nullptr, 0u);

		D3D11_BUFFER_DESC bd = {};
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0u;
		bd.ByteWidth = sizeof(VSConstantBuffer);
		bd.StructureByteStride = 0u;

		D3D11_SUBRESOURCE_DATA sd = {};
		sd.pSysMem = &_vsConstantBuffer;

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
		{ "Texture",0,DXGI_FORMAT_R32G32_FLOAT,0,24u,D3D11_INPUT_PER_VERTEX_DATA,0 },
		{ "Normal",0,DXGI_FORMAT_R32G32B32_FLOAT,0,32u,D3D11_INPUT_PER_VERTEX_DATA,0 },
		{ "Index",0,DXGI_FORMAT_R32_UINT,0,44u,D3D11_INPUT_PER_VERTEX_DATA,0 }
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