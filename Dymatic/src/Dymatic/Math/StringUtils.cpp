#include "dypch.h"
#include "StringUtils.h"

namespace Dymatic::String {

	std::string FloatToString(float Float)
	{
		std::string text = std::to_string(Float);
		int check = text.find_last_not_of("0");
		int checkStop = text.find_first_of(".");
		int finalCheck = check <= checkStop ? check : check + 1;
		return (text.substr(0, finalCheck));
	}

	int Find_nth_of(const std::string& str, const std::string& find, int nth)
	{
		size_t  pos = 0;
		int     cnt = 0;

		while (cnt != nth)
		{
			pos += 1;
			pos = str.find(find, pos);
			if (pos == std::string::npos)
				return -1;
			cnt++;
		}
		return pos;
	}

	void ReplaceAll(std::string& str, const std::string& from, const std::string& to) {
		size_t start_pos = 0;
		while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
			str.replace(start_pos, from.length(), to);
			start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
		}
	}

	std::string GetNextNameWithIndex(std::vector<std::string>& vector, std::string prefix)
	{
		int index = 0;
		std::string name;
		while (true)
		{
			bool found = false;
			name = prefix + (index > 0 ? ("_" + std::string(index < 10 ? "0" : "") + std::to_string(index)) : "");
			for (auto& element : vector)
				if (element == name)
					found = true;
			if (!found)
				break;
			index++;
		}

		return name;
	}

	void EraseAllOfCharacter(std::string& string, char character)
	{
		while (string.find('"') != std::string::npos) string.erase(std::find(string.begin(), string.end(), character));
	}

	void SplitStringByDelimiter(const std::string& string, std::vector<std::string>& seglist, char delimiter)
	{
		std::string segment;
		for (auto& character : string)
			if (character == delimiter)
			{
				seglist.push_back(segment);
				segment.clear();
			}
			else
				segment += character;
		seglist.push_back(segment);
	}

	void ToLower(std::string& inString)
	{
		transform(inString.begin(), inString.end(), inString.begin(), ::tolower);
	}
}