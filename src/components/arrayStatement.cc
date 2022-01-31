#include "arrayStatement.h"
#include "../interpreter/interpreter.h"

bool ArrayStatement::ShouldParse(ts::Engine* engine) {
	return engine->tokenizer->peekToken().type == LEFT_BRACE;
}

ArrayStatement* ArrayStatement::Parse(Component* parent, ts::Engine* engine) {
	ArrayStatement* output = new ArrayStatement(engine);
	output->parent = parent;
	
	engine->parser->expectToken(LEFT_BRACE);
	output->component = Component::Parse(output, engine);
	engine->parser->expectToken(RIGHT_BRACE);
	
	return output;
}

string ArrayStatement::print() {
	return "[" + this->component->print() + "]";
}

string ArrayStatement::printJSON() {
	return "{\"type\":\"ARRAY_STATEMENT\",\"component\":" + this->component->printJSON() + "]}";
}

ts::InstructionReturn ArrayStatement::compile(ts::Engine* engine, ts::CompilationContext context) {
	ts::InstructionReturn output;
	output.add(this->component->compile(engine, context));

	ts::Instruction* instruction = new ts::Instruction(
		engine,
		this->getCharacterNumber(),
		this->getLineNumber()
	);
	instruction->type = ts::instruction::ARRAY_ACCESS;
	output.add(instruction);

	return output;
}
