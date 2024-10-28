#include "Engine.h"

#include <ctime>
#include <d3dcompiler.h>

#include "Structs/CBCamera.h"

LRESULT CALLBACK WndProc(const HWND hwnd, const UINT message, const WPARAM wParam, const LPARAM lParam)
{
	PAINTSTRUCT ps{};
	HDC hdc{};

	switch (message)
	{
	case WM_ACTIVATE:
	case WM_ACTIVATEAPP:
	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		EndPaint(hwnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_SIZE:
	case WM_EXITSIZEMOVE:
		//TODO: WM_SIZE Message
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}

	return 0;
}

Engine::~Engine()
{
#define RELEASE_RESOURCE(x) if (x) (x)->Release();

	RELEASE_RESOURCE(_device)
	RELEASE_RESOURCE(_deviceContext)
	RELEASE_RESOURCE(_renderTarget)
	RELEASE_RESOURCE(_inputLayout)
	RELEASE_RESOURCE(_rasterizerState)

	RELEASE_RESOURCE(_depthStencilBuffer)
	RELEASE_RESOURCE(_depthStencilView)

	RELEASE_RESOURCE(_dxgiDevice)
	RELEASE_RESOURCE(_dxgiFactory)
	RELEASE_RESOURCE(_dxgiSwapChain)

	RELEASE_RESOURCE(_vertexShader)
	RELEASE_RESOURCE(_pixelShader)

	RELEASE_RESOURCE(_bilinearSampler)

	RELEASE_RESOURCE(_cbCamera)
#ifdef _INSTANCED_RENDERER
	RELEASE_RESOURCE(_srvBuffer)
	RELEASE_RESOURCE(_srvInstance)
#endif

#undef RELEASE_RESOURCE
}

void Engine::Update()
{
#pragma region DeltaTime
	auto timePoint = std::chrono::high_resolution_clock::now();
	double deltaTime = std::chrono::duration_cast
		<std::chrono::nanoseconds>(timePoint - _lastFrameTime).count()
		/ 1000000000.0;

	_lastFrameTime = timePoint;

	//Catch large dT and void frame
	if (deltaTime > 1)
		return;
#pragma endregion

	//Update Code
}

void Engine::Draw()
{
	float backgroundColour[4] = { 0.025f, 0.025f, 0.025f, 1.0f };
	_deviceContext->OMSetRenderTargets(1, &_renderTarget, _depthStencilView);
	_deviceContext->ClearRenderTargetView(_renderTarget, backgroundColour);
	_deviceContext->ClearDepthStencilView(_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0.0f);

	//Draw Code
	UINT stride {sizeof(float)};
	UINT offset {0};
	_deviceContext->IASetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);
	_deviceContext->IASetIndexBuffer(_indexBuffer, DXGI_FORMAT_R32_UINT, offset);
	_deviceContext->VSSetShader(_vertexShader, nullptr, 0);
	_deviceContext->PSSetShader(_pixelShader, nullptr, 0);

	D3D11_MAPPED_SUBRESOURCE cameraData, objectData;
	//TODO: Update _cbCameraData
	_deviceContext->Map(_cbCamera, 0, D3D11_MAP_WRITE_DISCARD, 0, &cameraData);
	memcpy(cameraData.pData, &_cbCameraData, sizeof(CBCamera));
	_deviceContext->Unmap(_cbCamera, 0);

	

#ifdef _INSTANCED_RENDERER
	_deviceContext->VSSetShaderResources(0, 1, &_srvInstance);

	_deviceContext->DrawIndexedInstanced(36, OBJECTS_TO_INSTANCE, 0, 0, 0);
#else
	for (DirectX::XMFLOAT4 position : _positions)
	{
		_cbObjectData.World = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
		_deviceContext->Map(_cbObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &objectData);
		memcpy(objectData.pData, &_cbObjectData, sizeof(CBObject));
		_deviceContext->Unmap(_cbObject, 0);

		_deviceContext->DrawIndexed(36, 0, 0);
	}
#endif

	_dxgiSwapChain->Present(0, 0);
}


HRESULT Engine::Initialise(HINSTANCE hInstance)
{
	HRESULT hr = S_OK;
#define FAIL_CHECK if (FAILED(hr)) return hr;
	
	hr = CreateWindowHandle(hInstance); FAIL_CHECK
	hr = CreateD3DDevice(); FAIL_CHECK
	hr = CreateSwapChain(); FAIL_CHECK

	hr = InitialiseRuntimeData(); FAIL_CHECK

	hr = CreateFrameBuffer(); FAIL_CHECK
	hr = InitialiseShaders(); FAIL_CHECK
	hr = InitialisePipeline(); FAIL_CHECK

#undef FAIL_CHECK
	return hr;
}

