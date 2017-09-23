#ifndef _DX11_H_
#define _DX11_H_

#include <Windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>

#include <stdio.h>

#include "common.h"
#include "OpenCL20.h"

extern HINSTANCE					g_hInst;
extern HWND							g_hWnd;
extern D3D_DRIVER_TYPE				g_driverType;
extern D3D_FEATURE_LEVEL			g_featureLevel;
extern ID3D11Device*				g_pd3dDevice;
extern ID3D11Device1*				g_pd3dDevice1;
extern ID3D11DeviceContext*			g_pImmediateContext;
extern ID3D11DeviceContext1*		g_pImmediateContext1;
extern IDXGISwapChain*				g_pSwapChain;
extern IDXGISwapChain1*				g_pSwapChain1;
extern ID3D11RenderTargetView*		g_pRenderTargetView;
extern ID3D11VertexShader*			g_pVertexShader;
extern ID3D11PixelShader*			g_pPixelShader;
extern ID3D11InputLayout*			g_pVertexLayout;
extern ID3D11Buffer*				g_pVertexBuffer;

//Shared texture
extern ID3D11Texture2D*		 		g_pSharedDX11Texture2D;
extern ID3D11ShaderResourceView*	g_pShaderResourceView;
extern ID3D11Texture2D*		 		g_pSharedDX11Texture2DBlur;
extern ID3D11ShaderResourceView*	g_pShaderResourceViewBlur;
extern ID3D11SamplerState*			g_pSamplerLinear;

HRESULT InitWindowDX11(HINSTANCE hInstance, int nCmdShow);
HRESULT InitDeviceDX11();
void CleanupDeviceDX11();
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
void RenderDX11();
void CreateTextureDX11();
int InitTextureDX11();
void CreateTextureDX11();
void CreateBlurTextureDX11();
void CreateShaderResourceViewDX11();
void CreateSamplerStateDX11();
void CleanupDeviceDX11();

#endif _DX11_H_

