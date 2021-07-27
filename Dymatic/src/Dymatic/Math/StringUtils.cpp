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
}