HRESULT Engine::CreateWindowHandle(HINSTANCE hInstance)
{
	const wchar_t* windowName = L"Dissertation Artefact";

	WNDCLASSW wndClass{};
	wndClass.style = 0;
	wndClass.lpfnWndProc = WndProc;
	wndClass.lpszClassName = windowName;

	RegisterClass(&wndClass);

	_hWnd = CreateWindowEx(0, windowName, windowName, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
	                       800, 600, nullptr, nullptr, hInstance, nullptr);

	return S_OK;
}

HRESULT Engine::CreateD3DDevice()
{
	HRESULT hr = S_OK;
#define FAIL_CHECK if (FAILED(hr)) return hr;

	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
	};

	ID3D11Device* device;
	ID3D11DeviceContext* deviceContext;

	DWORD deviceFlags = 0;
#ifdef _DEBUG
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	hr = D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		D3D11_CREATE_DEVICE_BGRA_SUPPORT | deviceFlags,
		featureLevels,
		ARRAYSIZE(featureLevels),
		D3D11_SDK_VERSION,
		&device,
		nullptr,
		&deviceContext
	); FAIL_CHECK

	hr = device->QueryInterface(__uuidof(ID3D11Device), reinterpret_cast<void**>(&_device)); FAIL_CHECK
	hr = deviceContext->QueryInterface(__uuidof(ID3D11DeviceContext), reinterpret_cast<void**>(&_deviceContext)); FAIL_CHECK
	device->Release();
	deviceContext->Release();

	IDXGIAdapter* dxgiAdaptor;
	hr = _device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&_dxgiDevice)); FAIL_CHECK
	hr = _dxgiDevice->GetAdapter(&dxgiAdaptor); FAIL_CHECK
	hr = dxgiAdaptor->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&_dxgiFactory)); FAIL_CHECK
	dxgiAdaptor->Release();

#undef FAIL_CHECK
	return hr;
}

HRESULT Engine::CreateSwapChain()
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
	swapChainDesc.Width = 0; // Defer to WindowWidth
    swapChainDesc.Height = 0; // Defer to WindowHeight
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //FLIP* modes don't support sRGB back buffer
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = 0;

	return _dxgiFactory->CreateSwapChainForHwnd(_device, _hWnd, &swapChainDesc, nullptr, nullptr, &_dxgiSwapChain);
}

HRESULT Engine::CreateFrameBuffer()
{
	HRESULT hr = S_OK;
#define FAIL_CHECK if (FAILED(hr)) return hr;

	ID3D11Texture2D* frameBuffer = nullptr;
	hr = _dxgiSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&frameBuffer)); FAIL_CHECK

	D3D11_RENDER_TARGET_VIEW_DESC frameBufferDesc{};
	frameBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	frameBufferDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	hr = _device->CreateRenderTargetView(frameBuffer, &frameBufferDesc, &_renderTarget); FAIL_CHECK

	D3D11_TEXTURE2D_DESC depthBufferDesc{};
	frameBuffer->GetDesc(&depthBufferDesc);
	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	hr = _device->CreateTexture2D(&depthBufferDesc, nullptr, &_depthStencilBuffer); FAIL_CHECK
	hr = _device->CreateDepthStencilView(_depthStencilBuffer, nullptr, &_depthStencilView); FAIL_CHECK

	frameBuffer->Release();

	hr = _dxgiFactory->MakeWindowAssociation(_hWnd, DXGI_MWA_NO_ALT_ENTER); FAIL_CHECK

#undef FAIL_CHECK
	return hr;
}

HRESULT Engine::InitialiseShaders()
{
	LPCWSTR vertexPath;
	LPCWSTR pixelPath;
	//TODO: Write Shaders
#ifdef _INSTANCED_RENDERER
	vertexPath = L"Engine/Shaders/VS_Instanced.hlsl";
	pixelPath = L"Engine/Shaders/PS_Instanced.hlsl";
#else
	vertexPath = L"Engine/Shaders/VS.hlsl";
	pixelPath = L"Engine/Shaders/PS.hlsl";
#endif

	_vertexShader = CompileVertexShader(_hWnd, _device, &_inputLayout, vertexPath);
	//_pixelShader = CompilePixelShader(_hWnd, _device, pixelPath);

	return S_OK;
}

