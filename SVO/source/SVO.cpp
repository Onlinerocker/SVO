#include "SVO.h"

SVO::SVO(size_t size)
{
	_data.reserve(size);
}

std::vector<SVO::Element>& SVO::vec()
{
	return _data;
}