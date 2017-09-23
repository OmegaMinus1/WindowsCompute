#pragma once

#include "memBlock.h"
#include <vector>

using namespace std;

class memmanOCL
{
public:

	memmanOCL(cl_context inContext)
	{
		context = inContext;
	}

	virtual ~memmanOCL()
	{
		context = NULL;

		for each (memBlock *mb in blocks)
		{
			mb->~memBlock();
		}

	}

	cl_context context;
	vector<memBlock*> blocks;

	memBlock* Alloc(int size, int depth, cl_int flags);
};

memBlock* memmanOCL::Alloc(int size, int depth, cl_int flags)
{
	memBlock *mb = new memBlock(size, depth, context, flags);
	
	blocks.push_back(mb);
	
	return mb;
}