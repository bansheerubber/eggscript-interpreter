#include "parser.h"

#include "../components/classDeclaration.h"
#include "../components/sourceFile.h"

Parser::Parser(ts::Engine* engine, ParsedArguments args) {
	this->engine = engine;
	this->args = args;
}

Parser::~Parser() {
	for(Component* component: this->components) {
		delete component;
	}
}

void Parser::startParse() {
	for(Component* component: this->components) {
		delete component;
	}

	this->components.clear();
	
	this->sourceFile = new SourceFile(this->engine);
	Component::ParseBody(this->sourceFile, this->engine);

	for(ClassDeclaration* declaration: this->sourceFile->classDeclarations) {
		ts::MethodTree* tree = this->engine->getNamespace(declaration->className);
		if(tree == nullptr) {
			tree = this->engine->createMethodTreeFromNamespace(declaration->className);
		}

		ts::MethodTree* inheritedTree = this->engine->getNamespace(declaration->inheritedName);
		if(declaration->inheritedName != "" && inheritedTree != nullptr) {
			tree->addParent(inheritedTree);
		}
		else if(declaration->inheritedName != "" && inheritedTree == nullptr) {
			this->error("could not inherit non-declared class '%s'", declaration->inheritedName.c_str());
		}
	}

	if(this->args.arguments["json"] != "") {
		cout << this->printJSON() << endl;
	}
}

Token Parser::expectToken(TokenType type1, TokenType type2, TokenType type3, TokenType type4, TokenType type5) {
	bool foundType = false;
	Token token = this->engine->tokenizer->getToken();
	
	if(token.type == type1) {
		foundType = true;
	}
	else if(token.type == type2 && type2 != INVALID) {
		foundType = true;
	}
	else if(token.type == type3 && type3 != INVALID) {
		foundType = true;
	}
	else if(token.type == type4 && type4 != INVALID) {
		foundType = true;
	}
	else if(token.type == type5 && type5 != INVALID) {
		foundType = true;
	}

	if(!foundType) {
		this->error("unexpected token '%s' found, wanted %s", token.lexeme.c_str(), this->engine->tokenizer->typeToName(type1));
	}

	return token;
}

SourceFile* Parser::getSourceFile() {
	return this->sourceFile;
}

string Parser::printJSON() {
	return this->sourceFile->printJSON();
}
