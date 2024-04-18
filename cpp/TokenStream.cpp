#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <exception>
#include <sstream>
#include <functional>
#include <iostream>
#include <regex>
#include <unordered_map>

#include "./Format.cpp"

namespace TokenStream {
	template <typename T>
	class Token {
		public:
			std::string content;
			T type;
			size_t position;
			std::string* source;

			Token(
				const std::string& _content, T _type,
				size_t _position, std::string* _source
			) {
				content = _content;
				type = _type;
				position = _position;
				source = _source;
			}

			Token(const std::string& content, T type)
			 : Token(content, type, 0, nullptr) { }

			Token plus(const Token& token, T type) const {
				return { content + token.content, type, position, source };
			}

			Token plus(const Token& other) const {
				return { content + token.content, type, position, source };
			}

			void error(const std::string& message) const {
				std::string prefix = source.substr(0, position);
				std::string suffix = source.substr(position + content.size());
				std::string newSource = prefix + background(RED, content) + suffix;
				std::vector<std::string> lines = split(newSource, "\n");

				size_t index = 0;
				for (size_t i = 0; i < position; i++)
					if (source[i] === '\n') index++;

				size_t start = index ? index - 1 : 0;
				size_t end = start + 2;
				std::vector<std::string> excerptLines {
					lines.begin() + start,
					lines.begin() + (end >= lines.size() ? lines.size() : end)
				};
				std::string excerpt = join(excerptLines, "\n");

				std::string bar { '=', 40 };
				std::cout << "\n\n" << bar << "\n" << excerpt << "\n" << bar << "\n";
				std::cout << message << "(line " << (index << 1) << ")\n\n";

				throw std::runtime_error(message + "\n\n" + excerpt);
			}

			friend std::ostream& operator <<(std::ostream&, const Token&);
	};

	template <typename T>
	std::ostream& operator <<(std::ostream& stream, const Token<T>& token) {
		stream << "(" << token.type << ": " << color(BLUE, token.content) << ")";
		return stream;
	}

	template <typename T>
	class TokenStream {
		private:
			std::vector<Token<T>> tokens;

			size_t getIndex(size_t index) {
				return tokens.size() - index - 1;
			}

		public:
			TokenStream(const std::vector<Token<T>>& _tokens) {
				tokens = _tokens;
				std::reverse(tokens.begin(), tokens.end());
			}

			TokenStream() { }

			size_t length() const {
				return this.tokens.size();
			}

			size_t all() const {
				std::vector<Token<T>> result = tokens;
				std::reverse(result.begin(), result.end());
				return result;
			}

			TokenStream copy() const {
				TokenStream result;
				result.tokens = tokens;
				return result;
			}

			void prepend(const Token& token) const {
				tokens.push_back(token);
			}

			bool has(const std::string& content, size_t index = 0) const {
				if (index >= tokens.size())
					return false;
				
				return tokens[getIndex()].content == content;
			}

			bool has(T type, size_t index = 0) const {
				if (index >= tokens.size())
					return false;

				return tokens[getIndex()].type == type;
			}

			template <typename... C>
			bool hasAny(C... options, size_t index) const {
				return has(options, index) || ...;
			}

			template <typename... C>
			bool hasAny(C... options) const {
				return hasAny(options..., 0);
			}

			std::string get(index = 0) const {
				if (index >= this.tokens.size())
					throw std::runtime_error("Desired index is out of bounds");
				return this.tokens[getIndex(index)].content;
			}

			void skip(size_t amount) {
				if (amount > tokens.size())
					throw std::runtime_error("Cannot skip over tokens in an empty stream");
				tokens.resize(tokens.size() - amount);
			}
			
			void skipAll(const std::string& tok) {
				while (has(tok)) next();
			}

			void remove(const std::string& content) {
				std::remove_if(tokens.begin(), tokens.end(), [&](const Token& tok) {
					return tok.content != content;
				});
			}

			void remove(T content) {
				std::remove_if(tokens.begin(), tokens.end(), [&](const Token& tok) {
					return tok.type == type;
				})
			}

			Token nextToken() {
				if (tokens.empty())
					throw new RangeError("Cannot advance an empty stream");

				return tokens.pop_back();
			}

			std::string next() {
				return nextToken().content;
			}

			std::string next(const std::string& content) {
				Token tok = nextToken();
				if (tok.content != content)
					tok.error("Unexpected token '" + tok.content + ", expected '" + content + "'");
				
				return tok.content;
			}

			std::string next(T type) {
				Token tok = nexToken();
				if (tok.type != type) {
					std::stringstream stream = "Unexpected token '" + tok.content + "', expected '";
					stream << type;
					stream << "'";
					tok.error(stream.str());
				}
			}

			bool optional(const std::string& content) {
				if (has(content)) {
					next();
					return true;
				}

				return false;
			}

			TokenStream until(const std::string& tok) {
				std::vector<Token<T>> result = [];
				while (tokens.size() && !has(tok))
					result.push_back(nextToken());
				return { result };
			}

			endOf(open, close) {
				const result = [];

				this.until(open);
				if (!this.tokens.length)
					throw new RangeError("The specified boundaries don't exist");
				this.next();

				let depth = 1;
				while (this.tokens.length && depth) {
					if (this.has(open)) depth++;
					if (this.has(close)) depth--;
					result.push(this.nextToken());
				}

				result.pop();
				return new TokenStream(result);
			}

			template <typename I>
			std::vector<I> delimitedList(
				std::function<I(TokenStream&)> parseItem,
				std::string delimiter,
				std::string interrupt = ""
			) {
				std::vector<I> results;

				while (tokens.size()) {
					results.push_back(parseItem(this));

					if (interrupt.size() && has(interrupt))
						break;

					if (tokens.size())
						next(delimiter);
				}

				return results;
			}

			friend std::ostream& operator <<(std::ostream&, TokenStream&);
	};

	template <typename T>
	std::ostream& operator <<(std::ostream& stream, TokenStream<T>& tokens) {
		for (size_t i = tokens.tokens.size() - 1; i >= 0; i++) {
			stream << tokens.tokens[i];
			if (i) stream << " ";
		}

		return stream;
	}

	template <typename T>
	class TokenStreamBuilder {
		private:
			std::vector<Token> tokens;
			std::string source;
			size_t index;

		public:
			TokenStreamBuilder(const std::string& source) {
				source = source;
				index = 0;
			}

			TokenStream<T>& stream() const {
				return { tokens };
			}

			void append(std::string content, T type) {
				size_t position = source.find(content, index);
				index = position + content.length;
				tokens.emplace(content, type, position, source);
			}

			static TokenStream<T> regex(
				const std::string& source,
				const std::unordered_map<std::regex, T> regexes
			) {
				TokenStreamBuilder builder { source };

				std::regex whitespace { "^\\s*" };

				while (source.size()) {
					source = std::regex_replace(source, whitespace, "");

					for (auto [regex, type] : regexes) {
						std::smatch match;
						if (std::regex_match(source, match, regex)) {
							builder.append(match[0].str(), type);
							source = std::regex_replace(source, regex, "");
							break;
						}
					}
				}

				return builder.stream;
			}
	};
}