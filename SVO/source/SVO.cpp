#include "SVO.h"

SVO::SVO(size_t size)
{
	Elements.reserve(size);
}

std::vector<SVO::Element>& SVO::vec()
{
	return Elements;
}

SVO::uint SVO::getValidMask(uint mask, uint index)
{
    uint x = 1 << (24 + index);
    return mask & x;
}

SVO::uint SVO::getLeafMask(uint mask, uint index)
{
    uint x = 1 << (16 + index);
    return mask & x;
}

SVO::uint SVO::getChildPointer(Element parentElement, uint index)
{
    uint curPos = 0;
    for (uint i = 0; i < index; ++i)
    {
        if (getValidMask(parentElement.masks, i) > 0 && getLeafMask(parentElement.masks, i) == 0)
            ++curPos;
    }
    return parentElement.childPointer + curPos;
}

SVO::uint SVO::getChildIndex(float3 boxPos, float3 pos)
{
    if (pos.x >= boxPos.x && pos.y <= boxPos.y && pos.z >= boxPos.z)
        return 0;
    if (pos.x <= boxPos.x && pos.y <= boxPos.y && pos.z >= boxPos.z)
        return 1;
    if (pos.x <= boxPos.x && pos.y <= boxPos.y && pos.z <= boxPos.z)
        return 2;
    if (pos.x >= boxPos.x && pos.y <= boxPos.y && pos.z <= boxPos.z)
        return 3;

    if (pos.x >= boxPos.x && pos.y >= boxPos.y && pos.z >= boxPos.z)
        return 4;
    if (pos.x <= boxPos.x && pos.y >= boxPos.y && pos.z >= boxPos.z)
        return 5;
    if (pos.x <= boxPos.x && pos.y >= boxPos.y && pos.z <= boxPos.z)
        return 6;
    if (pos.x >= boxPos.x && pos.y >= boxPos.y && pos.z <= boxPos.z)
        return 7;

    return 9;
}

SVO::float3 SVO::getNormal(float3 boxPos, float3 pos)
{
    float3 v = pos - boxPos;
    float3 vAbs = abs(v);

    if (vAbs.x >= vAbs.y && vAbs.x >= vAbs.z)
    {
        return glm::sign(v.x) * float3(1, 0, 0);
    }

    if (vAbs.y > vAbs.x && vAbs.y >= vAbs.z)
    {
        return glm::sign(v.y) * float3(0, 1, 0);
    }

    return glm::sign(v.z) * float3(0, 0, 1);
}

SVO::uint SVO::getChildIndexNext(float3 boxPos, float3 pos, float rootScale, uint prevIndex)
{
    float4 prev = getChildBox(boxPos, rootScale, prevIndex);
    float3 norm = getNormal({prev.x, prev.y, prev.z}, pos);

    if (prevIndex == 0)
    {
        if (norm.x < 0.0)
            return 1;
        if (norm.y > 0.0)
            return 4;
        if (norm.z < 0.0)
            return 3;
    }

    if (prevIndex == 1)
    {
        if (norm.x > 0.0)
            return 0;
        if (norm.y > 0.0)
            return 5;
        if (norm.z < 0.0)
            return 2;
    }

    if (prevIndex == 2)
    {
        if (norm.x > 0.0)
            return 3;
        if (norm.y > 0.0)
            return 6;
        if (norm.z > 0.0)
            return 1;
    }

    if (prevIndex == 3)
    {
        if (norm.x < 0.0)
            return 2;
        if (norm.y > 0.0)
            return 7;
        if (norm.z > 0.0)
            return 0;
    }

    //top row
    if (prevIndex == 4)
    {
        if (norm.x < 0.0)
            return 5;
        if (norm.y < 0.0)
            return 0;
        if (norm.z < 0.0)
            return 7;
    }

    if (prevIndex == 5)
    {
        if (norm.x > 0.0)
            return 4;
        if (norm.y < 0.0)
            return 1;
        if (norm.z < 0.0)
            return 6;
    }

    if (prevIndex == 6)
    {
        if (norm.x > 0.0)
            return 7;
        if (norm.y < 0.0)
            return 2;
        if (norm.z > 0.0)
            return 5;
    }

    if (prevIndex == 7)
    {
        if (norm.x < 0.0)
            return 6;
        if (norm.y < 0.0)
            return 3;
        if (norm.z > 0.0)
            return 4;
    }

    return 9;
}

SVO::float4 SVO::getChildBox(float3 rootPos, float rootScale, uint index)
{
    float newScale = rootScale * 0.5f;

    if (index == 0)
        return float4(rootPos + float3(newScale, -newScale, newScale), newScale);
    if (index == 1)
        return float4(rootPos + float3(-newScale, -newScale, newScale), newScale);
    if (index == 2)
        return float4(rootPos + float3(-newScale, -newScale, -newScale), newScale);
    if (index == 3)
        return float4(rootPos + float3(newScale, -newScale, -newScale), newScale);

    if (index == 4)
        return float4(rootPos + float3(newScale, newScale, newScale), newScale);
    if (index == 5)
        return float4(rootPos + float3(-newScale, newScale, newScale), newScale);
    if (index == 6)
        return float4(rootPos + float3(-newScale, newScale, -newScale), newScale);
    if (index == 7)
        return float4(rootPos + float3(newScale, newScale, -newScale), newScale);

    return float4(0, 0, 0, 0);
}

