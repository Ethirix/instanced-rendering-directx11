#pragma once
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>

class Engine
{
public:
	~Engine();

	HRESULT Initialise(HINSTANCE hInstance);

	void Update();
	void Draw();

private:
	HRESULT CreateWindowHandle(HINSTANCE hInstance);
	HRESULT CreateD3DDevice();
	HRESULT CreateSwapChain();
	HRESULT CreateFrameBuffer();
	HRESULT InitialiseShaders();
	HRESULT InitialisePipeline();

	static HRESULT CreateVertexShaderLayout(ID3D11Device* device, ID3D11InputLayout* inputLayout, ID3DBlob* vsBlob);
	static ID3D11VertexShader* CompileVertexShader(HWND hWnd, ID3D11Device* device, ID3D11InputLayout* inputLayout, LPCWSTR path);
	static ID3D11PixelShader* CompilePixelShader(HWND hWnd, ID3D11Device* device, LPCWSTR path);

	HWND _hWnd{};

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

	ID3D11VertexShader* _vertexShader = nullptr;
	ID3D11PixelShader* _pixelShader = nullptr;

	D3D11_VIEWPORT _viewport{};
};

