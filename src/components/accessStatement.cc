#include "accessStatement.h"
#include "../interpreter/interpreter.h"

#include "../util/allocateString.h"
#include "arrayStatement.h"
#include "callStatement.h"
#include "../util/stringToChars.h"

bool AccessStatement::DatablockAsSymbol = false;

bool AccessStatement::ShouldParse(ts::Engine* engine, bool useKeyword) {
	Token token = engine->tokenizer->peekToken();
	return (
		token.type == LOCAL_VARIABLE
		|| token.type == GLOBAL_VARIABLE
		|| ( // handle functions
			token.type == SYMBOL
			&& engine->tokenizer->peekToken(1).type == LEFT_PARENTHESIS
		)
		|| (useKeyword && engine->tokenizer->isAlphabeticalKeyword(token.type))
	);
}

AccessStatement* AccessStatement::Parse(
	Component* firstValue,
	Component* parent,
	ts::Engine* engine,
	bool useKeyword
) {
	AccessStatement* output = new AccessStatement(engine);
	output->parent = parent;

	Token &token = engine->tokenizer->peekToken();
	if(firstValue != nullptr) {
		output->elements.push_back((AccessElement){
			component: firstValue,
		});
		firstValue->setParent(output);
	}
	else if(
		token.type == LOCAL_VARIABLE
		|| token.type == GLOBAL_VARIABLE
		|| token.type == SYMBOL
		|| (useKeyword && engine->tokenizer->isAlphabeticalKeyword(token.type))
	) { // we read in a single variable at the start of the chain
		output->elements.push_back((AccessElement){
			token: engine->tokenizer->getToken(),
		});
	}
	
	int expectingArrayOrCall = 1; // doesn't quite work for something like %test();
	while(!engine->tokenizer->eof()) {
		token = engine->tokenizer->peekToken();
		if(ArrayStatement::ShouldParse(engine)) {
			if(expectingArrayOrCall > 0) {
				output->elements.push_back((AccessElement){
					isArray: true,
					component: ArrayStatement::Parse(output, engine),
				});
				expectingArrayOrCall = 2;
			}
			else {
				engine->parser->error("was not expecting array access");
			}
		}
		else if(CallStatement::ShouldParse(engine)) {
			if(expectingArrayOrCall == 1) {
				output->elements.push_back((AccessElement){
					isCall: true,
					component: CallStatement::Parse(output, engine),
				});
				expectingArrayOrCall = 0;
			}
			else {
				engine->parser->error("was not expecting call");
			}
		}
		else if(token.type == MEMBER_CHAIN) {
			engine->tokenizer->getToken(); // absorb the token we peeked
			output->elements.push_back((AccessElement){
				token: token,
			});
			expectingArrayOrCall = 1;
		}
		else {
			break;
		}
	}
	return output;
}

string AccessStatement::print() {
	string output;
	for(AccessElement element: this->elements) {
		if(element.token.type == LOCAL_VARIABLE) {
			output += "%" + element.token.lexeme;
		}
		else if(element.token.type == GLOBAL_VARIABLE) {
			output += "$" + element.token.lexeme;
		}
		else if(element.component != nullptr) {
			output += element.component->print();
		}
		else if(element.token.type) {
			output += element.token.lexeme;
		}
	}
	
	if(this->parent->requiresSemicolon(this)) {
		output += ";";
	}

	return output;
}

string AccessStatement::printJSON() {
	string output = "{\"type\":\"ACCESS_STATEMENT\",";
	if(this->elements[0].token.type == LOCAL_VARIABLE) {
		output += "\"variableType\":\"LOCAL_VARIABLE\",";
	}
	else if(this->elements[0].token.type == GLOBAL_VARIABLE) {
		output += "\"variableType\":\"GLOBAL_VARIABLE\",";
	}
	else if(this->elements[0].token.type) {
		output += "\"variableType\":\"SYMBOL\",";
	}
	else if(this->elements[0].component != nullptr) {
		output += "\"variableType\":\"NONE\",";
	}

	output += "\"chain\":[";
	string comma = this->elements.size() != 1 ? "," : "";
	for(AccessElement &element: this->elements) {
		if(element.component != nullptr) {
			output += element.component->printJSON() + comma;
		}
		else if(element.token.type) {
			output += "\"" + element.token.lexeme + "\"" + comma;
		}
	}

	if(output.back() == ',') {
		output.pop_back();
	}

	output += "]}";
	return output;
}

