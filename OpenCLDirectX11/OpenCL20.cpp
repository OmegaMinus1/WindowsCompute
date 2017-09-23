#include "OpenCL20.h"

#include "memBlock.h"
#include "memmanOCL.h"

cl_context			g_clContext;
cl_platform_id		g_platformToUse;
cl_device_id *		g_clDevices = NULL;
cl_command_queue	g_clCommandQueue;
cl_program			g_clProgram;
char *				g_clProgramString = NULL;

cl_mem				g_SharedRGBAimageCLMemObject;
cl_mem				g_SharedRGBAimageCLMemObjectBlur;
bool				g_SharedResourceFlagPerformanceFlag;

//Kernels
cl_kernel			cl_kernel_RenderSDF;
cl_kernel			cl_kernel_blur;

//Memory Manager
memmanOCL *memman;

//Blur Kernel Filter
int filterWidth = 9;
memBlock *filterBuffer;

//Extension Functions
clCreateFromD3D11Texture2DKHR_fn   ptrF_clCreateFromD3D11Texture2DKHR = NULL;
clEnqueueAcquireD3D11ObjectsKHR_fn ptrF_clEnqueueAcquireD3D11ObjectsKHR = NULL;
clEnqueueReleaseD3D11ObjectsKHR_fn ptrF_clEnqueueReleaseD3D11ObjectsKHR = NULL;

#pragma region Helper Functions

void CheckResult(int result, char *errorMsg)
{
	if(result != SUCCESS)
	{
		if(errorMsg == NULL)
		{
			MessageBoxA(NULL, "No error message provided!", "Error OpenCL Compiler", MB_OK);
		}
		else
		{
			MessageBoxA(NULL, errorMsg, "Error OpenCL Compiler", MB_OK);
		}
		exit(EXIT_FAILURE);
	}
}
int CheckCompileResult(void)
{
	cl_int logStatus;
	
	char *buildLog = NULL;
	size_t buildLogSize = 0;
	
	logStatus = clGetProgramBuildInfo( g_clProgram, g_clDevices[0], CL_PROGRAM_BUILD_LOG, buildLogSize, buildLog, &buildLogSize);
	if(logStatus != CL_SUCCESS)
	{
		MessageBoxA(NULL, "Unable to get Error Build Log", "Error OpenCL Compiler", MB_OK); 
		exit(EXIT_FAILURE);
	}

	buildLog = (char *)malloc(buildLogSize);
	if(buildLog == NULL)
	{
		MessageBoxA(NULL, "No memory to store Error Build Log", "Error OpenCL Compiler", MB_OK);
		exit(EXIT_FAILURE);
	}

	memset(buildLog, 0, buildLogSize);

	logStatus = clGetProgramBuildInfo (g_clProgram, g_clDevices[0], CL_PROGRAM_BUILD_LOG, buildLogSize, buildLog, NULL);
	if(logStatus != CL_SUCCESS)
	{
		free(buildLog);
		return FAIL;
	}
	
	MessageBoxA(NULL, buildLog, "Error OpenCL Compiler", MB_OK);
    
	free(buildLog);

	return SUCCESS;
}
int LoadKernelFromFile(const char *fileName)
{
	FILE *fp = NULL;
	int status;

	char buffer[1024];
	memset(buffer, 0, 1024);

	errno_t err = fopen_s(&fp, fileName, "r");
	if(err != 0)
	{
		sprintf_s(buffer,1024, "Error opening %s, check path", fileName);
		MessageBoxA(NULL, buffer, "Error", MB_OK);
		exit(EXIT_FAILURE);
	}

	status = fseek(fp, 0, SEEK_END);
	if(status != 0)
	{
		sprintf_s(buffer, 1024, "Error finding end of file");
		MessageBoxA(NULL, buffer, "Error", MB_OK);
		exit(EXIT_FAILURE);
	}

	int len = ftell(fp);
	if(len == -1L)
	{
		sprintf_s(buffer, 1024, "Error reporting position of file pointer");
		MessageBoxA(NULL, buffer, "Error", MB_OK);
		exit(EXIT_FAILURE);
	}
	
	rewind(fp);
	
	g_clProgramString = (char *)malloc((len * sizeof(char))+1);
	if(g_clProgramString == NULL)
	{
		sprintf_s(buffer, 1024, "Error in allocation when converting CL source file to a string");
		MessageBoxA(NULL, buffer, "Error", MB_OK);
		exit(EXIT_FAILURE);
	}

	memset(g_clProgramString, '\0', len+1);
	fread(g_clProgramString, sizeof(char), len, fp);
	status = ferror(fp);
	if(status != 0)
	{
		sprintf_s(buffer, 1024, "Error reading into the program string from file");
		MessageBoxA(NULL, buffer, "Error", MB_OK);
		exit(EXIT_FAILURE);
	}

	fclose(fp);
	
	return SUCCESS;
}

