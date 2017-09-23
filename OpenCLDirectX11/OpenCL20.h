#ifndef OCL_H
#define OCL_H

#define g_ScreenWidth 1920
#define g_ScreenHeight 1200
#define SHARED_IMAGE_HEIGHT g_ScreenHeight
#define SHARED_IMAGE_WIDTH g_ScreenWidth

#include "vector3.h"




#include <CL/cl.h>
#include <Windows.h> //needed to make DX11 header inclusion errors go away

#include <CL/cl_d3d11.h> 

#include <string.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>

#include "common.h"




//FOR SHARING
extern cl_mem		g_SharedRGBAimageCLMemObject;
extern cl_mem		g_SharedRGBAimageCLMemObjectBlur;
extern bool			g_SharedResourceFlagPerformanceFlag;

//verification if support for surface sharing exists
int DX11SurfaceSharingPlatformAvailable();
int SetSharedResourceFlagByPerformance(); //returns TRUE if sharing is faster with this flag on surface creation on DX11 side

//entry point for initialization of CL platform, creation of context, queue, etc.
int StartupOpenCL(void);
//compile program and create kernels to be used on device side
int BuildKernels();

//CL/DX11 sharing, major API call for this tutorial
int GetDX11SharingExtensions();
int ShareDX11BufferWithCL();

//run per frame
void RunKernelsSDF(double seconds);
int ShutdownOpenCL(void);

//Debugging functions
int handleCompileError(void);
int convertToString(const char *filename, char *buf);

//utility functions
//testStatus is simplified to minimize extraneous code in a sample
void CheckResult(int status, char *errorMsg);

#endif OCL_H
