#include "DirectX11.h"

using namespace DirectX;

HINSTANCE					g_hInst = nullptr;
HWND						g_hWnd = nullptr;
D3D_DRIVER_TYPE				g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL			g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*				g_pd3dDevice = nullptr;
ID3D11Device1*				g_pd3dDevice1 = nullptr;
ID3D11DeviceContext*		g_pImmediateContext = nullptr;
ID3D11DeviceContext1*		g_pImmediateContext1 = nullptr;
IDXGISwapChain*				g_pSwapChain = nullptr;
IDXGISwapChain1*			g_pSwapChain1 = nullptr;
ID3D11RenderTargetView*		g_pRenderTargetView = nullptr;
ID3D11VertexShader*			g_pVertexShader = nullptr;
ID3D11PixelShader*			g_pPixelShader = nullptr;
ID3D11InputLayout*			g_pVertexLayout = nullptr;
ID3D11Buffer*				g_pVertexBuffer = nullptr;

//for the shared texture
ID3D11Texture2D*			g_pSharedDX11Texture2D = nullptr;
ID3D11Texture2D*			g_pSharedDX11Texture2DBlur = nullptr;
ID3D11ShaderResourceView*	g_pShaderResourceView = nullptr;
ID3D11ShaderResourceView*	g_pShaderResourceViewBlur = nullptr;
ID3D11SamplerState*			g_pSamplerLinear = nullptr;

struct SimpleVertex
{
	XMFLOAT4 pos;
};

struct VertexAndTexture2D
{
	XMFLOAT4 pos; //4 floats, 4 bytes per float * 4 = 16 bytes between each
	XMFLOAT2 tex;// 2 floats, 4 bytes per float * 2 = 8 bytes
}; //24 bytes    total 


void RenderDX11()
{
	//clear the back buffer
	//g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, Colors::Yellow);

	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pShaderResourceView);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

	//render a triangle
	g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);
	g_pImmediateContext->Draw(NUM_VERTICES, 0);

	//preset the information renderd to the back buffer to the front buffer
	g_pSwapChain->Present(0, 0);
}

int InitTextureDX11()
{
	CreateTextureDX11();
	CreateBlurTextureDX11();
	CreateShaderResourceViewDX11();
	CreateSamplerStateDX11();

	return S_OK;
}

void CreateTextureDX11()
{
	unsigned char *texture = NULL;
	texture = (unsigned char *)malloc(sizeof(unsigned char) * NUM_IMAGE_CHANNELS * SHARED_IMAGE_HEIGHT * SHARED_IMAGE_WIDTH);
	if (texture == nullptr)
	{
		printf("error creating texture\n");
	}

	for (unsigned int i = 0; i<NUM_IMAGE_CHANNELS * SHARED_IMAGE_HEIGHT * SHARED_IMAGE_WIDTH;)
	{
		texture[i++] = 255;
		texture[i++] = 0;
		texture[i++] = 0;
		texture[i++] = 255;
	}

	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
	desc.Width = SHARED_IMAGE_WIDTH;
	desc.Height = SHARED_IMAGE_HEIGHT;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; //sending in unsigned values representing floats from 0...255
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	if (g_SharedResourceFlagPerformanceFlag == true)
	{
		printf("Using the D3D11_RESOURCE_MISC_SHARED flag\n");
		desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
	}
	else
	{
		desc.MiscFlags = 0;

	}

	D3D11_SUBRESOURCE_DATA tbsd;
	ZeroMemory(&tbsd, sizeof(D3D11_SUBRESOURCE_DATA));
	tbsd.pSysMem = (void *)texture;
	tbsd.SysMemPitch = SHARED_IMAGE_WIDTH * NUM_IMAGE_CHANNELS;
	tbsd.SysMemSlicePitch = SHARED_IMAGE_WIDTH * SHARED_IMAGE_HEIGHT * NUM_IMAGE_CHANNELS;
	g_pd3dDevice->CreateTexture2D(&desc, &tbsd, &g_pSharedDX11Texture2D);
	//still need to bind

	free(texture);
}