#pragma endregion 

#pragma region DX11 Sharing

int GetDX11SharingExtensions()
{
	int result = SUCCESS;

	ptrF_clCreateFromD3D11Texture2DKHR = (clCreateFromD3D11Texture2DKHR_fn)clGetExtensionFunctionAddressForPlatform(g_platformToUse, "clCreateFromD3D11Texture2DKHR");
	if (ptrF_clCreateFromD3D11Texture2DKHR == NULL)
	{
		result = FAIL;
	}

	ptrF_clEnqueueAcquireD3D11ObjectsKHR = (clEnqueueAcquireD3D11ObjectsKHR_fn)clGetExtensionFunctionAddressForPlatform(g_platformToUse, "clEnqueueAcquireD3D11ObjectsKHR");
	if (ptrF_clEnqueueAcquireD3D11ObjectsKHR == NULL)
	{
		result = FAIL;
	}

	ptrF_clEnqueueReleaseD3D11ObjectsKHR = (clEnqueueReleaseD3D11ObjectsKHR_fn)clGetExtensionFunctionAddressForPlatform(g_platformToUse, "clEnqueueReleaseD3D11ObjectsKHR");
	if (ptrF_clEnqueueReleaseD3D11ObjectsKHR == NULL)
	{
		result = FAIL;
	}

	return result;
}
int SetSharedResourceFlagByPerformance()
{
	int result = FAIL;
	cl_bool bResult;

	bResult = clGetContextInfo(g_clContext, CL_CONTEXT_D3D11_PREFER_SHARED_RESOURCES_KHR, sizeof(cl_bool), &bResult, NULL);
	if(bResult == CL_TRUE)
	{
		//Shared Flag recommend by OpenCL
		g_SharedResourceFlagPerformanceFlag = true;
		result = SUCCESS;
	}
	else
	{
		//Shared Flag NOT recommend by OpenCL
		g_SharedResourceFlagPerformanceFlag = false;
		result = SUCCESS;
	}

	return result;
}
int DX11SurfaceSharingPlatformAvailable()
{
	int result = 0;
	cl_uint numPlatforms = 0;

	char buffer[1024];
	memset(buffer, 0, 1024);

	result = clGetPlatformIDs(0, NULL, &numPlatforms);
	CheckResult(result, "clGetPlatformIDs error\n");
	
	cl_platform_id *platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id) * numPlatforms);
	if(platforms == NULL)
	{
		MessageBoxA(NULL, "Error when allocating space for the platforms", "OpenCL Platform Error", MB_OK);
		exit(EXIT_FAILURE);
	}

	result = clGetPlatformIDs(numPlatforms, platforms, NULL);
	CheckResult(result, "clGetPlatformIDs error");

	for(unsigned int i=0;i<numPlatforms;i++)
	{
		char platformVendor[100];
		memset(platformVendor, '\0',100);
		result = clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, sizeof(platformVendor), platformVendor, NULL);
		CheckResult(result, "clGetPlatformInfo error");
		
		char platformName[100];
		memset(platformName, '\0',100);
		result = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, sizeof(platformName), platformName, NULL);
		CheckResult(result, "clGetPlatformInfo error");

		char extension_string[1024];
		memset(extension_string, '\0', 1024);
		result = clGetPlatformInfo(platforms[i], CL_PLATFORM_EXTENSIONS, sizeof(extension_string), extension_string, NULL);
	
		char *extStringStart = NULL;
		extStringStart = strstr(extension_string, "cl_khr_d3d11_sharing");
		if(extStringStart != 0)
		{
			//This Platform supports the cl_khr_d3d11_sharing extension
			cl_uint num_devices = 0;

			result = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices);
			CheckResult(result, "Error getting number of devices\n");

			cl_device_id *clDevices = NULL;
			clDevices = (cl_device_id *)malloc(sizeof(cl_device_id)*num_devices);
			if(clDevices == NULL)
			{
				MessageBoxA(NULL, "Error when allocating", "OpenCL Platform Error", MB_OK);
			}

			result = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, num_devices, clDevices, 0);
			CheckResult(result, "clGetDeviceIDs error");

			for(cl_uint iDevNum=0;iDevNum<num_devices;iDevNum++)
			{
				//Query each for their extension string
				cl_device_type device_type;
				char vendorName[256];
				memset(vendorName, '\0', 256);
				char devExtString[1024];
				memset(devExtString, '\0', 1024);

				clGetDeviceInfo(clDevices[iDevNum], CL_DEVICE_TYPE, sizeof(cl_device_type), (void *)&device_type, NULL);
				clGetDeviceInfo(clDevices[iDevNum], CL_DEVICE_VENDOR, (sizeof(char)*256), &vendorName, NULL);
				clGetDeviceInfo(clDevices[iDevNum], CL_DEVICE_EXTENSIONS, (sizeof(char)*1024), &devExtString, NULL);

				char *extStringStart = NULL;
				extStringStart = strstr(devExtString, "cl_khr_d3d11_sharing");
		
				char devTypeString[256];
				memset(devTypeString, '\0', 256);

				if(extStringStart != 0)
				{
					//Device has support for the CL/DX11 sharing Extention
					if(device_type == CL_DEVICE_TYPE_GPU)
					{
						strcpy_s(devTypeString, "GPU");
					}
					else
					{
						//Other types will be added later
						strcpy_s(devTypeString, "Not GPU"); 
					}
					
				}
				else
				{
					//sprintf_s(buffer, 1024, "Device %s in %s platform does not support CL/DX11 sharing Extension", devTypeString, vendorName);
					//MessageBoxA(NULL, buffer, "OpenCL Platform Error", MB_OK);
				}
 			} 

			free(clDevices);
		}
		else
		{
			sprintf_s(buffer, 1024, "Platform %s does not support cl_khr_d3d11_sharing Extension", platformName);
			MessageBoxA(NULL, buffer, "OpenCL Platform Error", MB_OK);

		}
	}

	return result;
}
int ShareDX11BufferWithCL()
{
	int result = 0;

#pragma region SDF Render Kernel

	g_SharedRGBAimageCLMemObject = ptrF_clCreateFromD3D11Texture2DKHR(g_clContext, CL_MEM_WRITE_ONLY, g_pSharedDX11Texture2D, 0, &result);
	if (result == 0)
	{
		result = SUCCESS;
	}
	else
	{
		MessageBoxA(NULL, "Sharing failed on DX11 Texture2D", "Error OpenCL DX11 Sharing", MB_OK);
		result = FAIL;
	}

#pragma endregion

#pragma region Blur Kernel

	g_SharedRGBAimageCLMemObjectBlur = ptrF_clCreateFromD3D11Texture2DKHR(g_clContext, CL_MEM_WRITE_ONLY, g_pSharedDX11Texture2DBlur, 0, &result);
	if (result == 0)
	{
		result = SUCCESS;
	}
	else
	{
		MessageBoxA(NULL, "Sharing failed on DX11 Texture2D", "Error OpenCL DX11 Sharing", MB_OK);
		result = FAIL;
	}
	
#pragma endregion
	
	return result;
}

