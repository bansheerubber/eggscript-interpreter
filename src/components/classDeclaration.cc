#include "classDeclaration.h"

#include "sourceFile.h"

bool ClassDeclaration::ShouldParse(ts::Engine* engine) {
	return engine->tokenizer->peekToken().type == CLASS && engine->tokenizer->peekToken(1).type == SYMBOL;
}

ClassDeclaration* ClassDeclaration::Parse(Component* parent, ts::Engine* engine) {
	if(parent->getType() != SOURCE_FILE) {
		engine->parser->error("cannot declare scoped classes");
	}
	
	ClassDeclaration* output = new ClassDeclaration(engine);

	engine->parser->expectToken(CLASS);
	
	Token symbol = engine->parser->expectToken(SYMBOL);
	output->className = symbol.lexeme;

	if(engine->tokenizer->peekToken().type == INHERITS) {
		engine->parser->expectToken(INHERITS);

		Token symbol = engine->parser->expectToken(SYMBOL);
		output->inheritedName = symbol.lexeme;
	}

	((SourceFile*)parent)->classDeclarations.push_back(output);

	return output;
}

string ClassDeclaration::print() {
	return "";
}

string ClassDeclaration::printJSON() {
	return "";
}

ts::InstructionReturn ClassDeclaration::compile(ts::Engine* engine, ts::CompilationContext context) {
	return {};
}
