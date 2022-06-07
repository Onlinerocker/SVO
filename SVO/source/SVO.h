#pragma once
#include <cinttypes>
#include <vector>

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
	struct Element
	{
		uint32_t childPtr;  //relative pointer to children in SVO
		uint32_t masks;
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

private:
	std::vector<Element> _data;

};

