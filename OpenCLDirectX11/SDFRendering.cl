float3 Rotate3X(float3 v, float a)
{
	float3 temp;
	temp.x = v.x;
	temp.y = (v.y * cos(a)) + (v.z * -sin(a));
	temp.z = (v.y * sin(a)) + (v.z * cos(a));
	return temp;
}

float3 Rotate3Y(float3 v, float a)
{
	float3 temp;
	temp.x = (v.x * cos(a)) + (v.z * sin(a));
	temp.y = v.y;
	temp.z = (v.x * -sin(a)) + (v.z * cos(a));
	return temp;
}

float3 Rotate3Z(float3 v, float a)
{
	float3 temp;
	temp.x = (v.x * cos(a)) + (v.y * -sin(a));
	temp.y = (v.x * sin(a)) + (v.y * cos(a));
	temp.z = v.z;
	return temp;
}

float plane(float3 pos)
{
	return pos.y;
}

float sphere(float3 pos, float radius)
{
	return length(pos) - radius;
}

float box(float3 pos, float3 size)
{
	return length(max(fabs(pos) - size, 0.0f));
}

float roundedBox(float3 pos, float3 size, float radius)
{
	return length(max(fabs(pos) - size, 0.0f)) - radius;
}

float map(float3 pos, float iTime)
{
	float planeDistance = plane(pos);
	
	pos.xy = Rotate3Z(pos, 100.0f * sin(iTime) * 0.01f).xy;
	
	pos = fabs(pos);
	
	pos = fmod(pos + 10.0f, 20.0f) - 10.0f;
	
	pos.xy = Rotate3Z(pos, iTime).xy;
	pos.xz = Rotate3Y(pos, iTime * 0.7f).xz;
	
	////if(iMouse.z > 0.0f)
    // 		return min(planeDistance, roundedBox(pos, (float3)(1.0f), 1.0f));
	////else
	
	return min(planeDistance, sphere(pos, 3.0f));
}

float3 computeNormal(float3 pos, float iTime)
{
	float2 eps = (float2)(0.01f, 0.0f);
	return normalize((float3)(
	map(pos + eps.xyy, iTime) - map(pos - eps.xyy, iTime), 
	map(pos + eps.yxy, iTime) - map(pos - eps.yxy, iTime), 
	map(pos + eps.yyx, iTime) - map(pos - eps.yyx, iTime)));
	
}

float diffuse(float3 normal, float3 lightDirection)
{
	// return max(dot(normal, lightDirection), 0.0f);
	// wrap lighting
	return dot(normal, lightDirection) * 0.5f + 0.5f;
}

float specular(float3 normal, float3 dirToLight, float3 dir)
{
	// IBL
	//float3 h = normalize(dir);
	//return pow(max(dot(h, normal), 0.0f), 100.0f);

	float sceneLightScale = 18.0f;

	float lightDist = length(dirToLight) / sceneLightScale;
	float diffuseScale = sqrt(lightDist);

	float3 LL = normalize(dirToLight);
	
	float NdotL = dot(LL, normal);

	//float diffuse = max(0.5f * NdotL + 0.5f, 0.0f);
	float diffuse = 0.5f * NdotL + 0.5f;
	
	//Calc Specular Phong
	float cosAlpha = dot((float3)(LL.x + 2.0f * NdotL * normal.x,
							       LL.y + 2.0f * NdotL * normal.y,
							       LL.z + 2.0f * NdotL * normal.z), dir);
	
	float spec = max(0.5f * pow(cosAlpha, 4.0f) / lightDist, 0.0f);
    
	return (diffuse + spec);
}

float3 material(float3 pos, float iTime)
{
   float i;
   return (float3)(smoothstep(0.4f, 0.41f,	fract(pos.x + sin(pos.z * 0.4f + iTime), &i)));
}