float SVO::calculateT(float plane, float origin, float direction)
{
    //if (plane - origin == 0 || direction == 0) return 0;
    return (plane - origin) / direction;
}

//https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
SVO::float2 SVO::raytraceBox(float3 boxPos, float boxRad, float3 cameraPos, float3 rayDirection)
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
    tMax = glm::min(tMax, tyMax);

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
    tMax = glm::min(tMax, tzMax);

    return float2(tMin, tMax);
}

bool SVO::isInside(float3 pos, float3 posRoot, float scale)
{
    return pos.x <= posRoot.x + scale && pos.x >= posRoot.x - scale &&
        pos.y <= posRoot.y + scale && pos.y >= posRoot.y - scale &&
        pos.z <= posRoot.z + scale && pos.z >= posRoot.z - scale;
}

SVO::HitReturn SVO::getHit(float3 cameraPos, float3 dir)
{
    ParentElement stack[64];
    uint stackIndex = 0;

    float3 rootPos = float3(0, 0, 0);
    float rootScale = 256.0;

    {
        float4 child = float4(rootPos, rootScale);
        float2 retChild = raytraceBox({child.x, child.y, child.z}, child.w, cameraPos, dir);

        float rootExit = retChild.y;
        float exitMax = retChild.y;
        uint rootIndex = 0;

        float4 noHitColor = float4(0.2, 0.2, 0.2, 1);
        float4 escapeColor = float4(0.3, 0, 0.3, 1);

        float3 childPos = cameraPos + (retChild.x * dir);

        uint childHitIndex = getChildIndex(rootPos, childPos);
        if (childHitIndex > 7)
            return { 0, 0, false };
        float3 indexPos = childPos;
        child = getChildBox(rootPos, rootScale, childHitIndex);

        bool didNotHit = true;

        uint steps = 0;

        while (didNotHit)
        {
            retChild = raytraceBox({ child.x, child.y, child.z }, child.w, cameraPos, dir);

            childPos = cameraPos + (retChild.x * dir);
            float3 childPosMax = cameraPos + (retChild.y * dir);

            if (steps > 500)
                return { 0, 0, false };

            if (getValidMask(Elements[rootIndex].masks, childHitIndex) > 0 && (retChild.x >= 0.0 || (retChild.x < 0.0 && isInside(cameraPos, rootPos, rootScale))))
            {
                if (getLeafMask(Elements[rootIndex].masks, childHitIndex) > 0 && retChild.x >= 0.0 && retChild.y > 0.0)
                {
                    didNotHit = false;
                    break;
                }
                else if (getLeafMask(Elements[rootIndex].masks, childHitIndex) == 0)
                {
                    ParentElement parent;
                    parent.pos = rootPos;
                    parent.scale = rootScale;
                    parent.exit = rootExit;
                    parent.index = rootIndex;
                    parent.childIndex = childHitIndex;
                    stack[stackIndex++] = parent;

                    rootPos = { child.x, child.y, child.z };
                    rootScale = child.w;

                    rootExit = retChild.y;
                    rootIndex = getChildPointer(Elements[rootIndex], childHitIndex);

                    float3 downChild = cameraPos + (retChild.x * dir);
                    childHitIndex = getChildIndex(rootPos, downChild);
                    if (childHitIndex > 7)
                        return { 0, 0, false };

                    indexPos = childPos;
                    child = getChildBox(rootPos, rootScale, childHitIndex);

                    continue;
                }
            }

            if (retChild.y >= rootExit)
            {
                if (retChild.y >= exitMax)
                {
                    return { 0, 0, false };
                }
                else
                {
                    --stackIndex;
                    rootPos = stack[stackIndex].pos;
                    rootScale = stack[stackIndex].scale;
                    rootExit = stack[stackIndex].exit;
                    rootIndex = stack[stackIndex].index;

                    childHitIndex = getChildIndexNext(rootPos, childPosMax, rootScale, stack[stackIndex].childIndex);
                    while (childHitIndex > 7 && stackIndex >= 1)
                    {
                        --stackIndex;
                        rootPos = stack[stackIndex].pos;
                        rootScale = stack[stackIndex].scale;
                        rootExit = stack[stackIndex].exit;
                        rootIndex = stack[stackIndex].index;

                        childHitIndex = getChildIndexNext(rootPos, childPosMax, rootScale, stack[stackIndex].childIndex);
                    }

                    indexPos = childPosMax;
                    child = getChildBox(rootPos, rootScale, childHitIndex);
                }
            }
            else
            {
                childHitIndex = getChildIndexNext(rootPos, childPosMax, rootScale, childHitIndex);
                indexPos = childPosMax;
                child = getChildBox(rootPos, rootScale, childHitIndex);
            }

            ++steps;
        }
        if (!didNotHit)
        {
            return { rootIndex, (uint8_t)childHitIndex, true };
        }
    }

    return { 0, 0, false };
}