#pragma once

#include <chrono>
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#include "Camera.h"
#include "Mesh.h"
#include "RenderObject.h"

#include "Structs/CBCamera.h"
#ifdef _INSTANCED_RENDERER
#include "Structs/PerInstanceBuffer.h"
#endif
#ifndef _INSTANCED_RENDERER
#include "Structs/CBObject.h"
#endif

// Major Settings
constexpr unsigned OBJECTS_TO_RENDER = 500000;
constexpr unsigned OBJECTS_WIDTH_COUNT = 128;
constexpr float OBJECTS_UNIT_SIZE =  16;
constexpr unsigned RESOLUTION_X = 1280;
constexpr unsigned RESOLUTION_Y = 720;
#define RENDER 1
#define UPDATE 1
#define BACKWARDS_RENDER 0

class Engine
{
public:
	~Engine();

	HRESULT Initialise(HINSTANCE hInstance);

	HRESULT Update();
	HRESULT Draw();

private:
	HRESULT CreateWindowHandle(HINSTANCE hInstance);
	HRESULT CreateD3DDevice();
	HRESULT CreateSwapChain();
	HRESULT CreateFrameBuffer();
	HRESULT InitialiseShaders();
	HRESULT InitialisePipeline();
	HRESULT InitialiseRuntimeData();

	static HRESULT CreateVertexShaderLayout(ID3D11Device* device, ID3D11InputLayout** inputLayout, ID3DBlob* vsBlob);
	static ID3D11VertexShader* CompileVertexShader(HWND hWnd, ID3D11Device* device, ID3D11InputLayout** inputLayout, LPCWSTR path);
	static ID3D11PixelShader* CompilePixelShader(HWND hWnd, ID3D11Device* device, LPCWSTR path);

	HWND _hWnd{};

	RenderObject* _objects = nullptr;
	Mesh* _meshes[3] = {};
#if defined(_INSTANCED_RENDERER)
	PerInstanceBuffer* _lod0World = nullptr;
	PerInstanceBuffer* _lod1World = nullptr;
	PerInstanceBuffer* _lod2World = nullptr;
#endif
	Camera _camera{};

	ID3D11Device* _device = nullptr;
	ID3D11DeviceContext* _deviceContext = nullptr;
	ID3D11RenderTargetView* _renderTarget = nullptr;
	ID3D11InputLayout* _inputLayout = nullptr;
	ID3D11RasterizerState* _rasterizerState = nullptr;

	ID3D11Texture2D* _depthStencilBuffer = nullptr;
	ID3D11DepthStencilView* _depthStencilView = nullptr;

	IDXGIDevice* _dxgiDevice = nullptr;
	IDXGIFactory2* _dxgiFactory = nullptr;
	IDXGISwapChain1* _dxgiSwapChain = nullptr;

	ID3D11Buffer* _perVertexBuffer = nullptr;
#if defined(_INSTANCED_RENDERER) && defined(_INSTANCED_INPUT_LAYOUT)
	ID3D11Buffer* _perInstanceBuffer = nullptr;
#endif

	ID3D11Buffer* _indexBuffer = nullptr;
	ID3D11VertexShader* _vertexShader = nullptr;
	ID3D11PixelShader* _pixelShader = nullptr;

	ID3D11SamplerState* _bilinearSampler = nullptr;

	ID3D11Buffer* _cbCamera = nullptr;
	CBCamera _cbCameraData{};

#ifndef _INSTANCED_RENDERER
	ID3D11Buffer* _cbObject = nullptr;
	CBObject _cbObjectData{};
#endif

#ifdef _INSTANCED_RENDERER
	ID3D11Buffer* _srvBuffer = nullptr;
	ID3D11ShaderResourceView* _srvInstance = nullptr;
#endif

	D3D11_VIEWPORT _viewport{};

	std::chrono::time_point<std::chrono::steady_clock> _lastFrameTime = std::chrono::high_resolution_clock::now();
};

