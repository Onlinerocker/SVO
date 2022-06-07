struct SVOElement
{
    uint childPointer;
    uint masks;
};

cbuffer CameraInfo : register(b0)
{
    float3 CamPos;
};

cbuffer SVOInfo : register(b1)
{
    SVOElement Elements[64];
};

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
        return float4(bg.rgb, -1);
	
    if (tyMin > tMin)
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
        return float4(bg.rgb, -1);
		
    if (tzMin > tMin)
    {
        tMin = tzMin;
        norm = zNorm;
    }

    return float4(norm, tMin);
}

uint getValidMask(uint mask, uint index)
{
    uint x = 1 << (24 + index);
    return mask & x;
}

float4 pixelMain(float4 position : SV_POSITION) : SV_TARGET
{
    float x = position.x - 640.0;
    x /= 640.0;

    float y = position.y - 360.0;
    y /= -360.0;

    x *= (1280.0 / 720.0);

    float3 cameraPos = CamPos; //float3(-1, 0.7, 0);
    float3 dir = float3(x, y, 1);
    dir = normalize(dir);

    float3 col = float3(0.3, 0.3, 0.3);
    float tMin = 0;
	
	//1098 per person, 2536 first month
	//OUC electric
	//Hotwire internet
	//Renters insurance
	
    if (getValidMask(Elements[0].masks, 0) > 0)
    {
        float4 retMin = raytraceBox(float3(0.25, 0, 0.25), 0.25, cameraPos, dir);
        
        if (retMin.a > 0.0) 
            col = retMin.rgb;
		
        tMin = retMin.a;
    }
	
    if (getValidMask(Elements[0].masks, 1) > 0)
    {
        float4 retMin = raytraceBox(float3(-0.25, 0, 0.25), 0.25, cameraPos, dir);
        if (retMin.a > 0.0 && (tMin < 0 || retMin.a < tMin))
        {
            col = retMin.rgb;
            tMin = retMin.a;
        }
    }
     
    if (getValidMask(Elements[0].masks, 2) > 0)
    {
        float4 retMin = raytraceBox(float3(-0.25, 0, -0.25), 0.25, cameraPos, dir);
        if (retMin.a > 0.0 && (tMin < 0 || retMin.a < tMin))
        {
            col = retMin.rgb;
            tMin = retMin.a;
        }
    }
	
    if (getValidMask(Elements[0].masks, 3) > 0)
    {
        float4 retMin = raytraceBox(float3(0.25, 0, -0.25), 0.25, cameraPos, dir);
        if (retMin.a > 0.0 && (tMin < 0 || retMin.a < tMin))
        {
            col = retMin.rgb;
            tMin = retMin.a;
        }
    }
	
	
    return float4(col, 1);

}