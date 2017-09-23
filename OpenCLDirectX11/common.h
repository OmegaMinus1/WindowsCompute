#ifndef COMMON_CL_DX11_H
#define COMMON_CL_DX11_H

#define CL_DEVICE_CPU_OR_GPU CL_DEVICE_TYPE_GPU

#define SUCCESS 0
#define FAIL -1

#define VERTEX_SHADER L"vertexshader.hlsl"
#define PIXEL_SHADER L"pixelshader.hlsl"

#define CL_KERNELS "SDFRendering.cl"
#define CL_KERNEL_BLUR "blur.cl"

//#define USE_GL_PIXEL_UNPACK_BUFFERS 0 //used as an option only for initialization

#define NUM_IMAGE_CHANNELS 16
#define NUM_VERTICES 6
#define REMOTE_DEBUG 0

extern ID3D11Device*	g_pd3dDevice;
extern ID3D11Texture2D*		g_pSharedDX11Texture2D;
extern ID3D11Texture2D*		g_pSharedDX11Texture2DBlur;

extern float vay;
extern float const PI_180ED;

extern float view_point_x, view_point_y, view_point_z;
extern float view_angle_x, view_angle_y, view_angle_z;
extern float oldvpx, oldvpy, oldvpz;
extern float nosex, nosey, nosez;

extern int Running;
extern int pointerx, pointery;
extern int oldpointerx, oldpointery;


#endif COMMON_CL_DX11_H