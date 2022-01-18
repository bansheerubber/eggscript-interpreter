#include "namespaceStatement.h"
#include "../interpreter/interpreter.h"

#include "../util/allocateString.h"
#include "../util/cloneString.h"

bool NamespaceStatement::ShouldParse(ts::Engine* engine) {
	return (
		engine->tokenizer->peekToken().type == PARENT
		|| (
			engine->tokenizer->peekToken().type == SYMBOL
			&& engine->tokenizer->peekToken(1).type == NAMESPACE
		)
	);
}

NamespaceStatement* NamespaceStatement::Parse(Component* parent, ts::Engine* engine) {
	NamespaceStatement* output = new NamespaceStatement(engine);
	output->parent = parent;

	if(engine->tokenizer->peekToken().type == PARENT) {
		output->parentToken = engine->parser->expectToken(PARENT);
	}
	else {
		// parse a symbol
		if(!Symbol::ShouldParse(engine)) {
			engine->parser->error("invalid namespace operation");
		}
		output->name = Symbol::Parse(output, engine);
	}

	engine->parser->expectToken(NAMESPACE);

	// parse a symbol
	if(!Symbol::ShouldParse(engine)) {
		engine->parser->error("invalid namespace operation");
	}
	output->operation = Symbol::Parse(output, engine);

	// parse call statement
	if(!CallStatement::ShouldParse(engine)) {
		engine->parser->error("invalid namespace operation");
	}
	output->call = CallStatement::Parse(parent, engine);

	return output;
}

string NamespaceStatement::print() {
	string output = "Parent::" + this->operation->print() + this->call->print();
	if(this->name != nullptr) {
		output = this->name->print() + "::" + this->operation->print() + this->call->print();
	}

	if(this->parent->requiresSemicolon(this)) {
		output += ";";
	}

	return output;
}

string NamespaceStatement::printJSON() {
	if(this->name != nullptr) {
		return "{\"type\":\"NAMESPACE_STATEMENT\",\"name\":" + this->name->printJSON() + ",\"operation\":" + this->operation->printJSON() + ",\"call\":" + this->call->printJSON() + "}";
	}
	else {
		return "{\"type\":\"NAMESPACE_STATEMENT\",\"name\":\"Parent\",\"operation\":" + this->operation->printJSON() + ",\"call\":" + this->call->printJSON() + "}";
	}
}

ts::InstructionReturn NamespaceStatement::compile(ts::Engine* engine, ts::CompilationContext context) {
	ts::InstructionReturn output;
	
	if(this->name != nullptr) {
		output.add(this->call->compile(engine, context)); // push arguments

		// push the amount of arguments we just found
		ts::Instruction* instruction = new ts::Instruction(
			engine,
			this->getCharacterNumber(),
			this->getLineNumber()
		);
		instruction->type = ts::instruction::PUSH;
		instruction->push.entry = ts::Entry();
		instruction->push.entry.type = ts::entry::NUMBER;
		instruction->push.entry.setNumber(((CallStatement*)this->call)->getElementCount());
		output.add(instruction);

		// build call instruction
		ts::Instruction* callFunction = new ts::Instruction(
			engine,
			this->getCharacterNumber(),
			this->getLineNumber()
		);
		callFunction->type = ts::instruction::CALL_NAMESPACE_FUNCTION_UNLINKED;
		callFunction->callNamespaceFunction.name = cloneString(this->operation->print().c_str());
		callFunction->callNamespaceFunction.nameSpace = cloneString(this->name->print().c_str());
		callFunction->callNamespaceFunction.cachedEntry = nullptr;
		output.add(callFunction);

		engine->addUnlinkedInstruction(callFunction);

		if(this->parent->requiresSemicolon(this)) { // if we do not assign/need the value of the function, just pop it
			ts::Instruction* pop = new ts::Instruction(
				engine,
				this->getCharacterNumber(),
				this->getLineNumber()
			);
			pop->type = ts::instruction::POP;
			output.add(pop);
		}
	}
	else {
		output.add(this->call->compile(engine, context)); // push arguments

		// push the amount of arguments we just found
		ts::Instruction* instruction = new ts::Instruction(
			engine,
			this->getCharacterNumber(),
			this->getLineNumber()
		);
		instruction->type = ts::instruction::PUSH;
		instruction->push.entry = ts::Entry();
		instruction->push.entry.type = ts::entry::NUMBER;
		instruction->push.entry.setNumber(((CallStatement*)this->call)->getElementCount());
		output.add(instruction);

		// build call instruction
		ts::Instruction* callParent = new ts::Instruction(
			engine,
			this->getCharacterNumber(),
			this->getLineNumber()
		);
		callParent->type = ts::instruction::CALL_PARENT;
		output.add(callParent);

		if(this->parent->requiresSemicolon(this)) { // if we do not assign/need the value of the function, just pop it
			ts::Instruction* pop = new ts::Instruction(
				engine,
				this->getCharacterNumber(),
				this->getLineNumber()
			);
			pop->type = ts::instruction::POP;
			output.add(pop);
		}
	}
	
	return output;
}