#pragma endregion

int StartupOpenCL()
{
	cl_int result = 0;

#pragma region Get the platform to use

	cl_uint numPlatforms= 0;
		
	g_platformToUse = NULL;
	
	result = clGetPlatformIDs(0, NULL, &numPlatforms);
	CheckResult(result, "clGetPlatformIDs error");
	
	cl_platform_id *platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id) * numPlatforms);
	if(platforms == NULL)
	{
		MessageBoxA(NULL, "No memory for the platforms array", "Startup Error OpenCL", MB_OK);
		exit(EXIT_FAILURE);
	}

	result = clGetPlatformIDs(numPlatforms, platforms, NULL);
	CheckResult(result, "clGetPlatformIDs error");

#pragma endregion

#pragma region Get the device ids for the platform

	cl_uint num_devices = -1; 
	cl_device_info devTypeToUse = CL_DEVICE_TYPE_GPU;

	for(unsigned int i=0;i<numPlatforms;i++)
	{
		char pbuf[256];
		result = clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, sizeof(pbuf), pbuf, NULL);
		CheckResult(result, "clGetPlatformInfo error");

		if(!strcmp(pbuf, "Advanced Micro Devices, Inc.")) 
		{
			//AMD OpenCL platform
			g_platformToUse = platforms[i];
			result = clGetDeviceIDs(g_platformToUse, devTypeToUse, 0, g_clDevices, &num_devices);
		
			if (result == CL_DEVICE_NOT_FOUND)
				continue; 
			else
				break;
		}
	}
		
	if(g_platformToUse == NULL)
	{
		MessageBoxA(NULL, "No AMD OpenCL implementation, exiting application", "Startup Error OpenCL", MB_OK);
		exit(EXIT_FAILURE);
	}

	//get # of devices of this type on this platform and allocate space in g_clDevices (better be 1 for this tutorial)
	CheckResult(result, "clGetDeviceIDs error");
	
	//allocate space
	g_clDevices = (cl_device_id *)malloc(sizeof(cl_device_id)*num_devices);
	if(g_clDevices == NULL)
	{
		MessageBoxA(NULL, "No memory for the devices array", "Startup Error OpenCL", MB_OK);
		exit(EXIT_FAILURE);
	}
	
