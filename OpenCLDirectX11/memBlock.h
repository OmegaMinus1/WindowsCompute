#pragma once

#include <CL/cl.h>

class memBlock
{
public:
	size_t size = 0;
	size_t depth = 0;
	cl_int flags;
	cl_mem block;

	memBlock(int AllocSize, int AllocDepth, cl_context inContext, cl_int inFlags)
	{
		cl_int status;
		size = AllocSize;
		depth = AllocDepth;
		flags = inFlags;
		block = clCreateBuffer(inContext, inFlags, AllocSize * AllocDepth, NULL, &status);
	}

	virtual ~memBlock()
	{
		clReleaseMemObject(block);
		size = 0;
	}

};

