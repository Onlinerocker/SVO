#pragma once
#include <cinttypes>
#include <vector>
#include <glm\glm.hpp>

class SVO
{

	/* Mask Mapping
	
	Each bit corresponds to one of the child voxels.
	The first four bits correspond to the negative Y voxels.
	The second four bits correspond to the positive Y voxels.
	
	If you're looking at either of these set of four voxels from
	positive Y (i.e. "top down") the mapping is as follows:

	       |
	 0010  | 0001  
	-------|-------
	 0100  | 1000
		   |

	In other words:

	(+X, +Z): 0001
	(-X, +Z): 0010
	(-X, -Z): 0100
	(+X, -Z): 1000

	*/

public:
	using float2 = glm::vec2;
	using float3 = glm::vec3;
	using float4 = glm::vec4;
	using uint = uint32_t;

	struct Element
	{
		uint32_t childPointer;  //relative pointer to children in SVO
		uint32_t masks; //8 most sig bits = valid mask, next 8 bits = leaf mask
		//uint64_t padding; //needed for cbuffer
	};

	struct ParentElement
	{
		float3 pos;
		float scale;
		float exit;
		uint index;
		uint childIndex;
	};

	struct HitReturn
	{
		uint32_t index;
		uint8_t childIndex{ 0 };
		bool didHit{ false };

		ParentElement stack[16];
		uint32_t stackIndex;
	};

public:

	SVO() = default;
	
	SVO(const SVO& other) = default;
	SVO& operator=(const SVO& other) = default;

	SVO(SVO&& other) = default;
	SVO& operator=(SVO&& other) = default;

	~SVO() = default;

	SVO(size_t size);

	std::vector<Element>& vec();

	uint getValidMask(uint mask, uint index);
	uint getLeafMask(uint mask, uint index);
	uint getChildPointer(Element parentElement, uint index);
	uint getChildIndex(float3 boxPos, float3 pos);
	float3 getNormal(float3 boxPos, float3 pos);
	uint getChildIndexNext(float3 boxPos, float3 pos, float rootScale, uint prevIndex);
	float4 getChildBox(float3 rootPos, float rootScale, uint index);
	float calculateT(float plane, float origin, float direction);
	float2 raytraceBox(float3 boxPos, float boxRad, float3 cameraPos, float3 rayDirection);
	bool isInside(float3 pos, float3 posRoot, float scale);
	HitReturn getHit(float3 cameraPos, float3 dir);

private:
	std::vector<Element> Elements;

};

