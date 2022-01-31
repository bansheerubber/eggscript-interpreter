#include "numberLiteral.h"
#include "../interpreter/interpreter.h"

using namespace ts;

bool NumberLiteral::ShouldParse(ts::Engine* engine) {
	return engine->tokenizer->peekToken().type == NUMBER;
}

NumberLiteral* NumberLiteral::Parse(Component* parent, ts::Engine* engine) {
	NumberLiteral* output = new NumberLiteral(engine);
	output->parent = parent;
	output->token = engine->tokenizer->getToken();
	output->number = output->token.lexeme;
	return output;
}

string NumberLiteral::print() {
	return this->number;
}

string NumberLiteral::printJSON() {
	return "{\"type\":\"NUMBER_LITERAL\",\"value\":\"" + this->number + "\"}";
}

InstructionReturn NumberLiteral::compile(ts::Engine* engine, ts::CompilationContext context) {
	Instruction* instruction = new Instruction(
		engine,
		this->getCharacterNumber(),
		this->getLineNumber()
	);
	instruction->type = instruction::PUSH;
	instruction->push.entry = ts::Entry();
	instruction->push.entry.type = entry::NUMBER;
	instruction->push.entry.setNumber(stod(this->number));
	return InstructionReturn(instruction, instruction);
}

double NumberLiteral::getNumber() {
	return stod(this->number);
}
