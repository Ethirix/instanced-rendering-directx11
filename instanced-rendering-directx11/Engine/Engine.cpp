#include "Engine.h"

#include <ctime>
#include <d3dcompiler.h>
#include <iostream>

#include "GlobalDefs.h"

#include "../Optick/optick.h"

#include "Structs/PerVertexBuffer.h"
#include "Tools/Loader.h"

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

	RELEASE_RESOURCE(_perVertexBuffer)
	RELEASE_RESOURCE(_indexBuffer)
	RELEASE_RESOURCE(_vertexShader)
	RELEASE_RESOURCE(_pixelShader)

	RELEASE_RESOURCE(_bilinearSampler)

	RELEASE_RESOURCE(_cbCamera)

#if !defined(_INSTANCED_RENDERER)
	RELEASE_RESOURCE(_cbObject)
#elif defined(_INSTANCED_RENDERER)
		delete [] _lod0World;
		delete [] _lod1World;
		delete [] _lod2World;
#elif defined(_INSTANCED_RENDERER) && !defined(_INSTANCED_INPUT_LAYOUT)
	RELEASE_RESOURCE(_srvBuffer)
	RELEASE_RESOURCE(_srvInstance)
#elif defined(_INSTANCED_RENDERER) && defined(_INSTANCED_INPUT_LAYOUT)
	RELEASE_RESOURCE(_perInstanceBuffer);
#endif

	delete [] _objects;

	for (Mesh* mesh : _meshes)
	{
		delete mesh;
	}
}

HRESULT Engine::Update()
{
	OPTICK_EVENT();
#pragma region DeltaTime
	auto timePoint = std::chrono::high_resolution_clock::now();
	double deltaTime = std::chrono::duration_cast
		<std::chrono::nanoseconds>(timePoint - _lastFrameTime).count()
		/ 1000000000.0;

	_lastFrameTime = timePoint;

	//Catch large dT and void frame
	if (deltaTime > 1)
		return S_OK;
#pragma endregion

#if USE_OPTICK && 0
			static double frameTime = 0;
			frameTime += deltaTime;
			if (frameTime >= 5)
				PostQuitMessage(0);
#endif

	//Update Code

	_camera.Update(deltaTime * 5.0);
	for (unsigned i = 0; i < OBJECTS_TO_RENDER; i++)
		_objects[i].Update(_camera.Eye);

	return S_OK;
}

