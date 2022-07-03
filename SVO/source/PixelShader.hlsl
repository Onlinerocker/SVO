#define PI 3.14159265359

struct SVOElement
{
    uint childPointer;
    uint masks;
    //uint pad;
    //uint pad1;
};

struct ParentElement
{
    float3 pos;
    float scale;
    float exit;
    uint index;
    uint childIndex;
};

cbuffer CameraInfo : register(b0)
{
    float3 CamPos;
    float Time;

    float4 Forward;
    float4 Right;
    float4 Up;

    uint HitIndex;
    uint HitChildIndex;
    uint DidHit;
    uint EditMode;

    float3 PlacementPos;
    float RootRadius;
};

cbuffer SVOInfo : register(b1)
{
    SVOElement Elements1[4096];
};

StructuredBuffer<SVOElement> Elements : register(t0);

float calculateT(float plane, float origin, float direction)
{
    if (plane - origin == 0.0f && direction == 0.0f) return 999999.0;
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
    //uint curPos = 0;
    //for (uint i = 0; i < index; ++i)
    //{
    //    if (getValidMask(parentElement.masks, i) > 0 && getLeafMask(parentElement.masks, i) == 0)
    //        ++curPos;
    //}
    return parentElement.childPointer + index;
}

float sdBox(float3 p, float3 b)
{
    float3 q = abs(p) - b;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

uint getChildIndex(float3 boxPos, float3 pos, float scale, float3 camPos, float3 dir)
{
    float3 offsets[8] =
    {
        {0.5,  -0.5, 0.5},
        {-0.5, -0.5, 0.5},
        {-0.5, -0.5, -0.5},
        {0.5,  -0.5, -0.5},

        {0.5, 0.5, 0.5},
        {-0.5, 0.5, 0.5},
        {-0.5, 0.5, -0.5},
        {0.5, 0.5, -0.5},
    };

    uint val = 9;
    bool first = true;
    float minDist = 1.0f;

    //if (sdBox(pos - boxPos, float3(scale, scale, scale)) > 0.0) return 9;

    if (pos.x >= boxPos.x && pos.y <= boxPos.y && pos.z >= boxPos.z)
    {
        float d = length((boxPos + (offsets[0] * scale)) - camPos);
        if (d < minDist || first)
        {
            minDist = d;
            val = 0;
            first = false;
        }
    }

    if (pos.x <= boxPos.x && pos.y <= boxPos.y && pos.z >= boxPos.z)
    {
        float d = length((boxPos + (offsets[1] * scale)) - camPos);
        if (d < minDist || first)
        {
            minDist = d;
            val = 1;
            first = false;
        }
    }

    if (pos.x <= boxPos.x && pos.y <= boxPos.y && pos.z <= boxPos.z)
    {
        float d = length((boxPos + (offsets[2] * scale)) - camPos);
        if (d < minDist || first)
        {
            minDist = d;
            val = 2;
            first = false;
        }
    }

    if (pos.x >= boxPos.x && pos.y <= boxPos.y && pos.z <= boxPos.z)
    {
        float d = length((boxPos + (offsets[3] * scale)) - camPos);
        if (d < minDist || first)
        {
            minDist = d;
            val = 3;
            first = false;
        }
    }

    if (pos.x >= boxPos.x && pos.y >= boxPos.y && pos.z >= boxPos.z)
    {
        float d = length((boxPos + (offsets[4] * scale)) - camPos);
        if (d < minDist || first)
        {
            minDist = d;
            val = 4;
            first = false;
        }
    }

    if (pos.x <= boxPos.x && pos.y >= boxPos.y && pos.z >= boxPos.z)
    {
        float d = length((boxPos + (offsets[5] * scale)) - camPos);
        if (d < minDist || first)
        {
            minDist = d;
            val = 5;
            first = false;
        }
    }

    if (pos.x <= boxPos.x && pos.y >= boxPos.y && pos.z <= boxPos.z)
    {
        float d = length((boxPos + (offsets[6] * scale)) - camPos);
        if (d < minDist || first)
        {
            minDist = d;
            val = 6;
            first = false;
        }
    }

    if (pos.x >= boxPos.x && pos.y >= boxPos.y && pos.z <= boxPos.z)
    {
        float d = length((boxPos + (offsets[7] * scale)) - camPos);
        if (d < minDist || first)
        {
            minDist = d;
            val = 7;
            first = false;
        }
    }

    return val;
}

float4 getChildBox(float3 rootPos, float rootScale, uint index)
{
    float newScale = rootScale * 0.5;

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

float3 getNormal(float3 boxPos, float3 pos, float3 dir)
{
    float3 v = pos - boxPos;
    float3 vAbs = abs(v);

    //if (vAbs.x == vAbs.y && vAbs.x != vAbs.z)
    //{
    //    return sign(v.x) * float3(1, 0, 0) + sign(v.y) * float3(0, 1, 0);
    //}

    float3 norm = float3(0, 0, 0);
    float dMax = -1.0f;


    if (vAbs.x >= vAbs.y && vAbs.x >= vAbs.z && dir.x != 0.0)
    {
        float3 normX = sign(v.x) * float3(1, 0, 0);
        norm += normX;
        //float d = dot(normX, dir);
        //if (d > dMax)
        //{
        //    dMax = d;
        //    norm = normX;
        //}
    }

    if (vAbs.y >= vAbs.x && vAbs.y >= vAbs.z && dir.y != 0.0)
    {
        float3 normY = sign(v.y) * float3(0, 1, 0);
        norm += normY;
        //float d = dot(normY, dir);
        //if (d > dMax)
        //{
        //    dMax = d;
        //    norm = normY;
        //}
    }

    if (vAbs.z >= vAbs.x && vAbs.z >= vAbs.y && dir.z != 0.0)
    {
        float3 normZ = sign(v.z) * float3(0, 0, 1);
        norm += normZ;
        //float d = dot(normZ, dir);
        //if (d > dMax)
        //{
        //    dMax = d;
        //    norm = normZ;
        //}
    }

    return norm;
}

uint getChildIndexNext(float3 boxPos, float3 pos, float rootScale, uint prevIndex, float3 dir)
{
    //float3 offsets[8] =
    //{
    //    {0.5,  -0.5, 0.5},
    //    {-0.5, -0.5, 0.5},
    //    {-0.5, -0.5, -0.5},
    //    {0.5,  -0.5, -0.5},
    //
    //    {0.5, 0.5, 0.5},
    //    {-0.5, 0.5, 0.5},
    //    {-0.5, 0.5, -0.5},
    //    {0.5, 0.5, -0.5},
    //};
    //
    //uint val = 9;
    //bool first = true;
    //float minDist = 1.0f;
    //float scale = rootScale;
    //float camPos = CamPos;
    //float thresh = length((boxPos + (offsets[prevIndex] * scale)) - camPos);
    //
    //if ((prevIndex == 1 || prevIndex == 3 || prevIndex == 4) && prevIndex != 0 && pos.x >= boxPos.x && pos.y <= boxPos.y && pos.z >= boxPos.z)
    //{
    //    float d = length((boxPos + (offsets[0] * scale)) - camPos);
    //    if ((d < minDist && d > thresh) || first)
    //    {
    //        minDist = d;
    //        val = 0;
    //        first = false;
    //    }
    //}
    //
    //if ((prevIndex == 0 || prevIndex == 2 || prevIndex == 5) && prevIndex != 1 && pos.x <= boxPos.x && pos.y <= boxPos.y && pos.z >= boxPos.z)
    //{
    //    float d = length((boxPos + (offsets[1] * scale)) - camPos);
    //    if ((d < minDist && d > thresh) || first)
    //    {
    //        minDist = d;
    //        val = 1;
    //        first = false;
    //    }
    //}
    //
    //if ((prevIndex == 1 || prevIndex == 3 || prevIndex == 6) && prevIndex != 2 && pos.x <= boxPos.x && pos.y <= boxPos.y && pos.z <= boxPos.z)
    //{
    //    float d = length((boxPos + (offsets[2] * scale)) - camPos);
    //    if ((d < minDist && d > thresh) || first)
    //    {
    //        minDist = d;
    //        val = 2;
    //        first = false;
    //    }
    //}
    //
    //if ((prevIndex == 2 || prevIndex == 0 || prevIndex == 7) && prevIndex != 3 && pos.x >= boxPos.x && pos.y <= boxPos.y && pos.z <= boxPos.z)
    //{
    //    float d = length((boxPos + (offsets[3] * scale)) - camPos);
    //    if ((d < minDist && d > thresh) || first)
    //    {
    //        minDist = d;
    //        val = 3;
    //        first = false;
    //    }
    //}
    //
    //if ((prevIndex == 0 || prevIndex == 5 || prevIndex == 7) && prevIndex != 4 && pos.x >= boxPos.x && pos.y >= boxPos.y && pos.z >= boxPos.z)
    //{
    //    float d = length((boxPos + (offsets[4] * scale)) - camPos);
    //    if ((d < minDist && d > thresh) || first)
    //    {
    //        minDist = d;
    //        val = 4;
    //        first = false;
    //    }
    //}
    //
    //if ((prevIndex == 1 || prevIndex == 4 || prevIndex == 6) && prevIndex != 5 && pos.x <= boxPos.x && pos.y >= boxPos.y && pos.z >= boxPos.z)
    //{
    //    float d = length((boxPos + (offsets[5] * scale)) - camPos);
    //    if ((d < minDist && d > thresh) || first)
    //    {
    //        minDist = d;
    //        val = 5;
    //        first = false;
    //    }
    //}
    //
    //if ((prevIndex == 5 || prevIndex == 2 || prevIndex == 7) && prevIndex != 6 && pos.x <= boxPos.x && pos.y >= boxPos.y && pos.z <= boxPos.z)
    //{
    //    float d = length((boxPos + (offsets[6] * scale)) - camPos);
    //    if ((d < minDist && d > thresh) || first)
    //    {
    //        minDist = d;
    //        val = 6;
    //        first = false;
    //    }
    //}
    //
    //if ((prevIndex == 6 || prevIndex == 4 || prevIndex == 3) && prevIndex != 7 && pos.x >= boxPos.x && pos.y >= boxPos.y && pos.z <= boxPos.z)
    //{
    //    float d = length((boxPos + (offsets[7] * scale)) - camPos);
    //    if ((d < minDist && d > thresh) || first)
    //    {
    //        minDist = d;
    //        val = 7;
    //        first = false;
    //    }
    //}
    //
    //return val;

    //uint id = getChildIndex(boxPos, pos + dir * 0.01, rootScale, CamPos);
    //
    //if (id == prevIndex) return 9;
    //
    //return id;

    float4 prev = getChildBox(boxPos, rootScale, prevIndex);
    float3 norm = getNormal(prev.xyz, pos, dir);

    //if (prevIndex > 7) return 10;
    //if (prev.x == 0) return 10;

    //if you hit a corner or edge... enter on the side closest to you... exit on the side farther from you...

    if (prevIndex == 0)
    {
        if (norm.x < 0.0 && dir.x < 0.0)
            return 1;
        if (norm.y > 0.0 && dir.y > 0.0)
            return 4;
        if (norm.z < 0.0 && dir.z < 0.0)
            return 3;
    }
    
    if (prevIndex == 1)
    {
        if (norm.x > 0.0 && dir.x > 0.0)
            return 0;
        if (norm.y > 0.0 && dir.y > 0.0)
            return 5;
        if (norm.z < 0.0 && dir.z < 0.0)
            return 2;
    }
    
    if (prevIndex == 2)
    {
        if (norm.x > 0.0 && dir.x > 0.0)
            return 3;
        if (norm.y > 0.0 && dir.y > 0.0)
            return 6;
        if (norm.z > 0.0 && dir.z > 0.0)
            return 1;
    }
    
    if (prevIndex == 3)
    {
        if (norm.x < 0.0 && dir.x < 0.0)
            return 2;
        if (norm.y > 0.0 && dir.y > 0.0)
            return 7;
        if (norm.z > 0.0 && dir.z > 0.0)
            return 0;
    }
    
    //top row
    if (prevIndex == 4)
    {
        if (norm.x < 0.0 && dir.x < 0.0)
            return 5;
        if (norm.y < 0.0 && dir.y < 0.0)
            return 0;
        if (norm.z < 0.0 && dir.z < 0.0)
            return 7;
    }
    
    if (prevIndex == 5)
    {
        if (norm.x > 0.0 && dir.x > 0.0)
            return 4;
        if (norm.y < 0.0 && dir.y < 0.0)
            return 1;
        if (norm.z < 0.0 && dir.z < 0.0)
            return 6;
    }
    
    if (prevIndex == 6)
    {
        if (norm.x > 0.0 && dir.x > 0.0)
            return 7;
        if (norm.y < 0.0 && dir.y < 0.0)
            return 2;
        if (norm.z > 0.0 && dir.z > 0.0)
            return 5;
    }
    
    if (prevIndex == 7)
    {
        if (norm.x < 0.0 && dir.x < 0.0)
            return 6;
        if (norm.y < 0.0 && dir.y < 0.0)
            return 3;
        if (norm.z > 0.0 && dir.z > 0.0)
            return 4;
    }
    
    return 9;
}

bool isInside(float3 pos, float3 posRoot, float scale)
{
    return pos.x <= posRoot.x + scale && pos.x >= posRoot.x - scale &&
        pos.y <= posRoot.y + scale && pos.y >= posRoot.y - scale &&
        pos.z <= posRoot.z + scale && pos.z >= posRoot.z - scale;

}

float4 pixelMain(float4 position : SV_POSITION) : SV_TARGET
{
    float x = position.x - 960.0;
    float y = position.y - 540.0;

    float len = length(float2(x, y));
    if (len <= 5.0 && len >= 3.0) return float4(1, 0, 0, 1);

    x /= 960.0;
    y /= -540.0;
    x *= (1280.0 / 720.0);

    ParentElement stack[64];
    uint stackIndex = 0;

    float3 cameraPos = CamPos; //float3(-1, 0.7, 0);
    float3 dir = (x * Right.xyz) + (y * Up.xyz) + Forward.xyz;
    dir = normalize(dir);

    float4 placementColor = float4(0, 0.5 + 0.2 * sin(Time * 7), 0, 1);
    float2 retPlace = raytraceBox(PlacementPos.xyz, 0.5, cameraPos, dir);

    float3 col = float3(0.3, 0.3, 0.3);
    float tMin = 0;
	
    float3 dirLight = float3(0, 1, -1);
    float3 movingLight = float3(512.0 /*200*sin(Time)*/, 300, -0);

    float3 rootPos = float3(0, 0, 0);
    float rootScale = RootRadius;

    {
        float4 child = float4(rootPos, rootScale);
        float2 retChild = raytraceBox(child.xyz, child.w, cameraPos, dir);
        
        float rootExit = retChild.y;
        float exitMax = retChild.y;
        uint rootIndex = 0;

        float4 noHitColor = float4(0.2, 0.2, 0.2, 1);
        float4 escapeColor = float4(0.3, 0, 0.3, 1);
        
        if (retChild.x <= 0.0 && retChild.y <= 0.0)
        {
            return noHitColor;
        }

        float3 childPos = cameraPos + (retChild.x * dir);

        uint childHitIndex = getChildIndex(rootPos, childPos, rootScale, cameraPos, dir);
        float3 indexPos = childPos;
        child = getChildBox(rootPos, rootScale, childHitIndex);

        bool didNotHit = true;

        uint steps = 0;

        while (didNotHit)
        {
            retChild = raytraceBox(child.xyz, child.w, cameraPos, dir);
            childPos = cameraPos + (retChild.x * dir);
            
            float3 childPosMax = cameraPos + (retChild.y * dir);

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

                    rootPos = child.xyz;
                    rootScale = child.w;

                    rootExit = retChild.y;
                    rootIndex = getChildPointer(Elements[rootIndex], childHitIndex);

                    float3 downChild = cameraPos + (retChild.x * dir);
                    childHitIndex = getChildIndex(rootPos, downChild, rootScale, cameraPos, dir);
                    //if (childHitIndex > 7)
                    //    return float4(1, 1, 1, 1);

                    indexPos = childPos;
                    child = getChildBox(rootPos, rootScale, childHitIndex);

                    ++steps;
                    continue;
                }
            }
            
            if (retChild.y >= rootExit)
            {
                if (retChild.y >= exitMax)
                {
                    if (EditMode == 1 && retPlace.x >= 0.0 && DidHit)
                    {
                        return placementColor;
                    }

                    return escapeColor;
                }
                else
                {
                    --stackIndex;
                    rootPos = stack[stackIndex].pos;
                    rootScale = stack[stackIndex].scale;
                    rootExit = stack[stackIndex].exit;
                    rootIndex = stack[stackIndex].index;

                    childHitIndex = getChildIndexNext(rootPos, childPosMax, rootScale, stack[stackIndex].childIndex, dir);
                    while (childHitIndex > 7 && stackIndex >= 1)
                    {
                        --stackIndex;
                        rootPos = stack[stackIndex].pos;
                        rootScale = stack[stackIndex].scale;
                        rootExit = stack[stackIndex].exit;
                        rootIndex = stack[stackIndex].index;

                        childHitIndex = getChildIndexNext(rootPos, childPosMax, rootScale, stack[stackIndex].childIndex, dir);
                    }
                        
                    indexPos = childPosMax;
                    child = getChildBox(rootPos, rootScale, childHitIndex);
                }
            }
            else
            {
                childHitIndex = getChildIndexNext(rootPos, childPosMax, rootScale, childHitIndex, dir);
                if (childHitIndex > 7)
                {
                    return float4(0, 0, 0, 1);
                }
                indexPos = childPosMax;
                child = getChildBox(rootPos, rootScale, childHitIndex);
            }

            ++steps;
        }
        if (!didNotHit)
        {
            float3 voxNorm = getNormal(child.xyz, indexPos, float3(1,1,1));
            float3 diffColor = lerp(float3(0.43, 0.31, 0.22) * 0.3, float3(0, 0.3, 0), smoothstep(100.0, 200.0, indexPos.y));
            float3 voxColor = diffColor * clamp(dot(voxNorm, normalize(dirLight)), 0, 1) * 2.0;
            voxColor += diffColor * ((clamp(dot(voxNorm, normalize(movingLight - indexPos)), 0, 1)) * 1.5) * 1.0f;
            voxColor += diffColor * ((clamp(dot(voxNorm, normalize(-movingLight - indexPos)), 0, 1)) * 1.5) * 1.0f;
            voxColor += diffColor * ((clamp(dot(voxNorm, normalize(float3(0, 0, movingLight.x) - indexPos)), 0, 1)) * 1.5) * 1.0f;

            //return lerp(float4(voxColor, 1), noHitColor, saturate(retChild.x / 100.0f));
            if (EditMode == 0 && HitIndex == rootIndex && HitChildIndex == childHitIndex)
            {
                float pulse = 0.2 * sin(Time * 7);
                return lerp(float4(voxColor, 1), float4(0, 0.5 + pulse, 0.5 + pulse, 1), 0.5);
            }
            else if (EditMode == 1 && retPlace.x >= 0.0 && retPlace.x < retChild.x && DidHit)
            {
                return placementColor;
            }
            
            return float4(voxColor, 1);
        }
    }
    
    return float4(0.2, 0.2, 0.2, 1);

}