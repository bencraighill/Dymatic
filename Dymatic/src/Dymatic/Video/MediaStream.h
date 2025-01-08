#pragma once
#include "Dymatic/Asset/Asset.h"

#include "Dymatic/Core/FileStream.h"

namespace Dymatic {

	class MediaStream : public Asset
	{
	public:
		MediaStream(const std::filesystem::path& filepath, size_t baseOffset, size_t size);

		inline FileStreamReader& GetReader() { return m_Reader; }

		size_t GetStreamPosition();
		bool SetStreamPosition(const size_t position);

		inline size_t GetStreamBaseOffset() const { return m_BaseOffset; }
		inline size_t GetStreamSize() const { return m_Size; }

		void Reset();
	private:
		FileStreamReader m_Reader;

		// Required for packaged runtime stream
		size_t m_BaseOffset;
		size_t m_Size;
	};

}
