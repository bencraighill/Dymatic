#include "dypch.h"
#include "Dymatic/Core/FileStream.h"

namespace Dymatic {

	FileStreamWriter::FileStreamWriter(const std::filesystem::path& path)
	{
		m_FileStream = std::ofstream(path, std::ios::binary | std::ios::ate);
	}

	bool FileStreamWriter::IsStreamGood() const
	{
		return m_FileStream.good();
	}

	uint64_t FileStreamWriter::GetStreamPosition()
	{
		return m_FileStream.tellp();
	}

	void FileStreamWriter::SetStreamPosition(uint64_t position)
	{
		m_FileStream.seekp(position);
	}

	bool FileStreamWriter::WriteData(const char* data, size_t size)
	{
		return (bool)m_FileStream.write(data, size);
	}

	FileStreamReader::FileStreamReader(const std::filesystem::path& path)
	{
		// Note: Unlike the FileStreamWriter, the reader does not use std::ios::ate as we likely want to read from the start
		m_FileStream = std::ifstream(path, std::ios::binary);
	}

	bool FileStreamReader::IsStreamGood() const
	{
		return m_FileStream.good();
	}

	uint64_t FileStreamReader::GetStreamPosition()
	{
		return m_FileStream.tellg();
	}

	void FileStreamReader::SetStreamPosition(uint64_t position)
	{
		m_FileStream.seekg(position);
	}

	bool FileStreamReader::ReadData(char* destination, size_t size)
	{
		return (bool)m_FileStream.read(destination, size);
	}

	bool FileStreamReader::IsEOF() const
	{
		return m_FileStream.eof();
	}

	uint64_t FileStreamReader::ReadDataLength(char* destination, size_t size)
	{
		m_FileStream.read(destination, size);
		return m_FileStream.gcount();
	}

	bool FileStreamReader::SetStreamPositionInDirection(const int64_t offset, const Direction direction)
	{
		// Determine standard library seek direction
		std::ios_base::seekdir seekDirection;
		
		switch (direction)
		{
		case Direction::Begin:
			seekDirection = std::ios::beg;
			break;
		case Direction::Current:
			seekDirection = std::ios::cur;
			break;
		case Direction::End:
			seekDirection = std::ios::end;
			break;
		default:
			return false;
		}

		// Clear failure bits and seek
		m_FileStream.clear();
		m_FileStream.seekg(offset, seekDirection);
		return !m_FileStream.fail();
	}

	void FileStreamReader::SetStreamPositionToEnd()
	{
		m_FileStream.seekg(0, std::ios::end);
	}

}