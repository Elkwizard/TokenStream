class Token {
	constructor(content, type, position = 0, source = content) {
		this.content = content;
		this.type = type;
		this.position = position;
		this.source = source;
	}

	plus(token, type) {
		return new Token(
			this.content + token.content,
			type ?? this.type,
			this.position,
			this.source
		);
	}

	error(message) {
		const prefix = this.source.slice(0, this.position);
		const suffix = this.source.slice(this.position + this.content.length);
		const newSource = prefix + background("red", this.content) + suffix;
		const lines = newSource.split("\n");
		
		let index = 0;
		for (let i = 0; i < this.position; i++)
			if (this.source[i] === "\n") index++;
		
		const startIndex = Math.max(0, index - 1);

		const excerptLines = lines.slice(
			startIndex,
			Math.min(lines.length, index + 2)
		);

		const maxWidth = String(excerptLines.length + startIndex).length;

		const excerpt = excerptLines
			.map((line, i) => `${String(i + startIndex + 1).padStart(maxWidth)} | ${line}`)
			.join("\n");

		const bar = "=".repeat(40);
		console.log(`\n\n${bar}\n${excerpt}\n${bar}\n${message} (line ${index + 1})\n\n`);
		throw new SyntaxError(message);
		// throw new SyntaxError(message + "\n\n" + excerpt);
	}

	toString() {
		return `(${this.type.toString()}: ${color("blue", this.content)})`;
	}
}

class TokenStream {
	constructor(tokens = []) {
		this.tokens = [...tokens].reverse();
	}

	get length() {
		return this.tokens.length;
	}

	get all() {
		return [...this.tokens].reverse();
	}

	copy() {
		return new TokenStream(this.all);
	}

	prepend(token) {
		this.tokens.push(token);
	}

	has(content, index = 0) {
		if (index >= this.tokens.length)
			return false;
		
		const token = this.tokens[this.tokens.length - index - 1];
		if (typeof content === "string")
			return token.content === content;
		return token.type === content;
	}

	hasAny(...options) {
		let index = 0;
		if (typeof options[options.length - 1] === "number")
			index = options.pop();

		for (let i = 0; i < options.length; i++)
			if (this.has(options[i], index))
				return true;

		return false;
	}

	get(index = 0) {
		return this.getToken(index).content;
	}

	getToken(index = 0) {
		if (index >= this.tokens.length)
			throw new RangeError("Desired index is out of bounds");
		return this.tokens[this.tokens.length - index - 1];
	}

	skip(amount) {
		if (amount > this.tokens.length)
			throw new RangeError("Cannot skip over tokens in an empty stream");
		this.tokens.length -= amount;
	}
	
	skipAll(tok) {
		while (this.has(tok)) this.next();
	}

	remove(content) {
		if (typeof content === "string")
			this.tokens = this.tokens.filter(token => token.content !== content);
		else
			this.tokens = this.tokens.filter(token => token.type !== content);
	}

	nextToken() {
		if (!this.tokens.length)
			throw new RangeError("Cannot advance an empty stream");

		return this.tokens.pop();
	}

	next(expected) {
		if (!this.tokens.length)
			throw new RangeError("Cannot advance an empty stream");
		
		const token = this.tokens.pop();

		if (expected !== undefined) {
			if (typeof expected === "string") {
				if (token.content !== expected)
					token.error(`Unexpected token '${token.content}', expected '${String(expected)}'`);
			} else {
				if (token.type !== expected)
					token.error(`Unexpected token '${token.content}', expected token of type '${String(expected)}'`);
			}
		}

		return token.content;
	}

	optional(content) {
		if (this.has(content)) {
			this.next();
			return true;
		}

		return false;
	}

	until(tok) {
		const result = [];
		while (this.tokens.length && !this.has(tok))
			result.push(this.nextToken());
		return new TokenStream(result);
	}

	endOf(open, close) {
		const result = [];

		this.until(open);
		if (!this.tokens.length)
			throw new RangeError(`The specified boundaries "${open}${close}" don't exist`);
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

	delimitedList(parseItem, delimiter, interrupt) {
		const results = [];

		while (this.tokens.length) {
			results.push(parseItem(this));

			if (interrupt !== undefined && this.has(interrupt))
				break;

			if (this.tokens.length)
				this.next(delimiter);
		}

		return results;
	}

	toString() {
		return this.all.join(" ");
	}
}

class TokenStreamBuilder {
	constructor(source) {
		this.source = source;
		this.index = 0;
		this.tokens = [];
	}

	get stream() {
		return new TokenStream(this.tokens);
	}

	append(content, type) {
		const position = this.source.indexOf(content, this.index);
		this.index = position + content.length;
		this.tokens.push(new Token(content, type, position, this.source));
	}

	static regex(source, regexes) {
		const builder = new TokenStreamBuilder(source);

		while (source.length) {
			source = source.replace(/^\s*/, "");
			for (let i = 0; i < regexes.length; i++) {
				const [regex, type] = regexes[i];
				if (regex.test(source)) {
					builder.append(source.match(regex)[0], type);
					source = source.replace(regex, "");
					break;
				}
			}
		}

		return builder.stream;
	}
}