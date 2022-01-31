#include "emptyLiteral.h"
#include "../interpreter/interpreter.h"

using namespace ts;

bool EmptyLiteral::ShouldParse(ts::Engine* engine) {
	return engine->tokenizer->peekToken().type == EMPTY;
}

EmptyLiteral* EmptyLiteral::Parse(Component* parent, ts::Engine* engine) {
	EmptyLiteral* output = new EmptyLiteral(engine);
	output->parent = parent;
	output->token = engine->parser->expectToken(EMPTY);
	return output;
}

string EmptyLiteral::print() {
	return "null";
}

string EmptyLiteral::printJSON() {
	return "{\"type\":\"EMPTY_LITERAL\"}";
}

InstructionReturn EmptyLiteral::compile(ts::Engine* engine, ts::CompilationContext context) {
	Instruction* instruction = new Instruction(
		engine,
		this->getCharacterNumber(),
		this->getLineNumber()
	);
	instruction->type = instruction::PUSH;
	instruction->push.entry = ts::Entry();
	instruction->push.entry.type = entry::EMPTY;
	return InstructionReturn(instruction, instruction);
}