#pragma endregion

#pragma region AMD platform, get the device we want to use
	
	result = clGetDeviceIDs(g_platformToUse, devTypeToUse, num_devices, g_clDevices, 0);
	CheckResult(result, "clGetDeviceIDs error");

	cl_device_type device_type;
    char vendorName[256];
	memset(vendorName, '\0', 256);

	clGetDeviceInfo(g_clDevices[0], CL_DEVICE_TYPE, sizeof(cl_device_type), (void *)&device_type, NULL);

	clGetDeviceInfo(g_clDevices[0], CL_DEVICE_VENDOR, (sizeof(char)*256), vendorName, NULL);
	
	if(device_type != CL_DEVICE_TYPE_GPU)
	{
		MessageBoxA(NULL, "No GPU Found, this version only supports AMD's GPU", "Startup Error OpenCL", MB_OK);
		exit(EXIT_FAILURE);
	}

#pragma endregion

#pragma region Context and Extensions

	cl_context_properties cps[] = 
	{ 
		CL_CONTEXT_PLATFORM, (cl_context_properties)g_platformToUse, 
		CL_CONTEXT_D3D11_DEVICE_KHR, (intptr_t)g_pd3dDevice,
		CL_CONTEXT_INTEROP_USER_SYNC, CL_FALSE,
		0 
	};

	clGetDeviceIDsFromD3D11KHR_fn ptrToFunction_clGetDeviceIDsFromD3D11KHR = NULL;
	ptrToFunction_clGetDeviceIDsFromD3D11KHR = (clGetDeviceIDsFromD3D11KHR_fn) clGetExtensionFunctionAddressForPlatform(g_platformToUse, "clGetDeviceIDsFromD3D11KHR");
	cl_uint numDevs = 0;
	//size_t bytes = 0;
	//careful with the g_pd3DDevice
	result = ptrToFunction_clGetDeviceIDsFromD3D11KHR(g_platformToUse, CL_D3D11_DEVICE_KHR, (void *)g_pd3dDevice, CL_PREFERRED_DEVICES_FOR_D3D11_KHR, 0, NULL, &numDevs);
	CheckResult(result, "Failed on clGetDeviceIDsFromD3D11KHR");

	cl_device_id *devID = NULL;
	g_clDevices = (cl_device_id *)malloc(sizeof(cl_device_id) * numDevs);
	ptrToFunction_clGetDeviceIDsFromD3D11KHR(g_platformToUse, CL_D3D11_DEVICE_KHR, (void *)g_pd3dDevice, CL_PREFERRED_DEVICES_FOR_D3D11_KHR, numDevs, g_clDevices, NULL);
	CheckResult(result, "Failed on clGetDeviceIDsFromD3D11KHR");

	//create an OCL context from the device we are using as our DX11 rendering device
	g_clContext = clCreateContext(cps, 1, g_clDevices, NULL, NULL, &result);
	CheckResult(result, "clCreateContext error");

	//create an openCL commandqueue
	g_clCommandQueue = clCreateCommandQueueWithProperties(g_clContext, g_clDevices[0], 0, &result);
	CheckResult(result, "clCreateCommandQueue error");
	
	//create device side program, compile and create program objects
	result = BuildKernels();
	CheckResult(result, "error in initDevice()");
	
	if(numPlatforms > 0)
	{
		free(platforms);
	}

	if(g_clDevices != NULL)
	{
		free(g_clDevices);
	}