bool AccessStatement::isLocalVariable() {
	return this->elements.front().token.type == LOCAL_VARIABLE;
}

bool AccessStatement::isGlobalVariable() {
	return this->elements.front().token.type == GLOBAL_VARIABLE;
}

bool AccessStatement::startsWithFunction() {
	if(
		this->elements.size() < 2
		|| this->elements.front().token.type != SYMBOL
		|| this->elements[1].component == nullptr
		|| this->elements[1].component->getType() != CALL_STATEMENT
	) {
		return false;
	}
	
	return true;
}

bool AccessStatement::hasChain() {
	for(AccessElement element: this->elements) {
		if(element.token.type == MEMBER_CHAIN) {
			return true;
		}
	}
	return false;
}

bool AccessStatement::hasArray() {
	for(AccessElement element: this->elements) {
		if(element.isArray) {
			return true;
		}
	}
	return false;
}

bool AccessStatement::hasCall() {
	for(AccessElement element: this->elements) {
		if(element.isCall) {
			return true;
		}
	}
	return false;
}

size_t AccessStatement::chainSize() {
	return this->elements.size();
}

// we cannot assign to a function call or to a symbol
bool AccessStatement::isValidLValue() {
	if(this->elements.size() == 1 && this->elements.back().token.type == SYMBOL) {
		return false;
	}
	else if(this->elements.back().isCall) {
		return false;
	}
	return true;
}

size_t AccessStatement::getStackIndex(ts::Scope* scope) {
	if(!this->isLocalVariable() || this->chainSize() != 1) {
		return -1;
	}
	else {
		string name = this->getVariableName();
		return scope->allocateVariable(name).stackIndex;
	}
}

ts::InstructionReturn AccessStatement::compile(ts::Engine* engine, ts::CompilationContext context) {
	return this->compileAccess(engine, context).output;
}

