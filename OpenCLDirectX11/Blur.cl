//Blur
__kernel void Blur(__read_only image2d_t inImage, __write_only image2d_t outImage, __constant float *filter,  int filterWidth)
{
    sampler_t samplerBlur = CLK_NORMALIZED_COORDS_FALSE | CLK_FILTER_LINEAR | CLK_ADDRESS_CLAMP_TO_EDGE;

	int column = get_global_id(0);
	int row = get_global_id(1);

	int halfWidth = (int)(filterWidth / 2);

	float4 sum = { 0.0f, 0.0f, 0.0f, 0.0f };

	int filterIdx = 0;

	int2 coords;
	
	for(int i = -halfWidth; i <= halfWidth; i++)
	{
		coords.y = row + i;

		for(int j = -halfWidth; j <= halfWidth; j++)
		{
			coords.x = column + j;
			
			float4 pixel;
			pixel = read_imagef(inImage, samplerBlur, coords);	
			
			sum += pixel * filter[filterIdx++];
		}
	}

	coords.x = column;
	coords.y = row;
	
	//write_imagef(outImage, coords, (float4)(1.0f));
	write_imagef(outImage, coords, sum);
}