#pragma endregion

#pragma region Upload the Blur Filter to the GPU
	
	float blur9[9 * 9] = {
		0.0f, 0.000001f, 0.000014f, 0.000055f, 0.000088f, 0.000055f, 0.000014f, 0.000001f, 0.0f,
		0.000001f, 0.000036f, 0.000362f, 0.001445f, 0.002289f, 0.001445f, 0.000362f, 0.000036f, 0.000001f,
		0.000014f, 0.000362f, 0.003672f, 0.014648f, 0.023205f, 0.014648f, 0.003672f, 0.000362f, 0.000014f,
		0.000055f, 0.001445f, 0.014648f, 0.058434f, 0.092566f, 0.058434f, 0.014648f, 0.001445f, 0.000055f,
		0.000088f, 0.002289f, 0.023205f, 0.092566f, 0.146634f, 0.092566f, 0.023205f, 0.002289f, 0.000088f,
		0.000055f, 0.001445f, 0.014648f, 0.058434f, 0.092566f, 0.058434f, 0.014648f, 0.001445f, 0.000055f,
		0.000014f, 0.000362f, 0.003672f, 0.014648f, 0.023205f, 0.014648f, 0.003672f, 0.000362f, 0.000014f,
		0.000001f, 0.000036f, 0.000362f, 0.001445f, 0.002289f, 0.001445f, 0.000362f, 0.000036f, 0.000001f,
		0.0f, 0.000001f, 0.000014f, 0.000055f, 0.000088f, 0.000055f, 0.000014f, 0.000001f, 0.0f
	};

	float *filter = blur9;
	memman = new memmanOCL(g_clContext);

	int zero = 0;
	filterBuffer = memman->Alloc(9 * 9, sizeof(CL_FLOAT), CL_MEM_READ_ONLY);
	int err = clEnqueueWriteBuffer(g_clCommandQueue, filterBuffer->block, CL_TRUE, 0, 9 * 9 * sizeof(float), filter, 0, NULL, NULL);

#pragma endregion

	return SUCCESS;
}