HRESULT Engine::InitialisePipeline()
{
	HRESULT hr = S_OK;
#define FAIL_CHECK if (FAILED(hr)) return hr;

	_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_deviceContext->IASetInputLayout(_inputLayout);

	D3D11_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.DepthClipEnable = true;
	hr = _device->CreateRasterizerState(&rasterizerDesc, &_rasterizerState); FAIL_CHECK
	_deviceContext->RSSetState(_rasterizerState);

	_viewport = { 0.0f, 0.0f, 800, 600, 0.0f, 1.0f };
	_deviceContext->RSSetViewports(1, &_viewport);

	D3D11_BUFFER_DESC bufferDesc{};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	bufferDesc.ByteWidth = sizeof(CBCamera);
	hr = _device->CreateBuffer(&bufferDesc, nullptr, &_cbCamera); FAIL_CHECK
	_deviceContext->VSSetConstantBuffers(0, 1, &_cbCamera);
	_deviceContext->PSSetConstantBuffers(0, 1, &_cbCamera);

	bufferDesc.ByteWidth = sizeof(CBObject);
	hr = _device->CreateBuffer(&bufferDesc, nullptr, &_cbObject); FAIL_CHECK
	_deviceContext->VSSetConstantBuffers(1, 1, &_cbObject);
	_deviceContext->PSSetConstantBuffers(1, 1, &_cbObject);

#ifdef _INSTANCED_RENDERER
	bufferDesc.ByteWidth = sizeof(DirectX::XMFLOAT4X4) * OBJECTS_TO_INSTANCE;
	bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bufferDesc.StructureByteStride = sizeof(DirectX::XMFLOAT4X4);

	D3D11_SUBRESOURCE_DATA srvData{};
	DirectX::XMFLOAT4X4 transformedPositions[OBJECTS_TO_INSTANCE]{};
	for (unsigned i = 0; i < OBJECTS_TO_INSTANCE; i++)
	{
		DirectX::XMFLOAT4 pos = _positions[i];
		DirectX::XMMATRIX matrix = DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
		XMStoreFloat4x4(&transformedPositions[i], matrix);
	}
	srvData.pSysMem = transformedPositions;

	hr = _device->CreateBuffer(&bufferDesc, &srvData, &_srvBuffer); FAIL_CHECK

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};

	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = OBJECTS_TO_INSTANCE;
	srvDesc.Buffer.ElementOffset = 0;
	srvDesc.Buffer.ElementWidth = sizeof(DirectX::XMFLOAT4X4);
	//Errors?

	hr = _device->CreateShaderResourceView(_srvBuffer, nullptr, &_srvInstance); FAIL_CHECK
#endif

	D3D11_SAMPLER_DESC bilinearSamplerDesc{};
	bilinearSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    bilinearSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    bilinearSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    bilinearSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    bilinearSamplerDesc.MaxLOD = 1;
    bilinearSamplerDesc.MinLOD = 0;

	hr = _device->CreateSamplerState(&bilinearSamplerDesc, &_bilinearSampler); FAIL_CHECK
    _deviceContext->PSSetSamplers(0, 1, &_bilinearSampler);

	return hr;

#undef FAIL_CHECK
}

HRESULT Engine::InitialiseRuntimeData()
{
#define RAND(MIN, MAX) (float)((MIN) + rand() / (RAND_MAX / ((MAX) - (MIN) + 1) + 1))

	HRESULT hr = S_OK;
#define FAIL_CHECK if (FAILED(hr)) return hr;

	srand(time(nullptr));
	for (DirectX::XMFLOAT4& position : _positions)
	{
		position = {RAND(0, 128), RAND(0, 128), RAND(0, 128), 0};
	}
#undef RAND

	float cubeVertices[]
	{
	    -1.0f, -1.0f,  1.0f,
	     1.0f, -1.0f,  1.0f,
	     1.0f,  1.0f,  1.0f,
	    -1.0f,  1.0f,  1.0f,
	    -1.0f, -1.0f, -1.0f,
	     1.0f, -1.0f, -1.0f,
	     1.0f,  1.0f, -1.0f,
	    -1.0f,  1.0f, -1.0f
	};

	D3D11_BUFFER_DESC vertexBufferDesc = {};
	vertexBufferDesc.ByteWidth = 24 * sizeof(float);
	vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA subresourceVertexData = { cubeVertices };
	hr = _device->CreateBuffer(&vertexBufferDesc, &subresourceVertexData, &_vertexBuffer); FAIL_CHECK

	int cubeIndex[]
	{
		0, 1, 2,
		2, 3, 0,
		1, 5, 6,
		6, 2, 1,
		7, 6, 5,
		5, 4, 7,
		4, 0, 3,
		3, 7, 4,
		4, 5, 1,
		1, 0, 4,
		3, 2, 6,
		6, 7, 3
	};

	D3D11_BUFFER_DESC indexBufferDesc = {};
	indexBufferDesc.ByteWidth = 36 * sizeof(int);
	indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA subresourceIndexData = { cubeIndex };

	hr = _device->CreateBuffer(&indexBufferDesc, &subresourceIndexData, &_indexBuffer); FAIL_CHECK

#undef FAIL_CHECK
	return hr;
}

