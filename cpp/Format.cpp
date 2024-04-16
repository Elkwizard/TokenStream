#pragma once

#include <string>

namespace TokenStream {
	enum Color {
		RED = 1, GREEN = 2, YELLOW = 3, BLUE = 4, MAGENTA = 5, CYAN = 6,
		WHITE = 67, LIGHT_GRAY = 7, DARK_GRAY = 60, BLACK = 0,
		LIGHT_RED = 61, LIGHT_GREEN = 62, LIGHT_YELLOW = 63,
		LIGHT_BLUE = 64, LIGHT_MAGENTA = 65, LIGHT_CYAN = 66
	};
	
	int FOREGROUND_OFFSET = 30;
	int BACKGROUND_OFFSET = 40;
	
	std::string color(Color color, const std::string& text) {
		std::string code = std::to_string(color + FOREGROUND_OFFSET);
		return "\x1b[" + code + "m" + text + "\x1b[0m";
	}

	std::string background(Color color, const std::string& text) {
		std::string code = std::to_string(color + BACKGROUND_OFFSET);
		return "\x1b[" + code + "m" + text + "\x1b[0m";
	}

	std::string normalizeLinebreaks(const std::string& str) {
		std::string result = "";
		for (const char& c : str)
			if (c != '\r') result += c;
		return result;
	}

	std::vector<std::string> split(std::string str, const std::string& delim) {
		std::vector<std::string> result { };
		size_t nextIndex; 
		while (true) {
			nextIndex = str.find(delim);
			if (nextIndex == -1) break;
			result.push_back(str.substr(0, nextIndex));
			str = str.substr(nextIndex + 1);
		}
		result.push_back(str);
		return result;
	}

	std::string join(const std::vector<std::string>& segments, const std::string& delim) {
		if (segments.empty())
			return std::string();
		
		std::string result = segments[0];
		for (int i = 1; i < segments.size(); i++)
			result += delim + segments[i];
		return result;
	}
	
	std::string indent(const std::string& str) {
		std::string_view view = str;
		std::string result = "";
		while (true) {
			size_t index = view.find('\n');
			if (index == -1) break;
			result += "\t";
			result += view.substr(0, index);
			result += "\n";
			view = view.substr(index + 1);
		}

		if (view.empty())
			return result;

		result += "\t";
		result += view;

		return result;
	}
}