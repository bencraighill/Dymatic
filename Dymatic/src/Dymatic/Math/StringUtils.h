#pragma once
#include <Dymatic.h>
#include "Dymatic/Core/Base.h"

namespace Dymatic::String {

	std::string FloatToString(float Float);
	int Find_nth_of(const std::string& str, const std::string& find, int nth);
	void ReplaceAll(std::string& str, const std::string& from, const std::string& to);
	std::string GetNextNameWithIndex(std::vector<std::string>& vector, std::string prefix);
	void EraseAllOfCharacter(std::string& string, char character);
	void SplitStringByDelimiter(const std::string& string, std::vector<std::string>& seglist, char delimiter);
	void ToLower(std::string& inString);
}