int BuildKernels()
{
	cl_int result;

#pragma region SDF Render Kernel

	//load CL file, build CL program object, create CL kernel object
	const char *filename = CL_KERNELS;
	result = LoadKernelFromFile(filename);

	size_t sourceSize = strlen(g_clProgramString);

	g_clProgram = clCreateProgramWithSource(g_clContext, 1, (const char **)&g_clProgramString, &sourceSize, &result);
	CheckResult(result, "clCreateProgramWithSource error");

	result = clBuildProgram(g_clProgram, 1, g_clDevices, NULL, NULL, NULL);
	if (result != CL_SUCCESS)
	{
		if (result == CL_BUILD_PROGRAM_FAILURE)
		{
			CheckCompileResult();
		}
	}

	cl_kernel_RenderSDF = clCreateKernel(g_clProgram, "RenderSDF", &result);
	CheckResult(result, "clCreateKernel error");

#pragma endregion

#pragma region Blur Kernel

	filename = CL_KERNEL_BLUR;

	result = LoadKernelFromFile(filename);

	sourceSize = strlen(g_clProgramString);

	g_clProgram = clCreateProgramWithSource(g_clContext, 1, (const char **)&g_clProgramString, &sourceSize, &result);
	CheckResult(result, "clCreateProgramWithSource error");

	result = clBuildProgram(g_clProgram, 1, g_clDevices, NULL, NULL, NULL);
	if (result != CL_SUCCESS)
	{
		if (result == CL_BUILD_PROGRAM_FAILURE)
		{
			CheckCompileResult();
		}
	}

	cl_kernel_blur = clCreateKernel(g_clProgram, "Blur", &result);
	CheckResult(result, "clCreateKernel error");

#pragma endregion

	return SUCCESS;

}

