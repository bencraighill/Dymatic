#pragma once

#include <string>
#include <vector>
#include <stdint.h>

namespace Dymatic {

	enum class CompilerMessageType
	{
		Info,
		Compile,
		Warning,
		Error
	};

	struct CompilerMessage
	{
		CompilerMessageType Type;
		std::string Message;
		uint64_t Handle = 0;
	};

	struct CompilerResult
	{
		std::vector<CompilerMessage> Messages;
		bool Success = true;
		uint32_t WarningCount = 0;
		uint32_t ErrorCount = 0;

		inline void Add(const std::string& message, const CompilerMessageType type = CompilerMessageType::Info, uint64_t handle = 0)
		{
			Messages.push_back({ type, message, handle });

			if (type == CompilerMessageType::Error)
			{
				Success = false;
				ErrorCount++;
			}
			else if (type == CompilerMessageType::Warning)
				WarningCount++;
		}
	};

}