// create instructions that set up the stack for an array access/object property access instruction
AccessStatementCompiled AccessStatement::compileAccess(ts::Engine* engine, ts::CompilationContext context) {
	AccessStatementCompiled c;

	auto iterator = this->elements.begin();

	if(this->startsWithFunction()) { // compile a function call
		c.output.add(this->elements[1].component->compile(engine, context)); // push arguments

		// push the amount of arguments we just found
		ts::Instruction* instruction = new ts::Instruction();
		instruction->type = ts::instruction::PUSH;
		instruction->push.entry.type = ts::entry::NUMBER;
		instruction->push.entry.setNumber(((CallStatement*)this->elements[1].component)->getElementCount());
		c.output.add(instruction);

		// build call instruction
		ts::Instruction* callFunction = new ts::Instruction();
		callFunction->type = ts::instruction::CALL_FUNCTION;
		ALLOCATE_STRING(this->elements[0].token.lexeme, callFunction->callFunction.name);
		ALLOCATE_STRING(string(""), callFunction->callFunction.nameSpace);
		callFunction->callFunction.cachedFunctionList = nullptr;
		callFunction->callFunction.cachedEntry = nullptr;
		callFunction->callFunction.isCached = false;
		c.output.add(callFunction);

		if(this->parent->requiresSemicolon(this)) { // if we do not assign/need the value of the function, just pop it
			ts::Instruction* pop = new ts::Instruction();
			pop->type = ts::instruction::POP;
			c.output.add(pop);
		}

		++iterator;
		++iterator;
	}


	// special cases for strings at the start of the access statement
	if(this->elements[0].component != nullptr && this->elements[0].component->getType() == STRING_LITERAL) {
		c.output.add(this->elements[0].component->compile(engine, context));
		++iterator;
	}

	int count = 0;
	ts::Instruction* lastInstruction = nullptr;
	for(; iterator != this->elements.end(); ++iterator) {
		AccessElement element = *iterator;
		if(element.token.type == LOCAL_VARIABLE) {
			ts::Instruction* instruction = new ts::Instruction();
			instruction->type = ts::instruction::LOCAL_ACCESS;
			instruction->localAccess.hash = hash<string>{}(toLower(element.token.lexeme));
			ALLOCATE_STRING(toLower(element.token.lexeme), instruction->localAccess.source);
			instruction->localAccess.stackIndex = -1;

			c.lastAccess = instruction;

			lastInstruction = instruction;
		}
		else if(element.token.type == GLOBAL_VARIABLE) {
			ts::Instruction* instruction = new ts::Instruction();
			instruction->type = ts::instruction::GLOBAL_ACCESS;
			instruction->globalAccess.hash = hash<string>{}(toLower(element.token.lexeme));
			ALLOCATE_STRING(toLower(element.token.lexeme), instruction->globalAccess.source);

			c.lastAccess = instruction;

			lastInstruction = instruction;
		}
		else if(element.token.type == SYMBOL) {
			ts::Instruction* instruction = new ts::Instruction();
			instruction->type = ts::instruction::SYMBOL_ACCESS;
			instruction->symbolAccess.hash = hash<string>{}(toLower(element.token.lexeme));
			ALLOCATE_STRING(toLower(element.token.lexeme), instruction->symbolAccess.source);

			c.lastAccess = instruction;

			lastInstruction = instruction;
		}
		else if(element.isArray) {
			if(lastInstruction != nullptr) {
				if(lastInstruction->type == ts::instruction::LOCAL_ACCESS) {
					lastInstruction->localAccess.stackIndex = context.scope->allocateVariable(lastInstruction->localAccess.source).stackIndex;
				}
				c.output.add(lastInstruction);
			}
			
			ts::InstructionReturn array = element.component->compile(engine, context);

			c.output.add(array);

			c.lastAccess = array.last;

			lastInstruction = nullptr;
		}
		else if(element.token.type == MEMBER_CHAIN) {
			if(lastInstruction != nullptr) {
				if(lastInstruction->type == ts::instruction::LOCAL_ACCESS) {
					lastInstruction->localAccess.stackIndex = context.scope->allocateVariable(lastInstruction->localAccess.source).stackIndex;
				}
				c.output.add(lastInstruction);
			}

			ts::Instruction* instruction = new ts::Instruction();
			instruction->type = ts::instruction::OBJECT_ACCESS;
			instruction->objectAccess.hash = hash<string>{}(toLower(element.token.lexeme));
			ALLOCATE_STRING(toLower(element.token.lexeme), instruction->objectAccess.source);

			c.lastAccess = instruction;

			lastInstruction = instruction;
		}
		else if(element.component != nullptr && element.component->getType() == PARENT_STATEMENT) {
			c.output.add(element.component->compile(engine, context));
			lastInstruction = nullptr;
		}
		else if(element.component != nullptr && element.component->getType() == CALL_STATEMENT) {
			// at this point, the object should already be on the stack. no need to push it. push the args
			c.output.add(element.component->compile(engine, context));

			// push the amount of arguments we just found
			ts::Instruction* pushArgumentCount = new ts::Instruction();
			pushArgumentCount->type = ts::instruction::PUSH;
			pushArgumentCount->push.entry = ts::Entry();
			pushArgumentCount->push.entry.type = ts::entry::NUMBER;
			pushArgumentCount->push.entry.setNumber(((CallStatement*)element.component)->getElementCount() + 1);
			c.output.add(pushArgumentCount);

			// compile the call instruction
			ts::Instruction* instruction = new ts::Instruction();
			instruction->type = ts::instruction::CALL_OBJECT;
			ALLOCATE_STRING(lastInstruction->objectAccess.source, instruction->callObject.name);
			instruction->callObject.cachedEntry = nullptr;
			instruction->callObject.isCached = false;

			c.output.add(instruction);

			if(this->parent->requiresSemicolon(this)) { // if we do not assign/need the value of the function, just pop it
				ts::Instruction* pop = new ts::Instruction();
				pop->type = ts::instruction::POP;
				c.output.add(pop);
			}
			
			delete lastInstruction; // this is a bit hacky, but whatever. forget about our last instruction
			lastInstruction = nullptr;
		}
		else if(element.component != nullptr && element.component->getType() == MATH_EXPRESSION) {
			c.output.add(element.component->compile(engine, context));
			lastInstruction = nullptr;
		}
		else if(element.component != nullptr && element.component->getType() == SYMBOL_STATEMENT) {
			ts::InstructionReturn symbol = element.component->compile(engine, context);
			c.lastAccess = symbol.first;
			lastInstruction = symbol.first;
		}
		count++;
	}

	if(lastInstruction != nullptr) {
		if(lastInstruction->type == ts::instruction::LOCAL_ACCESS) {
			lastInstruction->localAccess.stackIndex = context.scope->allocateVariable(lastInstruction->localAccess.source).stackIndex;
		}
		c.output.add(lastInstruction);
	}

	return c;
}

string AccessStatement::getVariableName() {
	return this->elements[0].token.lexeme;
}