HRESULT Engine::CreateVertexShaderLayout(ID3D11Device* device, ID3D11InputLayout** inputLayout, ID3DBlob* vsBlob)
{
	D3D11_INPUT_ELEMENT_DESC vsInputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA,   0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
#ifdef _INSTANCED_RENDERER
		{ "SV_InstanceID", 0, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
#endif
	};

	return device->CreateInputLayout(
		vsInputLayout,
		ARRAYSIZE(vsInputLayout),
		vsBlob->GetBufferPointer(),
		vsBlob->GetBufferSize(),
		inputLayout
	);
}

ID3D11VertexShader* Engine::CompileVertexShader(HWND hWnd, ID3D11Device* device, ID3D11InputLayout** inputLayout, LPCWSTR path)
{
#define RELEASE_BLOB(blob) if(blob) (blob)->Release();

	ID3D11VertexShader* vs = nullptr;
	ID3DBlob* errorBlob;
	ID3DBlob* vsBlob;

	DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	shaderFlags |= D3DCOMPILE_DEBUG;
#endif

	HRESULT hr = D3DCompileFromFile(
		path, 
		nullptr, 
		D3D_COMPILE_STANDARD_FILE_INCLUDE, 
		"VS_Main", 
		"vs_5_0", 
		shaderFlags, 
		0, 
		&vsBlob, 
		&errorBlob
	);

	if(FAILED(hr))
	{
		MessageBoxA(hWnd, static_cast<char*>(errorBlob->GetBufferPointer()), nullptr, ERROR);
		RELEASE_BLOB(vsBlob)
		RELEASE_BLOB(errorBlob)
		return nullptr;
	}


	hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vs);
	if(FAILED(hr))
	{
		RELEASE_BLOB(vsBlob)
		RELEASE_BLOB(errorBlob)
		return nullptr;
	}

	hr = CreateVertexShaderLayout(device, inputLayout, vsBlob);
	if(FAILED(hr))
	{
		RELEASE_BLOB(vsBlob)
		RELEASE_BLOB(errorBlob)
		return nullptr;
	}

	RELEASE_BLOB(vsBlob)
	RELEASE_BLOB(errorBlob)

#undef RELEASE_BLOB

	return vs;
}

ID3D11PixelShader* Engine::CompilePixelShader(HWND hWnd, ID3D11Device* device, LPCWSTR path)
{
	#define RELEASE_BLOB(blob) if(blob) (blob)->Release();

	ID3D11PixelShader* ps = nullptr;
	ID3DBlob* errorBlob;
	ID3DBlob* psBlob;

	DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	shaderFlags |= D3DCOMPILE_DEBUG;
#endif

	HRESULT hr = D3DCompileFromFile(
		path, 
		nullptr, 
		D3D_COMPILE_STANDARD_FILE_INCLUDE, 
		"PS_Main", 
		"ps_5_0", 
		shaderFlags, 
		0, 
		&psBlob, 
		&errorBlob
	);

	if(FAILED(hr))
	{
		MessageBoxA(hWnd, static_cast<char*>(errorBlob->GetBufferPointer()), nullptr, ERROR);
		RELEASE_BLOB(psBlob)
		RELEASE_BLOB(errorBlob)
		return nullptr;
	}


	hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &ps);
	if(FAILED(hr))
	{
		RELEASE_BLOB(psBlob)
		RELEASE_BLOB(errorBlob)
		return nullptr;
	}

	RELEASE_BLOB(psBlob)
	RELEASE_BLOB(errorBlob)

#undef RELEASE_BLOB

	return ps;
}