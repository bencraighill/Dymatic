#pragma once

#include <map>

namespace Dymatic::Memory
{
	class Allocator
	{
	public:
		static void Init();

		static void* AllocateRaw(size_t size);
		static void* Allocate(size_t size);
		static void* Allocate(size_t size, const char* desc);
		static void* Allocate(size_t size, const char* file, int line);
		static void FreeRaw(void* memory);
		static void Free(void* memory);
	};

	struct Allocation
	{
		void* Memory;
		size_t Size;
		const char* Category;
	};

	struct AllocationStats
	{
		size_t TotalAllocated;
		size_t TotalFreed;
	};

	std::map<void*, Allocation>& GetMemoryAllocations();
	std::map<const char*, AllocationStats>& GetMemoryAllocationStats();
	const AllocationStats& GetAllocationStats();
}

inline void* operator new(size_t size)
{
	return Dymatic::Memory::Allocator::Allocate(size);
}

inline void* operator new (size_t size, const char* desc)
{
	return Dymatic::Memory::Allocator::Allocate(size, desc);
}

inline void* operator new (size_t size, const char* file, int line)
{
	return Dymatic::Memory::Allocator::Allocate(size, file, line);
}

// Delete
inline void operator delete(void* ptr)
{
	Dymatic::Memory::Allocator::Free(ptr);
}

//#define dynew new(__FUNCTION__)
#define dynew new(__FILE__, __LINE__)
#define dydelete delete