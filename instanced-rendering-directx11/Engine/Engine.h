#pragma once

#include <chrono>
#include <windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <dxgi1_2.h>
#include <vector>

#include "Camera.h"

#include "Structs/CBCamera.h"
#ifndef _INSTANCED_RENDERER
#include "Structs/CBObject.h"
#endif

#define OBJECTS_TO_RENDER 160000

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

	std::vector<DirectX::XMFLOAT3> _positions{};
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

