#define PI 3.14159265359

struct SVOElement
{
    uint childPointer;
    uint masks;
};

cbuffer CameraInfo : register(b0)
{
    float3 CamPos;
    float Time;
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
    tMax = min(tMax, tzMax);

    norm.z = tMax; //DEBUG
    return float4(norm, tMin);
}

uint getValidMask(uint mask, uint index)
{
    uint x = 1 << (24 + index);
    return mask & x;
}

uint getChildIndex(float3 boxPos, float3 pos)
{
    if (pos.x > boxPos.x && pos.y < boxPos.y && pos.z > boxPos.z) return 0;
    if (pos.x < boxPos.x && pos.y < boxPos.y && pos.z > boxPos.z) return 1;
    if (pos.x < boxPos.x && pos.y < boxPos.y && pos.z < boxPos.z) return 2;
    if (pos.x > boxPos.x && pos.y < boxPos.y && pos.z < boxPos.z) return 3;

    if (pos.x > boxPos.x && pos.y > boxPos.y && pos.z > boxPos.z) return 4;
    if (pos.x < boxPos.x && pos.y > boxPos.y && pos.z > boxPos.z) return 5;
    if (pos.x < boxPos.x && pos.y > boxPos.y && pos.z < boxPos.z) return 6;
    if (pos.x > boxPos.x && pos.y > boxPos.y && pos.z < boxPos.z) return 7;

    return 9;
}

float4 getChildBox(float3 rootPos, float rootScale, uint index)
{
    float newScale = rootScale * 0.5;

    if (index == 0) return float4(rootPos + float3(newScale, -newScale, newScale), newScale);
    if (index == 1) return float4(rootPos + float3(-newScale, -newScale, newScale), newScale);
    if (index == 2) return float4(rootPos + float3(-newScale, -newScale, -newScale), newScale);
    if (index == 3) return float4(rootPos + float3(newScale, -newScale, -newScale), newScale);

    if (index == 4) return float4(rootPos + float3(newScale, newScale, newScale), newScale);
    if (index == 5) return float4(rootPos + float3(-newScale, newScale, newScale), newScale);
    if (index == 6) return float4(rootPos + float3(-newScale, newScale, -newScale), newScale);
    if (index == 7) return float4(rootPos + float3(newScale, newScale, -newScale), newScale);

    return float4(0, 0, 0, 0);
}

float3 getNormal(float3 boxPos, float3 pos)
{
    float3 v = pos - boxPos;
    float3 vAbs = abs(v);
    
    if (vAbs.x >= vAbs.y && vAbs.x >= vAbs.z)
    {
        return sign(v.x) * float3(1, 0, 0);
    }

    if (vAbs.y > vAbs.x && vAbs.y >= vAbs.z)
    {
        return sign(v.y) * float3(0, 1, 0);
    }

    return sign(v.z) * float3(0, 0, 1);
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
	
    float3 dirLight = float3(0, 1, -1);
    float3 movingLight = float3(3*sin(Time), 1, -1);

    float4 lightRet = raytraceBox(movingLight, 0.1, cameraPos, dir);
    if (lightRet.a >= 0.0) return float4(1, 1, 1, 1);

    float3 rootPos = float3(0, 0, 0);
    float rootScale = 0.5;
    float4 ret = raytraceBox(rootPos, rootScale, cameraPos, dir);

    if (ret.a < 0.0) return float4(0, 0, 0.3, 1);

    float3 pos = cameraPos + (ret.a * dir);
    float3 posMax = cameraPos + (ret.z * dir);

    uint childHitIndex = getChildIndex(float3(0, 0, 0), pos);
    uint childHitMaxIndex = getChildIndex(float3(0, 0, 0), posMax);

    if (getValidMask(Elements[0].masks, childHitIndex) > 0)
    {
        float4 child = getChildBox(rootPos, rootScale, childHitIndex);

        float3 voxNorm = getNormal(child.xyz, pos);
        float3 voxColor = float3(1, 0, 0) * clamp(dot(voxNorm, normalize(dirLight)), 0, 1);
        voxColor += float3(1, 0, 0) * (clamp(dot(voxNorm, normalize(movingLight - pos)), 0, 1) / length(movingLight - pos)) * 1.5;

        return float4(voxColor, 1);
    }
    else
    {
        float4 child = getChildBox(rootPos, rootScale, childHitIndex);
        float3 childPosMax = float3(0, 0, 0);

        bool didNotHit = true;
        while (didNotHit)
        {
            float4 retChild = raytraceBox(child.xyz, child.w, cameraPos, dir);
            childPosMax = cameraPos + (retChild.z * dir * 1.000025);

            childHitIndex = getChildIndex(rootPos, childPosMax);
            child = getChildBox(rootPos, rootScale, childHitIndex);
            if (getValidMask(Elements[0].masks, childHitIndex) > 0) didNotHit = false;
            else if (retChild.z >= ret.z) break;

        }
        if (!didNotHit)
        {
            float3 voxNorm = getNormal(child.xyz, childPosMax);
            float3 voxColor = float3(0, 0.3, 0) * clamp(dot(voxNorm, normalize(dirLight)), 0, 1);
            voxColor += float3(0, 0.3, 0) * (clamp(dot(voxNorm, normalize(movingLight - pos)), 0, 1) / length(movingLight - pos)) * 1.5;

            return float4(voxColor, 1);
        }
    }

    return float4(0.3, 0.0, 0.3, 1);

}