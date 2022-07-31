#include "dypch.h"
#include "Memory.h"

#include <map>

namespace Dymatic::Memory
{
	static Dymatic::Memory::AllocationStats s_GlobalStats;

	static bool s_InInit = false;
	static bool s_InAlloc = false;
	static bool s_InFree = false;

	struct AllocatorData
	{
		AllocatorData()
		{
		}

		std::map<void*, Allocation> m_AllocationMap;
		std::map<const char*, AllocationStats> m_AllocationStatsMap;
	};
	static AllocatorData* s_Data = nullptr;

	void Allocator::Init()
	{
		if (s_Data)
			return;

		s_InInit = true;
		AllocatorData* data = (AllocatorData*)Allocator::AllocateRaw(sizeof(AllocatorData));
		new(data) AllocatorData();
		s_Data = data;
		s_InInit = false;
	}

	void* Allocator::AllocateRaw(size_t size)
	{
		return malloc(size);
	}

	void* Allocator::Allocate(size_t size)
	{
		if (s_InInit || s_InAlloc)
			return AllocateRaw(size);

		if (!s_Data)
			Init();

		s_InAlloc = true;

		void* memory = malloc(size);

		{
			Allocation& alloc = s_Data->m_AllocationMap[memory];
			alloc.Memory = memory;
			alloc.Size = size;

			s_GlobalStats.TotalAllocated += size;
		}

		s_InAlloc = false;
		return memory;
	}

	void* Allocator::Allocate(size_t size, const char* desc)
	{
		if (!s_Data)
			Init();

		void* memory = malloc(size);

		{
			Allocation& alloc = s_Data->m_AllocationMap[memory];
			alloc.Memory = memory;
			alloc.Size = size;
			alloc.Category = desc;

			s_Data->m_AllocationStatsMap[desc].TotalAllocated += size;
			s_GlobalStats.TotalAllocated += size;
		}

		return memory;
	}

	void* Allocator::Allocate(size_t size, const char* file, int line)
	{
		if (!s_Data)
			Init();

		void* memory = malloc(size);

		{
			Allocation& alloc = s_Data->m_AllocationMap[memory];
			alloc.Memory = memory;
			alloc.Size = size;
			alloc.Category = file;

			s_Data->m_AllocationStatsMap[file].TotalAllocated += size;
			s_GlobalStats.TotalAllocated += size;
		}

		return memory;
	}

	void Allocator::FreeRaw(void* memory)
	{
		free(memory);
	}

	void Allocator::Free(void* memory)
	{
		if (s_InFree)
			return FreeRaw(memory);
		s_InFree = true;

		if (s_Data->m_AllocationMap.find(memory) != s_Data->m_AllocationMap.end())
		{
			const Allocation& alloc = s_Data->m_AllocationMap.at(memory);
			s_GlobalStats.TotalFreed += alloc.Size;
			if (alloc.Category)
				s_Data->m_AllocationStatsMap[alloc.Category].TotalFreed += alloc.Size;

			s_Data->m_AllocationMap.erase(memory);
		}
		else
		{
			//DY_CORE_WARN("MEMORY: Memmory block {0} not present in alloc map", memory);
		}
		free(memory);

		s_InFree = false;
	}

	std::map<void*, Dymatic::Memory::Allocation>& GetMemoryAllocations() { return s_Data->m_AllocationMap; }
	std::map<const char*, AllocationStats>& GetMemoryAllocationStats() { return s_Data->m_AllocationStatsMap; }

	const AllocationStats& GetAllocationStats() { return s_GlobalStats; }
}