void RunKernelsSDF(double seconds)
{
	cl_int status;
	int imageWidth = SHARED_IMAGE_WIDTH;
	int imageHeight = SHARED_IMAGE_HEIGHT;
	int imageWidthBy2 = imageWidth / 2;
	int imageHeightBy2 = imageHeight / 2;

	float aspectRatio = (float)imageHeight / (float)imageWidth;

	float uInc = 2.0f / (imageWidth - 1.0f);
	float vInc = 2.0f / (imageHeight - 1.0f) * aspectRatio;

	float viewAngle = 90.0f * 0.017453293f;
	float DVal = cos(viewAngle / 2.0f) / sin(viewAngle / 2.0f);

	vector3 from, at, up, A1, A2, A3;
	cl_float3 fromV, rightV, centerV, upV;

	from.x = view_point_x;
	from.y = view_point_y;
	from.z = view_point_z;

	vector3 v;
	v.x = 0.0f;
	v.y = -5.0f;
	v.z = 0.0f;

	RotateX(v, view_angle_x * PI_180ED);
	RotateY(v, view_angle_y * PI_180ED);
	RotateZ(v, view_angle_z * PI_180ED);

	at.x = view_point_x + v.x;
	at.y = view_point_y + v.y;
	at.z = view_point_z + v.z;

	up.x = 0.0f;
	up.y = -1.0f;
	up.z = 0.0f;

	A3 = subtract(&at, &from);
	A1 = cross(&A3, &up);
	A2 = cross(&A1, &A3);

	normalize(&A1);
	normalize(&A2);
	normalize(&A3);

	fromV.x = from.x;
	fromV.y = from.y;
	fromV.z = from.z;

	rightV.x = A1.x;
	rightV.y = A1.y;
	rightV.z = A1.z;

	upV.x = A2.x;
	upV.y = A2.y;
	upV.z = A2.z;

	centerV.x = A3.x * DVal;
	centerV.y = A3.y * DVal;
	centerV.z = A3.z * DVal;

	//see page 107 of rev21 of the spec, clearly states this won't return until all D3D11 has completed
	//status = ptrF_clEnqueueAcquireD3D11ObjectsKHR(g_clCommandQueue, 1, &g_SharedRGBAimageCLMemObjectBlur, 0, 0, 0);
	status = ptrF_clEnqueueAcquireD3D11ObjectsKHR(g_clCommandQueue, 1, &g_SharedRGBAimageCLMemObject, 0, 0, 0);

	status = clSetKernelArg(cl_kernel_RenderSDF, 0, sizeof(cl_mem), &g_SharedRGBAimageCLMemObject);
	CheckResult(status, "clSetKernelArg");

	status = clSetKernelArg(cl_kernel_RenderSDF, 1, sizeof(cl_float), &uInc);
	CheckResult(status, "clSetKernelArg");

	status = clSetKernelArg(cl_kernel_RenderSDF, 2, sizeof(cl_float), &vInc);
	CheckResult(status, "clSetKernelArg");

	status = clSetKernelArg(cl_kernel_RenderSDF, 3, sizeof(cl_float), &imageWidthBy2);
	CheckResult(status, "clSetKernelArg");

	status = clSetKernelArg(cl_kernel_RenderSDF, 4, sizeof(cl_float), &imageHeightBy2);
	CheckResult(status, "clSetKernelArg");

	status = clSetKernelArg(cl_kernel_RenderSDF, 5, sizeof(cl_float3), &centerV);
	CheckResult(status, "clSetKernelArg");

	status = clSetKernelArg(cl_kernel_RenderSDF, 6, sizeof(cl_float3), &rightV);
	CheckResult(status, "clSetKernelArg");

	status = clSetKernelArg(cl_kernel_RenderSDF, 7, sizeof(cl_float3), &upV);
	CheckResult(status, "clSetKernelArg");

	status = clSetKernelArg(cl_kernel_RenderSDF, 8, sizeof(cl_float3), &fromV);
	CheckResult(status, "clSetKernelArg");

	float iTime = (float)seconds;
	status = clSetKernelArg(cl_kernel_RenderSDF, 9, sizeof(cl_float), &iTime);
	CheckResult(status, "clSetKernelArg");


	size_t global_dim[2];
	global_dim[0] = SHARED_IMAGE_WIDTH;
	global_dim[1] = SHARED_IMAGE_HEIGHT;

	status = clEnqueueNDRangeKernel(g_clCommandQueue, cl_kernel_RenderSDF, 2, NULL, global_dim, NULL, 0, NULL, NULL);
	CheckResult(status, "clEnqueueNDRangeKernel fail");

	////Blur Pass
	//clSetKernelArg(cl_kernel_blur, 0, sizeof(cl_mem), &g_SharedRGBAimageCLMemObjectBlur);
	//clSetKernelArg(cl_kernel_blur, 1, sizeof(cl_mem), &g_SharedRGBAimageCLMemObject);
	//clSetKernelArg(cl_kernel_blur, 2, sizeof(cl_mem), &filterBuffer->block);
	//clSetKernelArg(cl_kernel_blur, 3, sizeof(cl_int), &filterWidth);

	//status = clEnqueueNDRangeKernel(g_clCommandQueue, cl_kernel_blur, 2, NULL, global_dim, NULL, 0, NULL, NULL);
	//CheckResult(status, "clEnqueueNDRangeKernel fail");


	status = ptrF_clEnqueueReleaseD3D11ObjectsKHR(g_clCommandQueue, 1, &g_SharedRGBAimageCLMemObject, 0, NULL, NULL);
	CheckResult(status, "Fail on clEnqueueReleaseD3D11ObjectsKHR");

	//status = ptrF_clEnqueueReleaseD3D11ObjectsKHR(g_clCommandQueue, 1, &g_SharedRGBAimageCLMemObjectBlur, 0, NULL, NULL);
	//CheckResult(status, "Fail on clEnqueueReleaseD3D11ObjectsKHR");

	clFinish(g_clCommandQueue);

}

int ShutdownOpenCL()
{
	cl_int status;

	status = clReleaseMemObject(g_SharedRGBAimageCLMemObject);
	CheckResult(status, "Error releasing mem object");

	status = clReleaseMemObject(g_SharedRGBAimageCLMemObjectBlur);
	CheckResult(status, "Error releasing mem object");

	status = clReleaseKernel(cl_kernel_RenderSDF);
	CheckResult(status, "Error releasing kernel");

	status = clReleaseKernel(cl_kernel_blur);
	CheckResult(status, "Error releasing kernel");

	status = clReleaseProgram(g_clProgram);
	CheckResult(status, "Error releasing program");

	status = clReleaseCommandQueue(g_clCommandQueue);
	CheckResult(status, "Error releasing command queue");

	status = clReleaseContext(g_clContext);
	CheckResult(status, "Error releasing context");

	return status;
}
