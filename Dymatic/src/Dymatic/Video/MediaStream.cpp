#include "dypch.h"
#include "Dymatic/Video/MediaStream.h"

namespace Dymatic {

	MediaStream::MediaStream(const std::filesystem::path& filepath, size_t baseOffset, size_t size)
		: m_Reader(filepath), m_BaseOffset(baseOffset), m_Size(size)
	{
		// Calculate size as entire stream if not provided
		if (m_Size == 0)
		{
			m_Reader.SetStreamPositionToEnd();
			m_Size = m_Reader.GetStreamPosition();
		}

		Reset();
	}

	size_t MediaStream::GetStreamPosition()
	{
		return m_Reader.GetStreamPosition() - m_BaseOffset;
	}

	bool MediaStream::SetStreamPosition(const size_t position)
	{
		if (position > m_Size)
			return false;

		return m_Reader.SetStreamPositionInDirection(m_BaseOffset + position, FileStreamReader::Direction::Begin);
	}

	void MediaStream::Reset()
	{
		m_Reader.SetStreamPosition(m_BaseOffset);
	}

}
