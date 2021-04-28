#include "tokenizer.h"
#include "../io.h"

Tokenizer::Tokenizer(string piped, bool isPiped, ParsedArguments args) {
	this->handleArgs(args);
	
	this->contentSize = piped.size();
	this->contents = new char[this->contentSize];
	strcpy(this->contents, piped.c_str());

	this->tokenize();
}

Tokenizer::Tokenizer(string fileName, ParsedArguments args) {
	this->handleArgs(args);
	
	// read the file
	ifstream file = ifstream(fileName);
	this->fileName = fileName;

	if((file.rdstate() & ifstream::failbit) != 0) {
		printError("failed to read file %s\n", fileName.c_str());
	}

	// TODO this is probably insecure
	file.seekg(0, ios::end);
	contentSize = file.tellg();
	this->contents = new char[this->contentSize];
	file.seekg(0);
	file.read(this->contents, this->contentSize);
	file.close();

	this->tokenize();
}

void Tokenizer::handleArgs(ParsedArguments args) {
	this->args = args;
	if(this->args.arguments["no-warnings"] != "") {
		this->showWarnings = false;
	}

	if(this->args.arguments["piped"] != "") {
		this->showWarnings = false;
	}
}

void Tokenizer::tokenize() {
	// initialize keyword tables
	this->initializeKeywords();

	char character;
	while(!this->isFileEOF() && (character = this->getChar())) {
		bool failedKeyword = false;
		
		// read a number
		if(this->isNumber(character)) {
			this->prevChar();
			this->tokens.push_back(this->readNumber());
		}
		// handle modulus differently than normal arguments
		else if(character == '%') {
			// if the next character is the start of a valid variable name, then read a variable.
			// note: this guarentees that variables are at least one character long, which is good enough
			// for declaring a variable
			char nextCharacter = this->getChar();
			if(this->isValidVariableFirstChar(nextCharacter)) {
				this->prevChar(); // give back first character of variable name
				this->prevChar(); // give back modulus
				this->tokens.push_back(this->readLocalVariable());
			}
			// we found modulus assign
			else if(nextCharacter == '=') {
				this->tokens.push_back(Token {
					lexeme: "%=",
					type: MODULUS_ASSIGN,
					lineNumber: this->getLineNumber(),
					characterNumber: this->getCharacterNumber(),
				});
			}
			// we just found a normal modulus token
			else {
				this->prevChar(); // give back first character of variable name
				this->tokens.push_back(Token {
					lexeme: "%",
					type: MODULUS,
					lineNumber: this->getLineNumber(),
					characterNumber: this->getCharacterNumber(),
				});
			}
		}
		// handle string/tagged string
		else if(character == '"' || character == '\'') {
			this->prevChar();
			this->tokens.push_back(this->readStringLiteral(character == '\''));
		}
		// read a keyword
		else if(!this->freezeKeywordTest && this->isPartialKeyword(character)) {
			char nextCharacter = this->getChar();
			this->prevChar();
			if(character == '$' && this->isValidVariableFirstChar(nextCharacter)) { // handle global variable
				this->prevChar();
				this->tokens.push_back(this->readGlobalVariable());
			}
			else if(character == '/' && nextCharacter == '/') { // handle comment
				this->prevChar();
				this->readComment();
			}
			else { // handle keyword
				this->prevChar();
				Token keyword = this->readKeyword();

				if(keyword.type != INVALID) {
					this->tokens.push_back(keyword);
				}
				else {
					failedKeyword = true;
				}	
			}
		}
		// member chain parsing
		else if(character == '.') {
			this->tokens.push_back(this->readMemberChain());
		}
		// read a symbol
		else if(this->isValidVariableFirstChar(character)) {
			this->prevChar();
			this->tokens.push_back(this->readSymbol());
		}
		else if(!this->isWhitespace(character)) {
			this->error("unexpected character '%c'", character);
		}

		if(!failedKeyword) {
			this->freezeKeywordTest = false;
		}
	}

	// print the tokens
	/*for(Token token: this->tokens) {
		this->printToken(token);
	}*/
}

Token& Tokenizer::getToken() {
	if(this->tokenIndex >= this->tokens.size()) {
		return this->emptyToken;
	}
	return this->tokens[this->tokenIndex++];
}

Token& Tokenizer::unGetToken() {
	if(this->tokenIndex <= 0) {
		return this->emptyToken;
	}
	return this->tokens[--this->tokenIndex];
}

Token& Tokenizer::peekToken(int offset) {
	if(this->tokenIndex + offset >= this->tokens.size()) {
		return this->emptyToken;
	}

	if(this->tokenIndex + offset < 0) {
		printError("token index is somehow below 0");
	}

	return this->tokens[this->tokenIndex + offset];
}

bool Tokenizer::eof() {
	if(this->tokenIndex >= this->tokens.size()) {
		return true;
	}
	return false;
}