void CreateBlurTextureDX11()
{
	unsigned char *texture = NULL;
	texture = (unsigned char *)malloc(sizeof(unsigned char) * NUM_IMAGE_CHANNELS * SHARED_IMAGE_HEIGHT * SHARED_IMAGE_WIDTH);
	if (texture == nullptr)
	{
		printf("error creating texture\n");
	}

	for (unsigned int i = 0; i<NUM_IMAGE_CHANNELS * SHARED_IMAGE_HEIGHT * SHARED_IMAGE_WIDTH;)
	{
		texture[i++] = 255;
		texture[i++] = 0;
		texture[i++] = 0;
		texture[i++] = 255;
	}

	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
	desc.Width = SHARED_IMAGE_WIDTH;
	desc.Height = SHARED_IMAGE_HEIGHT;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; //sending in unsigned values representing floats from 0...255
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	if (g_SharedResourceFlagPerformanceFlag == true)
	{
		printf("Using the D3D11_RESOURCE_MISC_SHARED flag\n");
		desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
	}
	else
	{
		desc.MiscFlags = 0;

	}

	D3D11_SUBRESOURCE_DATA tbsd;
	ZeroMemory(&tbsd, sizeof(D3D11_SUBRESOURCE_DATA));
	tbsd.pSysMem = (void *)texture;
	tbsd.SysMemPitch = SHARED_IMAGE_WIDTH * NUM_IMAGE_CHANNELS;
	tbsd.SysMemSlicePitch = SHARED_IMAGE_WIDTH * SHARED_IMAGE_HEIGHT * NUM_IMAGE_CHANNELS;
	g_pd3dDevice->CreateTexture2D(&desc, &tbsd, &g_pSharedDX11Texture2DBlur);
	//still need to bind

	free(texture);
}

void CreateShaderResourceViewDX11()
{
	//can use NULL if you want to use same parameters for view, nice!
	g_pd3dDevice->CreateShaderResourceView((ID3D11Resource *)g_pSharedDX11Texture2D, NULL, &g_pShaderResourceView);
	g_pd3dDevice->CreateShaderResourceView((ID3D11Resource *)g_pSharedDX11Texture2DBlur, NULL, &g_pShaderResourceViewBlur);
}

void CreateSamplerStateDX11()
{
	HRESULT hr;

	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(samplerDesc));
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP; //TODO: needed?
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX; //TODO: looks fishy
	hr = g_pd3dDevice->CreateSamplerState(&samplerDesc, &g_pSamplerLinear);
}

//Register class and create window
HRESULT InitWindowDX11(HINSTANCE hInstance, int nCmdShow)
{
	//register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = L"OpenCLDX11";
	wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	if (!RegisterClassEx(&wcex))
	{
		return E_FAIL;
	}

	//create window
	g_hInst = hInstance;
	RECT rc = { 0, 0, g_ScreenWidth, g_ScreenHeight };
	AdjustWindowRect(&rc, WS_POPUPWINDOW, FALSE);
	g_hWnd = CreateWindow(
		L"OpenCLDX11",
		L"SDF Rendering",
		WS_POPUPWINDOW,
		0,
		0,
		rc.right - rc.left,
		rc.bottom - rc.top,
		nullptr,
		nullptr,
		hInstance,
		nullptr);
	
	if (!g_hWnd)
	{
		return E_FAIL;
	}
	ShowWindow(g_hWnd, nCmdShow);

	//create window
	return S_OK;
}

HRESULT CompileShaderFromFile(WCHAR *szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob **ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	dwShaderFlags |= D3DCOMPILE_DEBUG;
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pErrorBlob = nullptr;
	//TODO: see this file on d3dcompiler later: http://msdn.microsoft.com/en-us/library/windows/desktop/hh404562(v=vs.85).aspx

	hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel, dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char *> (pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		return hr;
	}

	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}

