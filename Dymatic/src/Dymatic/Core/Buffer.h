#pragma once

#include <stdint.h>
#include <cstring>

#include "Dymatic/Core/Base.h"

namespace Dymatic {

	struct Buffer
	{
		uint8_t* Data;
		uint64_t Size;

		Buffer()
			: Data(nullptr), Size(0)
		{}

		Buffer(uint64_t size)
			: Data(nullptr)
		{
			Allocate(size);
		}

		Buffer(const void* data, uint64_t size)
			: Data((uint8_t*)data), Size(size)
		{}

		Buffer(const Buffer& other, uint64_t size)
			: Data(other.Data), Size(size)
		{}

		Buffer(const Buffer&) = default;

		void Copy(const void* data, size_t size) const
		{
			memcpy(Data, data, size);
		}

		void Copy(const Buffer& other) const
		{
			memcpy(Data, other.Data, other.Size);
		}

		Buffer Copy() const
		{
			Buffer buffer = Buffer(Size);
			memcpy(buffer.Data, Data, Size);

			return buffer;
		}

		void Set(Buffer other)
		{
			Allocate(other.Size);
			memcpy(Data, other.Data, other.Size);
		}

		void Allocate(uint64_t size)
		{
			Release();

			Data = new uint8_t[size];
			Size = size;
		}

		void Release()
		{
			if (!Data)
				return;

			delete[] Data;
			Data = nullptr;
			Size = 0;
		}

		void ZeroInitialize()
		{
			if (Data)
				memset(Data, 0, Size);
		}

		template<typename T>
		T& Read(uint64_t offset = 0)
		{
			return *(T*)((uint32_t*)Data + offset);
		}

		template<typename T>
		const T& Read(uint64_t offset = 0) const
		{
			return *(T*)((uint32_t*)Data + offset);
		}

		uint8_t* ReadBytes(uint64_t size, uint64_t offset) const
		{
			uint8_t* buffer = new uint8_t[size];
			memcpy(buffer, (uint8_t*)Data + offset, size);
			return buffer;
		}

		void Write(const void* data, uint64_t size, uint64_t offset = 0)
		{
			memcpy((uint8_t*)Data + offset, data, size);
		}

		operator bool() const
		{
			return (bool)Data;
		}

		uint8_t& operator[](int index)
		{
			return ((uint8_t*)Data)[index];
		}

		uint8_t operator[](int index) const
		{
			return ((uint8_t*)Data)[index];
		}

		template<typename T>
		T* As() const
		{
			return (T*)Data;
		}

		template<typename T>
		T Get(uint64_t offset) const
		{
			return As<T>()[offset];
		}

		template<typename T>
		void Set(uint64_t offset, T value) const
		{
			As<T>()[offset] = value;
		}

		inline uint64_t GetSize() const { return Size; }
	};

	struct ScopedBuffer : public Buffer
	{
		static Ref<ScopedBuffer> Create(uint64_t size) { return CreateRef<ScopedBuffer>(size); }

		ScopedBuffer() : Buffer() {}
		ScopedBuffer(const Buffer& buffer) : Buffer(buffer) {}
		ScopedBuffer(uint64_t size) : Buffer(size) {}
		ScopedBuffer(const void* data, uint64_t size) : Buffer(data, size) {}

		~ScopedBuffer()
		{
			Release();
		}
	};


}