#pragma once
#include <bit>
//TODO: Add this wrappers

template<typename T>
inline T* MEMORY_Alloc(void* pUserData, size_t nSize)
{
	return std::bit_cast<T*>(malloc(nSize));
}

void MEMORY_Free(void* pUserData, void* pPtr)
{
	free(pPtr);
}