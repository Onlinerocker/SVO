cbuffer CameraInfo
{
	float3 CamPos;
};

float4 raymarchSphere(float3 cameraPos, float3 rayDirection)
{
	float3 curPos = cameraPos;

	float4 diffuse = float4(194.0 / 255.0, 249.0 / 255.0, 224.0 / 255.0, 1);
	float3 spherePos = float3(0, 0, 2);
	float sphereRad = 1;
	
	for (int i = 0; i < 20; ++i)
	{
		float d = length(spherePos - curPos) - sphereRad;

		if (abs(d) < 0.01f)
		{
			float l = dot(normalize(curPos - spherePos), normalize(float3(1.5, 1, -2)));
			float l1 = dot(normalize(curPos - spherePos), normalize(float3(-1, 0, 0)));
			float l2 = dot(normalize(curPos - spherePos), normalize(float3(0, -1, 0)));
			
			float4 color = diffuse * l;
			color += diffuse * l1 * 0.5;
			color += diffuse * l2 * 0.25;

			return color;
		}
		else curPos += (rayDirection * d);
	}

	return float4(0.3, 0.3, 0.3, 1);
}

float calculateT(float plane, float origin, float direction)
{
	return (plane - origin) / direction;
}

//https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
float4 raytraceBox(float3 boxPos, float boxRad, float3 cameraPos, float3 rayDirection)
{
	float4 bg = float4(0.3, 0.3, 0.3, 1);
	float4 diffuse = float4(194.0 / 255.0, 249.0 / 255.0, 224.0 / 255.0, 1);
	
	float3 xNorm = float3(1, 0, 0);
	float3 yNorm = float3(0, 1, 0);
	float3 zNorm = float3(0, 0, 1);
	float3 norm = xNorm;
	
	float3 boxMin = boxPos - float3(boxRad, boxRad, boxRad);
	float3 boxMax = boxPos + float3(boxRad, boxRad, boxRad);
	
	float tMin = calculateT(boxMin.x, cameraPos.x, rayDirection.x);
	float tMax = calculateT(boxMax.x, cameraPos.x, rayDirection.x);
	
	if (tMin > tMax)
	{
		float temp = tMin;
		tMin = tMax;
		tMax = temp;
	}
	
	float tyMin = calculateT(boxMin.y, cameraPos.y, rayDirection.y);
	float tyMax = calculateT(boxMax.y, cameraPos.y, rayDirection.y);
	
	if (tyMin > tyMax)
	{
		float temp = tyMin;
		tyMin = tyMax;
		tyMax = temp;
	}
	
	if ((tMin > tyMax) || (tMax < tyMin))
		return bg;
	
	if(tyMin > tMin)
	{
		tMin = tyMin;
		norm = yNorm;
	}
	tMax = min(tMax, tyMax);
	
	float tzMin = calculateT(boxMin.z, cameraPos.z, rayDirection.z);
	float tzMax = calculateT(boxMax.z, cameraPos.z, rayDirection.z);
	
	if (tzMin > tzMax)
	{
		float temp = tzMin;
		tzMin = tzMax;
		tzMax = temp;
	}
	
	if ((tMin > tzMax) || (tMax < tzMin))
		return bg;
		
	if (tzMin > tMin)
	{
		tMin = tzMin;
		norm = zNorm;
	}

	return float4(norm, 1);
}

float4 pixelMain(float4 position : SV_POSITION) : SV_TARGET
{
	float x = position.x - 640.0;
	x /= 640.0;

	float y = position.y - 360.0;
	y /= -360.0;

	x *= (1280.0/720.0);

	float3 cameraPos = CamPos;//float3(-1, 0.7, 0);
	float3 dir = float3(x, y, 1);
	dir = normalize(dir);

	//return raymarchSphere(cameraPos, dir);
	return raytraceBox(float3(0, 0, 2), 0.5, cameraPos, dir);

}