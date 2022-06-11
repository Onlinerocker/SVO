#define PI 3.14159265359

struct SVOElement
{
    uint childPointer;
    uint masks;
};

struct ParentElement
{
    float3 pos;
    float scale;
    float exit;
    uint index;
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
    //if (plane - origin == 0 || direction == 0) return 0;
    return (plane - origin) / direction;
}

//https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
float2 raytraceBox(float3 boxPos, float boxRad, float3 cameraPos, float3 rayDirection)
{
    float4 bg = float4(0.3, 0.3, 0.3, 1);
    float4 diffuse = float4(194.0 / 255.0, 249.0 / 255.0, 224.0 / 255.0, 1);
	
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
        return float2(-1, -1);
	
    if (tyMin > tMin)
    {
        tMin = tyMin;
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
        return float2(-1, -1);
		
    if (tzMin > tMin)
    {
        tMin = tzMin;
    }
    tMax = min(tMax, tzMax);

    return float2(tMin, tMax);
}

uint getValidMask(uint mask, uint index)
{
    uint x = 1 << (24 + index);
    return mask & x;
}

uint getLeafMask(uint mask, uint index)
{
    uint x = 1 << (16 + index);
    return mask & x;
}

/* Could use extra space in SVO to store this information */
uint getChildPointer(SVOElement parentElement, uint index)
{
    uint curPos = 0;
    for (uint i = 0; i < index; ++i)
    {
        if (getValidMask(parentElement.masks, i) > 0 && !getLeafMask(parentElement.masks, i) > 0) ++curPos;
    }
    return parentElement.childPointer + curPos;
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

    ParentElement stack[64];
    uint stackIndex = 0;

    float3 cameraPos = CamPos; //float3(-1, 0.7, 0);
    float3 dir = float3(x, y, 1);
    dir = normalize(dir);

    float3 col = float3(0.3, 0.3, 0.3);
    float tMin = 0;
	
    float3 dirLight = float3(0, 1, -1);
    float3 movingLight = float3(200*sin(Time), 150, -0);

    float2 lightRet = raytraceBox(movingLight, 10, cameraPos, dir);
    if (lightRet.x >= 0.0) return float4(1, 1, 1, 1);

    float3 rootPos = float3(0, 0, 0);
    float rootScale = 100.0f;

    //if (getValidMask(Elements[0].masks, childHitIndex) > 0 && getLeafMask(Elements[0].masks, childHitIndex) > 0)
    //{
    //    float4 child = getChildBox(rootPos, rootScale, childHitIndex);
    //
    //    float3 voxNorm = getNormal(child.xyz, pos);
    //    float3 voxColor = float3(1, 0, 0) * clamp(dot(voxNorm, normalize(dirLight)), 0, 1);
    //    voxColor += float3(1, 0, 0) * (clamp(dot(voxNorm, normalize(movingLight - pos)), 0, 1) / length(movingLight - pos)) * 1.5;
    //
    //    return float4(voxColor, 1);
    //}
    //else
    {
        float4 child = float4(rootPos, rootScale);// getChildBox(rootPos, rootScale, childHitIndex);
        float2 retChild = raytraceBox(child.xyz, child.w, cameraPos, dir);

        //if (retChild.x > 0.0) return float4(1, 0, 0, 1);
        //else if (retChild.x <= 0.0 && retChild.y > 0.0) return float4(0, 1, 0, 1);

        float rootExit = retChild.y;
        float exitMax = retChild.y;
        uint rootIndex = 0;

        //if(retChild.x < 0.0) return float4(0.3, 0, 0.3, 1);

        float3 childPos = cameraPos + (retChild.x * dir);

        uint childHitIndex = getChildIndex(rootPos, childPos);
        float3 indexPos = childPos;
        child = getChildBox(rootPos, rootScale, childHitIndex);

        bool didNotHit = true;

        uint steps = 0;

        while (didNotHit)
        {
            float2 retChild = raytraceBox(child.xyz, child.w, cameraPos, dir);
            childPos = cameraPos + (retChild.x * dir);
            float3 childPosMax = cameraPos + (retChild.y * dir * 1.000025);

            if (stackIndex > 0 && steps > 1000) return float4(0, 0, 0, 1);
            //if (stackIndex > 2) return float4(1, 0, 0, 1);

            //if (retChild.y < 0.0) return float4(1, 1, 0, 1);
            //if (retChild.y < 0.0) return float4(abs(retChild.y) / 100.0f, abs(retChild.x)/100.0f, 0, 1);

            if (getValidMask(Elements[rootIndex].masks, childHitIndex) > 0 && retChild.x > 0.0)
            {
                if (getLeafMask(Elements[rootIndex].masks, childHitIndex) > 0)
                {
                    //if (stackIndex > 0) return float4(1, 0, 0, 1);
                    didNotHit = false;
                    break;
                }
                else
                {
                    ParentElement parent;
                    parent.pos = rootPos;
                    parent.scale = rootScale;
                    parent.exit = rootExit;
                    parent.index = rootIndex;
                    stack[stackIndex++] = parent;

                    rootPos = child.xyz;
                    rootScale = child.w;

                    rootExit = retChild.y;
                    rootIndex = getChildPointer(Elements[rootIndex], childHitIndex);

                    //return float4(rootIndex, 0, 0, 1);
                    
                    childHitIndex = getChildIndex(rootPos, childPos);
                    indexPos = childPos;
                    child = getChildBox(rootPos, rootScale, childHitIndex);

                    //if (childHitIndex == 0) return float4(0, 0, 1, 1);

                    continue;
                }
            }
            else if (retChild.y >= rootExit)
            {
                if (retChild.y >= exitMax) return float4(0.3, 0, 0.3, 1);
                else
                {
                    --stackIndex;
                    rootPos = stack[stackIndex].pos;
                    rootScale = stack[stackIndex].scale;
                    rootExit = stack[stackIndex].exit;
                    rootIndex = stack[stackIndex].index;

                    childHitIndex = getChildIndex(rootPos, childPosMax);
                    indexPos = childPosMax;
                    child = getChildBox(rootPos, rootScale, childHitIndex);

                    //return float4(1, 1, 0, 1);
                }
            }
            else
            {
                //return float4(0, 1, 0, 1);
                if (retChild.y < 0.0) return float4(0, 1, 0, 1);
                childHitIndex = getChildIndex(rootPos, childPosMax);
                indexPos = childPosMax;
                child = getChildBox(rootPos, rootScale, childHitIndex);
                if (stackIndex > 0) ++steps;
            }

        }
        if (!didNotHit)
        {
            float3 voxNorm = getNormal(child.xyz, indexPos);
            float3 voxColor = float3(0, 0.3, 0) * clamp(dot(voxNorm, normalize(dirLight)), 0, 1);
            voxColor += float3(0, 0.3, 0) * ((clamp(dot(voxNorm, normalize(movingLight - indexPos)), 0, 1) / length(movingLight - indexPos)) * 1.5) * 100.0f;

            return float4(voxColor, 1);
        }
    }

    return float4(0.3, 0.0, 0.3, 1);

}