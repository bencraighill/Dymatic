#pragma once

#include "Dymatic/Core/StreamWriter.h"
#include "Dymatic/Core/StreamReader.h"

#include <filesystem>
#include <fstream>

namespace Dymatic {

	class FileStreamWriter : public StreamWriter
	{
	public:
		FileStreamWriter(const std::filesystem::path& path);
		FileStreamWriter(const FileStreamWriter&) = delete;
		virtual ~FileStreamWriter() override = default;

		bool IsStreamGood() const final;
		uint64_t GetStreamPosition() override;
		void SetStreamPosition(uint64_t position) override;
		bool WriteData(const char* data, size_t size) final;
	private:
		std::ofstream m_FileStream;
	};

	class FileStreamReader : public StreamReader
	{
	public:
		enum class Direction
		{
			Begin,
			Current,
			End
		};

	public:
		FileStreamReader(const std::filesystem::path& path);
		FileStreamReader(const FileStreamReader&) = delete;
		virtual ~FileStreamReader() override = default;

		bool IsStreamGood() const final;
		uint64_t GetStreamPosition() override;
		void SetStreamPosition(uint64_t position) override;
		bool ReadData(char* destination, size_t size) override;

		bool IsEOF() const;
		uint64_t ReadDataLength(char* destination, size_t size);
		bool SetStreamPositionInDirection(const int64_t offset, const Direction direction);
		void SetStreamPositionToEnd();
	private:
		std::ifstream m_FileStream;
	};

}