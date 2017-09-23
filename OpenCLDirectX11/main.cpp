#include <Windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>

#include <stdio.h>

#include "common.h"
#include "OpenCL20.h"
#include "DirectX11.h"
using namespace DirectX;

//Ray Camera
float vay = 0.0;
float const PI_180ED = 3.1415693f / 180.0f;

float view_point_x = 7.624411f, view_point_y = 2.311336f, view_point_z = 6.098149f;
float view_angle_x = 95, view_angle_y = 15, view_angle_z = 0;
float oldvpx, oldvpy, oldvpz;
float nosex, nosey, nosez;

int Running = 0;
int pointerx, pointery;
int oldpointerx, oldpointery;

//Time/Clock
__int64  Start_clk;
__int64  Current_clk;
__int64  End_clk;
__int64  Elapsed_clk;
__int64  Freg_clk;


void process_mouse()
{
	oldpointerx = pointerx;
	oldpointery = pointery;

	POINT po;
	GetCursorPos(&po);
	pointerx = po.x;
	pointery = po.y;


	if (abs(oldpointerx - pointerx) > 2)
	{
		if (pointerx < oldpointerx)
		{
			view_angle_y += 5;
			if (view_angle_y > 360.0) view_angle_y -= 360;
		};

		if (pointerx > oldpointerx)
		{
			view_angle_y -= 5;
			if (view_angle_y < 0.0) view_angle_y += 360;
		};
	}

	if (abs(oldpointery - pointery) > 2)
	{
		if (pointery < oldpointery)
		{
			view_angle_x += 5;
			if (view_angle_x > 180.0) view_angle_x = 180;

		};

		if (pointery > oldpointery)
		{
			view_angle_x -= 5;
			if (view_angle_x < 0.0) view_angle_x = 0;

		};
	}

	if (pointerx >= g_ScreenWidth - 40)
	{
		pointerx = g_ScreenWidth / 2;
		oldpointerx = pointerx - 40;

	}
	else if (pointerx <= 40)
	{
		pointerx = g_ScreenWidth / 2;
		oldpointerx = pointerx + 40;

	};

	if (pointery >= g_ScreenHeight - 40)
	{
		pointery = g_ScreenHeight / 2;
		oldpointery = pointery - 40;

	}
	else if (pointery <= 40)
	{
		pointery = g_ScreenHeight / 2;
		oldpointery = pointery + 40;

	};
	SetCursorPos(pointerx, pointery);
};

void process_key()
{
	if (GetAsyncKeyState(0x41))
	{
		oldvpy = view_point_y;
		view_point_y += 2;
	};

	if (GetAsyncKeyState(0x5A))
	{
		oldvpy = view_point_y;
		view_point_y -= 2;
	};

	if (GetAsyncKeyState(0x51))
	{
		Running = 0;
	};

	if (GetAsyncKeyState(0x57)) //forward
	{
		vector3 v;
		v.x = 0.0f;
		v.y = -1.0f;
		v.z = 0.0f;

		RotateX(v, view_angle_x * PI_180ED);
		RotateY(v, view_angle_y * PI_180ED);
		RotateZ(v, view_angle_z * PI_180ED);

		view_point_x += v.x;
		view_point_y += v.y;
		view_point_z += v.z;

	};

	if (GetAsyncKeyState(0x44)) //strafe right
	{
		float vay = view_angle_y - 90;
		if (vay > 0) vay += 360;

		vector3 v;
		v.x = 0.0f;
		v.y = -1.0f;
		v.z = 0.0f;

		RotateX(v, view_angle_x * PI_180ED);
		RotateY(v, vay * PI_180ED);
		RotateZ(v, view_angle_z * PI_180ED);

		view_point_x += v.x;
		view_point_y += v.y;
		view_point_z += v.z;
	};

	if (GetAsyncKeyState(0x43))
	{
		//crouch
	};

	if (GetAsyncKeyState(0x41)) //strafe left
	{
		float vay = view_angle_y + 90;
		if (vay < 360) vay -= 360;

		vector3 v;
		v.x = 0.0f;
		v.y = -1.0f;
		v.z = 0.0f;

		RotateX(v, view_angle_x * PI_180ED);
		RotateY(v, vay * PI_180ED);
		RotateZ(v, view_angle_z * PI_180ED);

		view_point_x += v.x;
		view_point_y += v.y;
		view_point_z += v.z;
	};

	if (GetAsyncKeyState(0x53)) //backward
	{
		vector3 v;
		v.x = 0.0f;
		v.y = 1.0f;
		v.z = 0.0f;

		RotateX(v, view_angle_x * PI_180ED);
		RotateY(v, view_angle_y * PI_180ED);
		RotateZ(v, view_angle_z * PI_180ED);

		view_point_x += v.x;
		view_point_y += v.y;
		view_point_z += v.z;
	};
};

void Render(double seconds)
{
	process_mouse();
	process_key();

	RunKernelsSDF(seconds);
	RenderDX11();
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	int status;

	status = DX11SurfaceSharingPlatformAvailable();

	if (FAILED(InitWindowDX11(hInstance, nCmdShow)))
		return 0;

	if (FAILED(InitDeviceDX11()))
	{
		CleanupDeviceDX11();
		return 0;
	}

	status = StartupOpenCL();
	CheckResult(status, "initCL failed\n");

	//seperated from initCL because InitCL is boilerplate for any CL app
	status = GetDX11SharingExtensions();
	CheckResult(status, "Obtaining DX11 sharing functions failed\n");

	SetSharedResourceFlagByPerformance();

	//note that this needs to happen AFTER the UseXXXFlagCL() call is made
	if (FAILED(InitTextureDX11()))
	{
		return 0;
	}

	status = ShareDX11BufferWithCL();
	CheckResult(status, "Sharing Failed\n");

	QueryPerformanceFrequency((LARGE_INTEGER *)&Freg_clk);
	QueryPerformanceCounter((LARGE_INTEGER *)&Start_clk);

	float fps = 0.0f;
	double lastEnd_clk = 0.0;
	//main message loop
	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			QueryPerformanceCounter((LARGE_INTEGER *)&End_clk);
			
			fps = (float)(Freg_clk / (End_clk - lastEnd_clk));
			
			double seconds = ((double)(End_clk - Start_clk) / (double)(Freg_clk));

			Render(seconds);

			lastEnd_clk = (double)End_clk;
			
		}
	}
	
	CleanupDeviceDX11();
	ShutdownOpenCL();

    char buffer[1024];
	memset(buffer, 0, 1024);
	sprintf_s(buffer, 1024, "FPS %f", fps);
	
	MessageBoxA(NULL,buffer,"Spinning @",MB_OK);

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			switch (wParam)
			{
				case VK_ESCAPE:
					PostQuitMessage(0);
					return 0;
					break;
				case VK_SPACE:
					break;
			}
		}
		break;
		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

