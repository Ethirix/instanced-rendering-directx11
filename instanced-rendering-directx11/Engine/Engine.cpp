#include "../main.cpp"
#include "Engine.h"

#include <d3dcompiler.h>

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
#undef RELEASE_RESOURCE
}


HRESULT Engine::Initialise(HINSTANCE hInstance)
{

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
	hr = device->QueryInterface(__uuidof(ID3D11DeviceContext), reinterpret_cast<void**>(&_deviceContext)); FAIL_CHECK
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

	_vertexShader = CompileVertexShader(_hWnd, _device, _inputLayout, vertexPath);
	_pixelShader = CompilePixelShader(_hWnd, _device, pixelPath);

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


#undef FAIL_CHECK
}

HRESULT Engine::CreateVertexShaderLayout(ID3D11Device* device, ID3D11InputLayout* inputLayout, ID3DBlob* vsBlob)
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
		&inputLayout
	);
}

ID3D11VertexShader* Engine::CompileVertexShader(HWND hWnd, ID3D11Device* device, ID3D11InputLayout* inputLayout, LPCWSTR path)
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