#include "accessStatement.h"
#include "../interpreter/interpreter.h"

#include "../util/allocateString.h"
#include "arrayStatement.h"
#include "callStatement.h"
#include "../util/cloneString.h"
#include "../util/stringToChars.h"
#include "symbol.h"

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

uint64_t AccessStatement::chainSize() {
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

uint64_t AccessStatement::getStackIndex(ts::Scope* scope) {
	if(!this->isLocalVariable() || this->chainSize() != 1) {
		return -1;
	}
	else {
		string name = this->getVariableName();
		return scope->allocateVariable(name, false, this->getCharacterNumber(), this->getLineNumber()).stackIndex;
	}
}

ts::InstructionReturn AccessStatement::compile(ts::Engine* engine, ts::CompilationContext context) {
	return this->compileAccess(engine, context).output;
}

// create instructions that set up the stack for an array access/object property access instruction
AccessStatementCompiled AccessStatement::compileAccess(ts::Engine* engine, ts::CompilationContext context) {
	AccessStatementCompiled c;

	auto iterator = this->elements.begin();
	int count = 0;

	if(this->startsWithFunction()) { // compile a function call
		c.output.add(this->elements[1].component->compile(engine, context)); // push arguments

		// push the amount of arguments we just found
		ts::Instruction* instruction = new ts::Instruction(
			engine,
			this->elements[0].token.characterNumber,
			this->elements[0].token.lineNumber
		);
		instruction->type = ts::instruction::PUSH;
		instruction->push.entry.type = ts::entry::NUMBER;
		instruction->push.entry.setNumber(((CallStatement*)this->elements[1].component)->getElementCount());
		c.output.add(instruction);

		// build call instruction
		ts::Instruction* callFunction = new ts::Instruction(
			engine,
			this->elements[0].token.characterNumber,
			this->elements[0].token.lineNumber
		);
		callFunction->type = ts::instruction::CALL_FUNCTION_UNLINKED;
		callFunction->callFunction.name = cloneString(this->elements[0].token.lexeme.c_str());
		callFunction->callFunction.cachedFunctionList = nullptr;
		c.output.add(callFunction);

		engine->addUnlinkedInstruction(callFunction);

		if(this->parent->requiresSemicolon(this) && this->elements.size() == 2) { // if we do not assign/need the value of the function, just pop it
			ts::Instruction* pop = new ts::Instruction(
				engine,
				this->elements[0].token.characterNumber,
				this->elements[0].token.lineNumber
			);
			pop->type = ts::instruction::POP;
			c.output.add(pop);
		}

		++iterator;
		++iterator;
		count += 2;
	}


	// special cases for strings at the start of the access statement
	if(this->elements[0].component != nullptr && this->elements[0].component->getType() == STRING_LITERAL) {
		c.output.add(this->elements[0].component->compile(engine, context));
		++iterator;
		count++;
	}

	ts::Instruction* lastInstruction = nullptr;
	for(; iterator != this->elements.end(); ++iterator) {
		AccessElement element = *iterator;
		if(element.token.type == LOCAL_VARIABLE) {
			ts::Instruction* instruction = new ts::Instruction(
				engine,
				element.token.characterNumber,
				element.token.lineNumber
			);
			instruction->type = ts::instruction::LOCAL_ACCESS;
			instruction->localAccess.hash = hash<string>{}(element.token.lexeme);
			instruction->localAccess.source = cloneString(element.token.lexeme.c_str());
			instruction->localAccess.stackIndex = -1;

			c.lastAccess = instruction;

			lastInstruction = instruction;
		}
		else if(element.token.type == GLOBAL_VARIABLE) {
			ts::Instruction* instruction = new ts::Instruction(
				engine,
				element.token.characterNumber,
				element.token.lineNumber
			);
			instruction->type = ts::instruction::GLOBAL_ACCESS;
			instruction->globalAccess.hash = hash<string>{}(element.token.lexeme);
			instruction->globalAccess.source = cloneString(element.token.lexeme.c_str());

			c.lastAccess = instruction;

			lastInstruction = instruction;
		}
		else if(element.isArray) {
			if(lastInstruction != nullptr) {
				if(lastInstruction->type == ts::instruction::LOCAL_ACCESS) {
					lastInstruction->localAccess.stackIndex = context.scope->allocateVariable(
						string(lastInstruction->localAccess.source),
						false,
						engine->getInstructionDebug(lastInstruction).character,
						engine->getInstructionDebug(lastInstruction).line
					).stackIndex;
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
					lastInstruction->localAccess.stackIndex = context.scope->allocateVariable(
						string(lastInstruction->localAccess.source),
						false,
						engine->getInstructionDebug(lastInstruction).character,
						engine->getInstructionDebug(lastInstruction).line
					).stackIndex;
				}
				c.output.add(lastInstruction);
			}

			ts::Instruction* instruction = new ts::Instruction(
				engine,
				element.token.characterNumber,
				element.token.lineNumber
			);
			instruction->type = ts::instruction::OBJECT_ACCESS;
			instruction->objectAccess.hash = hash<string>{}(element.token.lexeme);
			instruction->objectAccess.source = cloneString(element.token.lexeme.c_str());

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
			ts::Instruction* pushArgumentCount = new ts::Instruction(
				engine,
				element.component->getCharacterNumber(),
				element.component->getLineNumber()
			);
			pushArgumentCount->type = ts::instruction::PUSH;
			pushArgumentCount->push.entry = ts::Entry();
			pushArgumentCount->push.entry.type = ts::entry::NUMBER;
			pushArgumentCount->push.entry.setNumber(((CallStatement*)element.component)->getElementCount() + 1);
			c.output.add(pushArgumentCount);

			// compile the call instruction
			ts::Instruction* instruction = new ts::Instruction(
				engine,
				element.component->getCharacterNumber(),
				element.component->getLineNumber()
			);
			instruction->type = ts::instruction::CALL_OBJECT_UNLINKED;
			instruction->callObject.name = cloneString(lastInstruction->objectAccess.source);
			instruction->callObject.cachedIndex = 0;

			engine->addUnlinkedInstruction(instruction);

			c.output.add(instruction);

			if(this->parent->requiresSemicolon(this) && (unsigned int)count == this->elements.size() - 1) { // if we do not assign/need the value of the function, just pop it
				ts::Instruction* pop = new ts::Instruction(
					engine,
					element.component->getCharacterNumber(),
					element.component->getLineNumber()
				);
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
			ts::Instruction* instruction = new ts::Instruction(
				engine,
				element.component->getCharacterNumber(),
				element.component->getLineNumber()
			);
			instruction->type = ts::instruction::OBJECT_ASSIGN_EQUAL;
			instruction->objectAssign.entry = ts::Entry(); // initialize memory to avoid crash

			instruction->objectAssign.hash = hash<string>{}(((Symbol*)element.component)->value);
			instruction->objectAssign.destination = cloneString(((Symbol*)element.component)->value.c_str());
			instruction->objectAssign.fromStack = false;
			instruction->objectAssign.pushResult = false;
			instruction->objectAssign.popObject = false;
			
			c.lastAccess = instruction;
			lastInstruction = instruction;
		}
		count++;
	}

	if(lastInstruction != nullptr) {
		if(lastInstruction->type == ts::instruction::LOCAL_ACCESS) {
			lastInstruction->localAccess.stackIndex = context.scope->allocateVariable(
				string(lastInstruction->localAccess.source),
				false,
				engine->getInstructionDebug(lastInstruction).character,
				engine->getInstructionDebug(lastInstruction).line
			).stackIndex;
		}
		c.output.add(lastInstruction);
	}

	return c;
}

string AccessStatement::getVariableName() {
	return this->elements[0].token.lexeme;
}