HRESULT Engine::Draw()
{
	OPTICK_EVENT();
	float backgroundColour[4] = { 0.025f, 0.025f, 0.025f, 1.0f };
	_deviceContext->OMSetRenderTargets(1, &_renderTarget, _depthStencilView);
	_deviceContext->ClearRenderTargetView(_renderTarget, backgroundColour);
	_deviceContext->ClearDepthStencilView(_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0.0f);

	D3D11_MAPPED_SUBRESOURCE cameraData;
	_cbCameraData.View = XMMatrixTranspose(XMLoadFloat4x4(&_camera.View));
	_cbCameraData.Projection = XMMatrixTranspose(XMLoadFloat4x4(&_camera.Projection));
	_cbCameraData.CameraPosition = {_camera.Eye.x, _camera.Eye.y, _camera.Eye.z, 1};
	_deviceContext->Map(_cbCamera, 0, D3D11_MAP_WRITE_DISCARD, 0, &cameraData);
	memcpy(cameraData.pData, &_cbCameraData, sizeof(CBCamera));
	_deviceContext->Unmap(_cbCamera, 0);

#if defined(_INSTANCED_RENDERER)
	unsigned lod0Count = 0;
	unsigned lod1Count = 0;
	unsigned lod2Count = 0;
	for (unsigned i = 0; i < OBJECTS_TO_RENDER; i++)
	{
		switch (RenderObject* ro = &_objects[i]; ro->Switch)
		{
		case 0:
			_lod0World[lod0Count].World = ro->Transform.GetWorldMatrix();
			lod0Count++;
			break;
		case 1:
			_lod1World[lod1Count].World = ro->Transform.GetWorldMatrix();
			lod1Count++;
			break;
		default:
			_lod2World[lod2Count].World = ro->Transform.GetWorldMatrix();
			lod2Count++;
			break;
		}
	}

	OPTICK_GPU_EVENT("GPU::Draw");
#if !defined(_INSTANCED_INPUT_LAYOUT)
	D3D11_MAPPED_SUBRESOURCE instanceData;
	HRESULT hr = _deviceContext->Map(_srvBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &instanceData); FAIL_CHECK
	memcpy(instanceData.pData, _lod0World, sizeof(PerInstanceBuffer) * lod0Count);
	_deviceContext->Unmap(_srvBuffer, 0);

#else
	D3D11_MAPPED_SUBRESOURCE instanceData;
	HRESULT hr = _deviceContext->Map(_perInstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &instanceData); FAIL_CHECK
	memcpy(instanceData.pData, _lod0World, sizeof(PerInstanceBuffer) * lod0Count);
	_deviceContext->Unmap(_perInstanceBuffer, 0);
#endif
	Mesh* m = _meshes[0];
	UINT stride = sizeof(PerVertexBuffer);
	UINT offset = 0;
	_deviceContext->IASetVertexBuffers(0, 1, &m->VertexBuffer, &stride, &offset);
	_deviceContext->IASetIndexBuffer(m->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	_deviceContext->DrawIndexedInstanced(m->IndicesCount, lod0Count, 0, 0, 0);

#if !defined(_INSTANCED_INPUT_LAYOUT)
	hr = _deviceContext->Map(_srvBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &instanceData); FAIL_CHECK
	memcpy(instanceData.pData, _lod1World, sizeof(PerInstanceBuffer) * lod1Count);
	_deviceContext->Unmap(_srvBuffer, 0);

#else
	hr = _deviceContext->Map(_perInstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &instanceData); FAIL_CHECK
	memcpy(instanceData.pData, _lod1World, sizeof(PerInstanceBuffer) * lod1Count);
	_deviceContext->Unmap(_perInstanceBuffer, 0);
#endif
	m = _meshes[1];
	_deviceContext->IASetVertexBuffers(0, 1, &m->VertexBuffer, &stride, &offset);
	_deviceContext->IASetIndexBuffer(m->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	_deviceContext->DrawIndexedInstanced(m->IndicesCount, lod1Count, 0, 0, 0);

#if !defined(_INSTANCED_INPUT_LAYOUT)
	hr = _deviceContext->Map(_srvBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &instanceData); FAIL_CHECK
	memcpy(instanceData.pData, _lod2World, sizeof(PerInstanceBuffer) * lod2Count);
	_deviceContext->Unmap(_srvBuffer, 0);

#else
	hr = _deviceContext->Map(_perInstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &instanceData); FAIL_CHECK
	memcpy(instanceData.pData, _lod2World, sizeof(PerInstanceBuffer) * lod2Count);
	_deviceContext->Unmap(_perInstanceBuffer, 0);
#endif
	m = _meshes[2];
	_deviceContext->IASetVertexBuffers(0, 1, &m->VertexBuffer, &stride, &offset);
	_deviceContext->IASetIndexBuffer(m->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	_deviceContext->DrawIndexedInstanced(m->IndicesCount, lod2Count, 0, 0, 0);

#else
	D3D11_MAPPED_SUBRESOURCE objectData;
	OPTICK_GPU_EVENT("GPU::Draw");
	for (unsigned sw = 0; sw < 3; sw++)
	{
		for (unsigned i = 0; i < OBJECTS_TO_RENDER; i++)
		{
			RenderObject* ro = &_objects[i];
			if (ro->Switch != sw)
				continue;
			Mesh* m = ro->LODMeshes[ro->Switch];
			_cbObjectData.World = ro->Transform.GetWorldMatrix();
			HRESULT hr = _deviceContext->Map(_cbObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &objectData); FAIL_CHECK
			memcpy(objectData.pData, &_cbObjectData, sizeof(CBObject));
			_deviceContext->Unmap(_cbObject, 0);

			UINT stride = sizeof(PerVertexBuffer);
			UINT offset = 0;
			_deviceContext->IASetVertexBuffers(0, 1, &m->VertexBuffer, &stride, &offset);
			_deviceContext->IASetIndexBuffer(m->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

			_deviceContext->DrawIndexed(m->IndicesCount, 0, 0);
		}
	}

#endif

	OPTICK_GPU_FLIP(_dxgiSwapChain);
	return _dxgiSwapChain->Present(0, 0);
}

HRESULT Engine::Initialise(HINSTANCE hInstance)
{
	OPTICK_EVENT();
	HRESULT hr = CreateWindowHandle(hInstance); FAIL_CHECK
	hr = CreateD3DDevice(); FAIL_CHECK
	hr = CreateSwapChain(); FAIL_CHECK

	hr = InitialiseRuntimeData(); FAIL_CHECK

	hr = CreateFrameBuffer(); FAIL_CHECK
	hr = InitialiseShaders(); FAIL_CHECK
	hr = InitialisePipeline(); FAIL_CHECK

	return hr;
}

HRESULT Engine::CreateWindowHandle(HINSTANCE hInstance)
{
	OPTICK_EVENT();
	const wchar_t* windowName = L"Dissertation Artefact";

	WNDCLASSW wndClass{};
	wndClass.style = 0;
	wndClass.lpfnWndProc = WndProc;
	wndClass.lpszClassName = windowName;

	RegisterClass(&wndClass);

	_hWnd = CreateWindowEx(0, windowName, windowName, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
	                       RESOLUTION_X, RESOLUTION_Y, nullptr, nullptr, hInstance, nullptr);

	return S_OK;
}

HRESULT Engine::CreateD3DDevice()
{
	OPTICK_EVENT();
	HRESULT hr = S_OK;

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

	return hr;
}

HRESULT Engine::CreateSwapChain()
{
	OPTICK_EVENT();
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

HRESULT Engine::InitialiseRuntimeData()
{
	OPTICK_EVENT();
	HRESULT hr = S_OK;
	srand(time(nullptr));

	_meshes[0] = Loader::WavefrontOBJLoader("Assets/tree_lod0.obj", _device);
	_meshes[1] = Loader::WavefrontOBJLoader("Assets/tree_lod1.obj", _device);
	_meshes[2] = Loader::WavefrontOBJLoader("Assets/tree_lod2.obj", _device);

	_objects = new RenderObject[OBJECTS_TO_RENDER];

	for (unsigned i = 0; i < OBJECTS_TO_RENDER; i++)
	{
		RenderObject* ro = new RenderObject();
		ro->Transform.SetPosition({
			(i % OBJECTS_WIDTH_COUNT * OBJECTS_UNIT_SIZE) - (OBJECTS_WIDTH_COUNT * OBJECTS_UNIT_SIZE / 2) + OBJECTS_UNIT_SIZE / 2.0f, 0,
			(i / OBJECTS_WIDTH_COUNT * OBJECTS_UNIT_SIZE)
		});
		for (unsigned j = 0; j < 3; j++)
			ro->LODMeshes[j] = _meshes[j];
		_objects[i] = *ro;
	}

#ifdef _INSTANCED_RENDERER
		_lod0World = new PerInstanceBuffer[OBJECTS_TO_RENDER];
		_lod1World = new PerInstanceBuffer[OBJECTS_TO_RENDER];
		_lod2World = new PerInstanceBuffer[OBJECTS_TO_RENDER];
#endif

#if defined(_INSTANCED_RENDERER) && defined(_INSTANCED_INPUT_LAYOUT)
	D3D11_BUFFER_DESC perInstanceBufferDesc = {};
	perInstanceBufferDesc.ByteWidth         = OBJECTS_TO_RENDER * sizeof(PerInstanceBuffer);
	perInstanceBufferDesc.Usage             = D3D11_USAGE_DYNAMIC;
	perInstanceBufferDesc.BindFlags         = D3D11_BIND_VERTEX_BUFFER;
	perInstanceBufferDesc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;

	hr = _device->CreateBuffer(&perInstanceBufferDesc, nullptr, &_perInstanceBuffer); FAIL_CHECK
#endif


#ifdef _INSTANCED_INPUT_LAYOUT
	UINT stride = sizeof(PerInstanceBuffer);
	UINT offset = 0;
	_deviceContext->IASetVertexBuffers(1, 1, &_perInstanceBuffer, &stride, &offset);
#endif

	_camera.FieldOfView = 90.0f;
	_camera.At = {0.0f, 0.0f, 1.0f};
	_camera.Up = {0.0f, 1.0f, 0.0f};

	_camera.NearDepth = 0.1f;
	_camera.FarDepth = 1000.0f;

	_camera.Eye = { 0, 10, -20 };

	XMStoreFloat4x4(&_camera.Projection,
		DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(_camera.FieldOfView),
			static_cast<float>(RESOLUTION_X) / static_cast<float>(RESOLUTION_Y),
			_camera.NearDepth,
			_camera.FarDepth));

	return hr;
}

HRESULT Engine::CreateFrameBuffer()
{
	OPTICK_EVENT();
	ID3D11Texture2D* frameBuffer = nullptr;
	HRESULT hr = _dxgiSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&frameBuffer)); FAIL_CHECK

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

	return hr;
}

HRESULT Engine::InitialiseShaders()
{
	OPTICK_EVENT();
	LPCWSTR vertexPath;
	LPCWSTR pixelPath = L"Engine/Shaders/PS.hlsl";

#if defined(_INSTANCED_RENDERER) && !defined(_INSTANCED_INPUT_LAYOUT)
	vertexPath = L"Engine/Shaders/VS_Instanced.hlsl";
#elif defined(_INSTANCED_RENDERER) && defined(_INSTANCED_INPUT_LAYOUT)
	vertexPath = L"Engine/Shaders/VS_InstancedInputLayout.hlsl";
#else
	vertexPath = L"Engine/Shaders/VS.hlsl";
#endif

	_vertexShader = CompileVertexShader(_hWnd, _device, &_inputLayout, vertexPath);
	_pixelShader = CompilePixelShader(_hWnd, _device, pixelPath);

	_deviceContext->VSSetShader(_vertexShader, nullptr, 0);
	_deviceContext->PSSetShader(_pixelShader, nullptr, 0);

	return S_OK;
}

HRESULT Engine::InitialisePipeline()
{
	OPTICK_EVENT();
	_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_deviceContext->IASetInputLayout(_inputLayout);

	D3D11_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.DepthClipEnable = true;
	HRESULT hr = _device->CreateRasterizerState(&rasterizerDesc, &_rasterizerState); FAIL_CHECK
	_deviceContext->RSSetState(_rasterizerState);

	_viewport = { 0.0f, 0.0f, RESOLUTION_X, RESOLUTION_Y, 0.0f, 1.0f };
	_deviceContext->RSSetViewports(1, &_viewport);

	D3D11_BUFFER_DESC bufferDesc{};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	bufferDesc.ByteWidth = sizeof(CBCamera);
	hr = _device->CreateBuffer(&bufferDesc, nullptr, &_cbCamera); FAIL_CHECK
	_deviceContext->VSSetConstantBuffers(0, 1, &_cbCamera);
	_deviceContext->PSSetConstantBuffers(0, 1, &_cbCamera);

#if !defined(_INSTANCED_RENDERER)
	bufferDesc.ByteWidth = sizeof(CBObject);
	hr = _device->CreateBuffer(&bufferDesc, nullptr, &_cbObject); FAIL_CHECK
	_deviceContext->VSSetConstantBuffers(1, 1, &_cbObject);
	_deviceContext->PSSetConstantBuffers(1, 1, &_cbObject);
#elif defined(_INSTANCED_RENDERER) && !defined(_INSTANCED_INPUT_LAYOUT)
	bufferDesc.ByteWidth = sizeof(PerInstanceBuffer) * OBJECTS_TO_RENDER;
	bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bufferDesc.StructureByteStride = sizeof(PerInstanceBuffer);

	hr = _device->CreateBuffer(&bufferDesc, nullptr, &_srvBuffer); FAIL_CHECK

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.NumElements = OBJECTS_TO_RENDER;

	hr = _device->CreateShaderResourceView(_srvBuffer, &srvDesc, &_srvInstance); FAIL_CHECK
	_deviceContext->VSSetShaderResources(0, 1, &_srvInstance);
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
}

HRESULT Engine::CreateVertexShaderLayout(ID3D11Device* device, ID3D11InputLayout** inputLayout, ID3DBlob* vsBlob)
{
	OPTICK_EVENT();
	D3D11_INPUT_ELEMENT_DESC vsInputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
#ifdef _INSTANCED_RENDERER
		{ "SV_InstanceID", 0, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
#endif

#ifdef _INSTANCED_INPUT_LAYOUT
		{ "WorldMatrix", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "WorldMatrix", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "WorldMatrix", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "WorldMatrix", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
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
	OPTICK_EVENT();
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
		RELEASE_RESOURCE(vsBlob)
		RELEASE_RESOURCE(errorBlob)
		return nullptr;
	}


	hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vs);
	if(FAILED(hr))
	{
		RELEASE_RESOURCE(vsBlob)
		RELEASE_RESOURCE(errorBlob)
		return nullptr;
	}

	hr = CreateVertexShaderLayout(device, inputLayout, vsBlob);
	if(FAILED(hr))
	{
		RELEASE_RESOURCE(vsBlob)
		RELEASE_RESOURCE(errorBlob)
		return nullptr;
	}

	RELEASE_RESOURCE(vsBlob)
	RELEASE_RESOURCE(errorBlob)

	return vs;
}

ID3D11PixelShader* Engine::CompilePixelShader(HWND hWnd, ID3D11Device* device, LPCWSTR path)
{
	OPTICK_EVENT();
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
		RELEASE_RESOURCE(psBlob)
		RELEASE_RESOURCE(errorBlob)
		return nullptr;
	}


	hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &ps);
	if(FAILED(hr))
	{
		RELEASE_RESOURCE(psBlob)
		RELEASE_RESOURCE(errorBlob)
		return nullptr;
	}

	RELEASE_RESOURCE(psBlob)
	RELEASE_RESOURCE(errorBlob)

	return ps;
}