#pragma once

#include <string>

namespace Dymatic::Editor {

	class CompilerWriter
	{
	public:
		void NewLine();
		void Write(const std::string& text);
		void WriteLine(const std::string& line);
		void WriteLineUnidented(const std::string& line);

		void Indent(uint32_t levels = 1);
		void Unident(uint32_t levels = 1);
		inline uint32_t GetIdentationLevel() const { return m_IndentationLevel; }

		void OpenScope();
		void CloseScope(const bool semicolon = false);

		inline const std::string& Contents() const { return m_Buffer; }

	private:
		std::string m_Buffer;
		uint32_t m_IndentationLevel = 0;
		bool m_IndentedCurrentLine = false;
	};

}