#include "returnStatement.h"
#include "../interpreter/interpreter.h"

bool ReturnStatement::ShouldParse(ts::Engine* engine) {
	return engine->tokenizer->peekToken().type == RETURN;
}

ReturnStatement* ReturnStatement::Parse(Component* parent, ts::Engine* engine) {
	ReturnStatement* output = new ReturnStatement(engine);
	output->parent = parent;
	output->token = engine->parser->expectToken(RETURN);

	// parse an expression
	if(engine->tokenizer->peekToken().type != SEMICOLON) {
		if(!Component::ShouldParse(output, engine)) {
			engine->parser->error("need valid expression for 'return' statement");
		}
		output->operation = Component::Parse(output, engine);

		engine->parser->expectToken(SEMICOLON);
	}
	else {
		engine->tokenizer->getToken(); // absorb semicolon
	}

	return output;
}

string ReturnStatement::print() {
	if(this->operation != nullptr) {
		return "return " + this->operation->print() + ";";
	}
	else {
		return "return;";
	}
}

string ReturnStatement::printJSON() {
	if(this->operation != nullptr) {
		return "{\"type\":\"RETURN_STATEMENT\",\"operation\":" + this->operation->printJSON() + "}";
	}
	else {
		return "{\"type\":\"RETURN_STATEMENT\"}";
	}
}

ts::InstructionReturn ReturnStatement::compile(ts::Engine* engine, ts::CompilationContext context) {
	ts::InstructionReturn output;

	// add a return statement that exits out from our function
	ts::Instruction* returnInstruction = new ts::Instruction(
		engine,
		this->getCharacterNumber(),
		this->getLineNumber()
	);
	returnInstruction->type = ts::instruction::RETURN_NO_VALUE;

	if(this->operation != nullptr) {
		ts::InstructionReturn operation = this->operation->compile(engine, context);
		
		if(
			this->operation->getType() == ACCESS_STATEMENT
			|| this->operation->getType() == PARENT_STATEMENT
			|| this->operation->getType() == NUMBER_LITERAL
			|| this->operation->getType() == STRING_LITERAL
			|| this->operation->getType() == BOOLEAN_LITERAL
		) { // TODO: remove the need to do this
			operation.last->pushType = ts::instruction::RETURN_REGISTER;
			returnInstruction->type = ts::instruction::RETURN;
		}
		else {
			returnInstruction->type = ts::instruction::MOVE_THEN_RETURN;
		}

		output.add(operation);
	}

	output.add(returnInstruction);
	
	return output;
}