HRESULT InitDeviceDX11()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};

	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		g_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);

		if (hr == E_INVALIDARG)
		{
			hr = D3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1, D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
		}

		if (SUCCEEDED(hr))
		{
			break;
		}
	}

	if (FAILED(hr))
	{
		return hr;
	}

	//Obtain DXGI factory from device (since we used nullptr for pAdapter above per example code from MS
	IDXGIFactory1* dxgiFactory = nullptr;
	{
		IDXGIDevice *dxgiDevice = nullptr;
		hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
		if (SUCCEEDED(hr))
		{
			IDXGIAdapter *adapter = nullptr;
			hr = dxgiDevice->GetAdapter(&adapter);
			if (SUCCEEDED(hr))
			{
				hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
				adapter->Release();
			}
			dxgiDevice->Release();
		}
	}
	if (FAILED(hr))
		return hr;

	//create swap chain
	IDXGIFactory2 *dxgiFactory2 = nullptr;
	hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));
	if (dxgiFactory2)
	{
		//dx11.1 or later
		hr = g_pd3dDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&g_pd3dDevice1));
		if (SUCCEEDED(hr))
		{
			(void)g_pImmediateContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&g_pImmediateContext1));
		}

		DXGI_SWAP_CHAIN_DESC1 sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.Width = width;
		sd.Height = height;
		sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;// DXGI_FORMAT_R10G10B10A2_UNORM;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 1;
		
		DXGI_SWAP_CHAIN_FULLSCREEN_DESC sd2;
		ZeroMemory(&sd2, sizeof(sd2));
		sd2.RefreshRate.Numerator = 60;
		sd2.RefreshRate.Denominator = 1;
		sd2.Windowed = false;

		hr = dxgiFactory2->CreateSwapChainForHwnd(g_pd3dDevice, g_hWnd, &sd, &sd2, nullptr, &g_pSwapChain1);
		if (SUCCEEDED(hr))
		{
			hr = g_pSwapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&g_pSwapChain));
		}
		dxgiFactory2->Release();
	}
	else
	{
		//DX11.0 systems handled here
		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.BufferCount = 1;
		sd.BufferDesc.Width = width;
		sd.BufferDesc.Height = height;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;// DXGI_FORMAT_R10G10B10A2_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = g_hWnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = false;

		hr = dxgiFactory->CreateSwapChain(g_pd3dDevice, &sd, &g_pSwapChain);
	}

	dxgiFactory->Release();

	if (FAILED(hr))
		return hr;

	//create a render target view
	ID3D11Texture2D *pBackBuffer = nullptr;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
	if (FAILED(hr))
	{
		return hr;
	}

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
	{
		return hr;
	}
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);

	//setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 0.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);

	//compiler the vertex shader
	ID3DBlob *pVSBlob = nullptr;
	hr = CompileShaderFromFile(VERTEX_SHADER, "VS", "vs_5_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr, L"The FX File Canont be compiled, please run executable from directory that contains the fx file.", L"Error", MB_OK);
		return hr;
	}

	//create the vertex shader
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	//define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,			0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	//create the input layout
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_pVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr))
	{
		return hr;
	}

	//set the input layout
	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

	//compile the pixel shader
	ID3DBlob *pPSBlob = nullptr;
	hr = CompileShaderFromFile(PIXEL_SHADER, "PS", "ps_5_0", &pPSBlob);
	if (FAILED(hr))
	{
		return hr;
	}

	//create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
	pPSBlob->Release();
	
	if (FAILED(hr))
		return hr;

	//create vertex buffer
	VertexAndTexture2D vertsWithTexture[] = /* note winding order is opposite OpenGL. OGL  is CCW, D3D is CW */
	{
		XMFLOAT4(-1.0f, -1.0f, 0.5f, 1.0f),		XMFLOAT2(1.0f, 0.0f),  //lower left
		XMFLOAT4(-1.0f,  1.0f, 0.5f, 1.0f),		XMFLOAT2(1.0f, 1.0f), //upper left
		XMFLOAT4(1.0f, -1.0f, 0.5f, 1.0f),		XMFLOAT2(0.0f, 0.0f), //lower right

		XMFLOAT4(1.0f, -1.0f, 0.5f, 1.0f),		XMFLOAT2(0.0f, 0.0f), //lower right
		XMFLOAT4(-1.0f, 1.0f, 0.5f, 1.0f),		XMFLOAT2(1.0f, 1.0f), //upper left
		XMFLOAT4(1.0f,  1.0f, 0.5f, 1.0f),		XMFLOAT2(0.0f, 1.0f), //upper right
	};

	int num_verts = 6;
	SimpleVertex vertices[] =
	{
		XMFLOAT4(0.0f,0.5f,0.5f, 1.0f),
		XMFLOAT4(0.5f, -0.5f, 0.5f, 1.0f),
		XMFLOAT4(-0.5f, -0.5f, 0.5f, 1.0f),
	};

	num_verts = NUM_VERTICES;
	
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(VertexAndTexture2D) * num_verts;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	
	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = vertsWithTexture;
	
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
	if (FAILED(hr))
	{
		return hr;
	}

	//set vertex buffer
	UINT stride = sizeof(VertexAndTexture2D);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	//set primitive topology
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return S_OK;
}

void CleanupDeviceDX11()
{
	if (g_pImmediateContext) g_pImmediateContext->ClearState();

	if (g_pSharedDX11Texture2D) g_pSharedDX11Texture2D->Release();
	if (g_pShaderResourceView) g_pShaderResourceView->Release();
	if (g_pSharedDX11Texture2DBlur) g_pSharedDX11Texture2DBlur->Release();
	if (g_pShaderResourceViewBlur) g_pShaderResourceViewBlur->Release();
	if (g_pSamplerLinear) g_pSamplerLinear->Release();

	if (g_pVertexBuffer) g_pVertexBuffer->Release();
	if (g_pVertexLayout) g_pVertexLayout->Release();
	if (g_pVertexShader) g_pVertexShader->Release();
	if (g_pPixelShader) g_pPixelShader->Release();
	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pSwapChain1) g_pSwapChain1->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext1) g_pImmediateContext->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice1) g_pd3dDevice1->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();

	CloseWindow(g_hWnd);

}