kernel void RenderSDF(__write_only image2d_t output, const float uInc, const float vInc, const int imageWidthBy2, const int imageHeightBy2, const float3 centerV, const float3 rightV, const float3 upV, const float3 from, const float iTime)
{
    const int gidX = get_global_id(0);
	const int gidY = get_global_id(1);

	//Calculate ray
	const float u = (gidX - imageWidthBy2) * uInc;
	const float v = (imageHeightBy2 - gidY) * vInc;

	//Calc Ray Dir
	float3 dir; 
	dir.x = centerV.x + u * rightV.x + v * upV.x;
	dir.y = centerV.y + u * rightV.y + v * upV.y;
	dir.z = centerV.z + u * rightV.z + v * upV.z;

	int2 coords = (int2)(gidX, gidY);
	
	float3 nDir = normalize(dir);
		
	//float3 pos = (float3)(sin(iTime * 0.2) * 4.0, 5.0 + sin(iTime * 0.4) * 3.0, -13.0);
	float3 pos = from;
	//float3 dir = normalize(vec3(uv, 1.0));
	
	// Ray March 
	float posDistance = 0.0f;
	for(int i = 0;i < 64;i++)
	{
		float d = map(pos, iTime);
		posDistance += d;
		pos += d * nDir;
	}
	
	float3 normal = computeNormal(pos, iTime);

	//float3 lightPos = (float3)(0.0f, 100.0f, -100.0f);
	float3 lightPos = (float3)(sin(iTime * 0.2f) * 100.0f, 100.0f + sin(iTime * 0.4f) * 10.0f, -100.0f);
	float3 dirToLight = normalize(lightPos - pos);
	float3 posToLight = pos + (0.0001f * dirToLight);
	
	float fShadowBias = 0.05f;
	float fStartDistance = fShadowBias / fabs(dot(dirToLight, normal));
	float fLightDistance = 100.0f;
	float fLength = fLightDistance - fStartDistance;
	
	float posToLightDistance = 0.0f;
	for(int i = 0;i < 64;i++)
	{
		float d = map(posToLight, iTime);
		posToLightDistance += d;
		posToLight += d * dirToLight;
	}
	
	float fShadow = (step(0.0f, posToLightDistance) * step(fLightDistance, posToLightDistance));		
	
	float fAmbientOcclusion = 1.0f;
	
	float fDist = 0.0f;
	float3 posForAO = pos + (0.1f * normal);
	for(int i = 0;i <= 5;i++)
	{
		fDist += 0.1f;
		float d = map(posForAO + (normal * fDist), iTime);
		
		//fDist runs 0.1 0.2 0.3 0.4 0.5 always 

		//if(fDist - d < 0.1f)
			fAmbientOcclusion *= 1.0f - max(0.0f, (fDist - d) * 0.2f / fDist);           
		         
		
	}

	//if(fAmbientOcclusion < 0.5f)
	//	fAmbientOcclusion = max(0.0f, fAmbientOcclusion - 0.5f);

	float fogFactor = exp(-posDistance * 0.01f);
	float4 fogColor = (float4)(0.1f, 0.2f, 0.81f, fogFactor);
	
	float diffSpec = specular(normal, lightPos - pos, nDir);
	float3 color3 = min(diffSpec * (float3)(0.0f, 0.2f, 0.81f), 1.0f) * material(pos, iTime);

	float4 color;
	color.x = color3.x;
	color.y = color3.y;
	color.z = color3.z;
	color.w = 1.0f;

	float4 colorQtr;
	colorQtr.x = color.x * 0.25f;
	colorQtr.y = color.y * 0.25f;
	colorQtr.z = color.z * 0.25f;
	colorQtr.w = 1.0f;

	color = mix(color, colorQtr, 1.0f - (fShadow * fAmbientOcclusion));
	color = mix(color, fogColor, 1.0f - fogColor.w);

	write_imagef(output, coords, color);
	//write_imagef(output, coords, (float4)(fAmbientOcclusion));
	
}