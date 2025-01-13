#include "dypch.h"
#include "Dymatic/Editor/CompilerWriter.h"

namespace Dymatic::Editor {

	void CompilerWriter::NewLine()
	{
		m_Buffer += '\n';
		m_IndentedCurrentLine = false;
	}

	void CompilerWriter::Write(const std::string& text)
	{
		// Indent the line as required
		if (!m_IndentedCurrentLine)
		{
			m_IndentedCurrentLine = true;

			for (uint32_t i = 0; i < m_IndentationLevel; i++)
				m_Buffer += '\t';
		}

		m_Buffer += text;
	}

	void CompilerWriter::WriteLine(const std::string& line)
	{
		Write(line);
		NewLine();
	}

	void CompilerWriter::WriteLineUnidented(const std::string& line)
	{
		m_Buffer += line + "\n";
	}

	void CompilerWriter::Indent(uint32_t levels)
	{
		m_IndentationLevel += levels;
	}

	void CompilerWriter::Unident(uint32_t levels)
	{
		m_IndentationLevel -= levels;
	}

	void CompilerWriter::OpenScope()
	{
		WriteLine("{");
		Indent();
	}

	void CompilerWriter::CloseScope(const bool semicolon)
	{
		Unident();
		WriteLine(semicolon ? "};" : "}");
	}

}