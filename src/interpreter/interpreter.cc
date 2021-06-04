#include "interpreter.h"
#include "entry.h"
#include "debug.h"
#include "../util/stringToNumber.h"
#include "../util/numberToString.h"
#include "../util/toLower.h"

using namespace ts;

Interpreter::Interpreter() {
	this->pushVariableContext();
	this->emptyEntry.setString(new string(""));
}

Interpreter::~Interpreter() {
	
}

void Interpreter::pushInstructionContainer(InstructionContainer* container) {
	// push container
	this->topContainer = container;
	this->containerStack[this->containerStackPointer] = container;
	this->containerStackPointer++;

	// push pointer
	this->pointerStack[this->pointerStackPointer] = 0;
	this->instructionPointer = &this->pointerStack[this->pointerStackPointer];
	this->pointerStackPointer++;
}

void Interpreter::popInstructionContainer() {
	// pop container
	this->containerStackPointer--;
	this->topContainer = this->containerStack[this->containerStackPointer - 1];

	// pop pointer
	this->pointerStackPointer--;
	this->instructionPointer = &this->pointerStack[this->pointerStackPointer - 1];
}

void Interpreter::pushVariableContext() {
	this->contextPointer++;
	this->getTopVariableContext().interpreter = this;
}

void Interpreter::popVariableContext() {
	this->getTopVariableContext().clear();
	this->contextPointer--;
}

VariableContext& Interpreter::getTopVariableContext() {
	return this->contexts[this->contextPointer - 1];
}

// push an entry onto the stack
void Interpreter::push(Entry &entry) {
	copyEntry(entry, this->stack[this->stackPointer]);
	this->stackPointer++;
}

// push a number onto the stack
void Interpreter::push(double number) {
	this->stack[this->stackPointer].setNumber(number);
	this->stackPointer++;
}

// push a string onto the stack
void Interpreter::push(string* value) {
	this->stack[this->stackPointer].setString(value);
	this->stackPointer++;
}

void Interpreter::pop() {
	if(this->stackPointer == 0) { // protect against empty stack
		printError("empty stack\n");
		exit(1);
	}
	
	this->stackPointer--;
}

void Interpreter::startInterpretation(Instruction* head) {
	this->pushInstructionContainer(new InstructionContainer(head)); // create the instructions
	// this->topContainer->print();
	this->startTime = chrono::high_resolution_clock::now();
	this->interpret();
}

void Interpreter::interpret() {
	if(*this->instructionPointer >= this->topContainer->size) { // quit once we run out of instructions
		long int elapsed = (chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - this->startTime)).count();
		printf("ran %d instructions in %lu us\n", this->ranInstructions, elapsed);
		this->getTopVariableContext().print();
		this->printStack();
		return;
	}

	Instruction &instruction = this->topContainer->array[*this->instructionPointer];
	(*this->instructionPointer)++;

	// PrintInstruction(instruction);
	
	switch(instruction.type) {
		case instruction::INVALID_INSTRUCTION: {
			printError("invalid instruction\n");
			exit(1);
		}
		
		case instruction::NOOP: {
			break;
		}

		case instruction::PUSH: { // push to the stack
			this->push(instruction.push.entry);
			break;
		}

		case instruction::POP: { // pop from the stack
			this->pop();
			break;
		}

		case instruction::JUMP: { // jump to an instruction
			*this->instructionPointer = instruction.jump.index;
			break;
		}

		case instruction::JUMP_IF_TRUE: { // jump to an instruction
			if(this->stack[this->stackPointer - 1].numberData != 0) {
				*this->instructionPointer = instruction.jumpIfTrue.index;
			}

			if(instruction.jumpIfTrue.pop) {
				this->pop();
			}
			break;
		}

		case instruction::JUMP_IF_FALSE: { // jump to an instruction
			if(this->stack[this->stackPointer - 1].numberData == 0) {
				*this->instructionPointer = instruction.jumpIfFalse.index;
			}

			if(instruction.jumpIfFalse.pop) {
				this->pop();
			}
			break;
		}

		case instruction::MATHEMATICS: { // do the math
			Entry* lvalue;
			Entry* rvalue;

			// do this at compile time so we for sure remove the conditional from runtime
			#if(TS_INTERPRETER_PREFIX == 1)
			{
				// we have to define lvalue first when working with prefix
				if(instruction.mathematics.lvalueEntry.type != entry::INVALID) {
					lvalue = &instruction.mathematics.lvalueEntry;
				}
				else {
					lvalue = &this->stack[this->stackPointer - 1];
					this->pop();
				}
				
				// set rvalue
				if(instruction.mathematics.rvalueEntry.type != entry::INVALID) {
					rvalue = &instruction.mathematics.rvalueEntry;
				}
				else {
					rvalue = &this->stack[this->stackPointer - 1];
					this->pop();
				}
			}
			#else
			{
				// we have to define rvalue first when working with postfix
				if(instruction.mathematics.rvalueEntry.type != entry::INVALID) {
					rvalue = &instruction.mathematics.rvalueEntry;
				}
				else {
					rvalue = &this->stack[this->stackPointer - 1];
					this->pop();
				}

				// set lvalue
				if(instruction.mathematics.lvalueEntry.type != entry::INVALID) {
					lvalue = &instruction.mathematics.lvalueEntry;
				}
				else {
					lvalue = &this->stack[this->stackPointer - 1];
					this->pop();
				}
			}
			#endif

			// start the hell that is dynamic typing
			double lvalueNumber = 0;
			double rvalueNumber = 0;
			string* lvalueString = nullptr;
			string* rvalueString = nullptr;
			bool deleteLValueString = false; // if we need to delete the string if we converted from number to string
			bool deleteRValueString = false;
			if(instruction.mathematics.operation < instruction::STRING_EQUAL) { // if we're dealing with numerical operation
				if(lvalue->type == entry::NUMBER) { // typecheck lvalue
					lvalueNumber = lvalue->numberData;
				}
				else {
					lvalueNumber = stringToNumber(*lvalue->stringData);
				}

				if(rvalue->type == entry::NUMBER) { // typecheck rvalue
					rvalueNumber = rvalue->numberData;
				}
				else {
					rvalueNumber = stringToNumber(*rvalue->stringData);
				}
			}
			else { // if we're dealing with a string operation
				if(lvalue->type == entry::STRING) { // typecheck lvalue
					lvalueString = lvalue->stringData;
				}
				else {
					lvalueString = numberToString(lvalue->numberData);
					deleteLValueString = true;
				}

				if(rvalue->type == entry::STRING) { // typecheck rvalue
					rvalueString = rvalue->stringData;
				}
				else {
					rvalueString = numberToString(rvalue->numberData);
					deleteRValueString = true;
				}
			}

			double numberResult = 0.0;
			string* stringResult = nullptr;
			bool evaluated = true;
			switch(instruction.mathematics.operation) {
				case instruction::ADDITION: {
					numberResult = lvalueNumber + rvalueNumber;
					break;
				}

				case instruction::SUBTRACT: {
					numberResult = lvalueNumber - rvalueNumber;
					break;
				}

				case instruction::DIVISION: {
					numberResult = lvalueNumber / rvalueNumber;
					break;
				}

				case instruction::MULTIPLY: {
					numberResult = lvalueNumber * rvalueNumber;
					break;
				}

				case instruction::MODULUS: {
					numberResult = (int)lvalueNumber % (int)rvalueNumber;
					break;
				}

				case instruction::EQUAL: {
					numberResult = lvalueNumber == rvalueNumber;
					break;
				}

				case instruction::LESS_THAN_EQUAL: {
					numberResult = lvalueNumber <= rvalueNumber;
					break;
				}

				case instruction::GREATER_THAN_EQUAL: {
					numberResult = lvalueNumber >= rvalueNumber;
					break;
				}

				case instruction::LESS_THAN: {
					numberResult = lvalueNumber < rvalueNumber;
					break;
				}

				case instruction::GREATER_THAN: {
					numberResult = lvalueNumber > rvalueNumber;
					break;
				}

				case instruction::BITWISE_AND: {
					numberResult = (int)lvalueNumber & (int)rvalueNumber;
					break;
				}

				case instruction::BITWISE_OR: {
					numberResult = (int)lvalueNumber | (int)rvalueNumber;
					break;
				}

				case instruction::BITWISE_XOR: {
					numberResult = (int)lvalueNumber ^ (int)rvalueNumber;
					break;
				}

				case instruction::STRING_EQUAL: {
					numberResult = toLower(*lvalueString).compare(toLower(*rvalueString)) == 0;
					break;
				}

				case instruction::STRING_NOT_EQUAL: {
					numberResult = toLower(*lvalueString).compare(toLower(*rvalueString)) != 0;
					break;
				}

				case instruction::APPEND: { // append data to lvalue
					stringResult = new string(*lvalueString);
					*stringResult += *rvalueString;
					break;
				}

				case instruction::SPC: { // append data to lvalue
					stringResult = new string(*lvalueString);
					*stringResult += ' ' + *rvalueString;
					break;
				}

				default: {
					evaluated = false;
					break;
				}
			}

			// delete lvalue string if we converted it from a number
			if(deleteLValueString) {
				delete lvalueString;
			}

			// delete rvalue string if we converted it from a number
			if(deleteRValueString) {
				delete rvalueString;
			}

			// push the results to the stack			
			if(evaluated) {
				if(stringResult == nullptr) {
					this->push(numberResult);
				}
				else {
					this->push(stringResult);
				}
			}
			else {
				printError("could not evaluate math instruction\n");
				exit(1);
			}

			break;
		}

		case instruction::ARGUMENT_ASSIGN: { // assign argument a value from the stack
			int actualArgumentCount = (int)this->stack[this->stackPointer - 1].numberData; // get the amount of arguments used
			int delta = instruction.argumentAssign.argc - actualArgumentCount;
			relative_stack_location location = this->stackPointer - 1 - instruction.argumentAssign.offset + delta;

			if((int)instruction.argumentAssign.offset <= delta) {
				this->getTopVariableContext().setVariableEntry(
					instruction,
					instruction.argumentAssign.destination,
					instruction.argumentAssign.hash,
					this->emptyEntry
				);
			}
			else {
				this->getTopVariableContext().setVariableEntry(
					instruction,
					instruction.argumentAssign.destination,
					instruction.argumentAssign.hash,
					this->stack[location]
				);
			}
			break;
		}

		// TODO this whole thing probably needs to be optimized
		case instruction::LOCAL_ASSIGN: {
			Entry* entry = nullptr;
			double entryNumber = 0;
			string* entryString = nullptr;
			if(instruction.localAssign.operation != instruction::EQUALS) {
				entry = &this->getTopVariableContext().getVariableEntry(
					instruction,
					instruction.localAssign.destination,
					instruction.localAssign.hash
				);

				if(instruction.localAssign.operation >= instruction::INCREMENT) {
					if(entry->type == entry::NUMBER) {
						entryNumber = entry->numberData;
					}
					else {
						entryNumber = stringToNumber(*entry->stringData);
					}
				}
			}

			Entry* value = nullptr;
			bool fromStack = instruction.localAssign.fromStack;
			if(fromStack) {
				value = &this->stack[this->stackPointer - 1 - instruction.localAssign.dimensions]; // start from top of stack
			}
			else {
				value = &instruction.localAssign.entry;
			}

			switch(instruction.localAssign.operation) {
				case instruction::EQUALS: {
					this->getTopVariableContext().setVariableEntry(
						instruction,
						instruction.localAssign.destination,
						instruction.localAssign.hash,
						*value
					);
					break;
				}

				case instruction::INCREMENT: {
					entry->setNumber(++entryNumber);
					break;
				}

				case instruction::DECREMENT: {
					entry->setNumber(--entryNumber);
					break;
				}

				default: {
					break;
				}
			}

			for(int i = 0; i < instruction.localAssign.dimensions; i++) {
				this->pop(); // pop the dimensions if we have any
			}

			if(instruction.localAssign.fromStack) {
				this->pop(); // pop value from the stack
			}

			if(instruction.localAssign.operation != instruction::EQUALS) {
				if(instruction.localAssign.pushResult && entry != nullptr) {
					this->push(*entry);
				}
			}
			else if(instruction.localAssign.pushResult) {
				this->push(*value);
			}

			break;
		}

		case instruction::LOCAL_ACCESS: { // push local variable to stack
			Entry &entry = this->getTopVariableContext().getVariableEntry(
				instruction,
				instruction.localAccess.source,
				instruction.localAssign.hash
			);

			for(int i = 0; i < instruction.localAccess.dimensions; i++) {
				this->pop(); // pop the dimensions if we have any
			}

			this->push(entry);

			break;
		}

		case instruction::CALL_FUNCTION: { // jump to a new instruction container
			if(!instruction.callFunction.isCached) {
				bool found = false;
				if(this->nameToIndex.find(instruction.callFunction.name) != this->nameToIndex.end()) {
					instruction.callFunction.cachedIndex = this->nameToIndex[instruction.callFunction.name];
					instruction.callFunction.isCached = true;
					found = true;
				}

				if(functions::nameToIndex.find(instruction.callFunction.name) != functions::nameToIndex.end()) {
					instruction.callFunction.cachedIndex = functions::nameToIndex[instruction.callFunction.name];
					instruction.callFunction.isCached = true;
					instruction.callFunction.isTSSL = true;
					found = true;
				}

				if(found == false) {
					printError("could not find function with name '%s'\n", instruction.callFunction.name.c_str());
					exit(1);
				}
			}

			if(instruction.callFunction.isTSSL) { // call a standard library function if this instruction is defined as such
				functions::Function* function = functions::functions[instruction.callFunction.cachedIndex];

				void* arguments[TS_ARG_COUNT];
				// copy the same logic from ARGUMENT_ASSIGN instruction for consistency
				int actualArgumentCount = (int)this->stack[this->stackPointer - 1].numberData; // get the amount of arguments used in the program
				int delta = function->argumentCount - actualArgumentCount;

				int count = 1;
				for(int i = function->argumentCount; i >= 1; i--) { // load arguments array backwards
					if(count <= delta) { // TODO better error handling
						printError("incorrect amount of arguments\n");
						exit(0);
					}
					else {
						relative_stack_location location = this->stackPointer - 1 - i + delta;
						Entry &entry = this->stack[location];
						if(entry.type == entry::NUMBER) {
							arguments[count - 1] = &entry.numberData;
						}
						else {
							arguments[count - 1] = entry.stringData;
						}
					}
					count++;
				}

				// pop the data
				for(int i = 0; i < actualArgumentCount + 1; i++) {
					this->pop();
				}

				void* returnValue = function->function(actualArgumentCount, arguments);
				if(function->returnType == functions::type::STRING) {
					this->push((string*)returnValue);
				}
				else if(function->returnType == functions::type::NUMBER) {
					this->push(*((double*)returnValue));
					delete (double*)returnValue;
				}
				else { // push void return value
					this->push(this->emptyEntry);
				}
			}
			else {
				this->pushInstructionContainer(this->indexToFunction[instruction.callFunction.cachedIndex]);
				this->pushVariableContext();
			}
			break;
		}

		case instruction::RETURN: { // return from a function
			this->popInstructionContainer();
			this->popVariableContext();
			break;
		}

		case instruction::POP_ARGUMENTS: {
			Entry &numberOfArguments = this->stack[this->stackPointer - 1];

			int number = (int)numberOfArguments.numberData;
			for(int i = 0; i < number + 1; i++) {
				this->pop();
			}

			break;
		}
	}

	// this->printStack();

	this->ranInstructions++;

	this->interpret();
}

void Interpreter::printStack() {
	printf("\nSTACK: %d\n", this->stackPointer);
	for(unsigned int i = 0; i < this->stackPointer; i++) {
		Entry &entry = this->stack[i];
		
		printf("ENTRY #%d {\n", i);
		printf("   type: %d,\n", entry.type);

		if(entry.type == entry::STRING) {
			printf("   data: \"%s\",\n", entry.stringData->c_str());
		}
		if(entry.type == entry::NUMBER) {
			printf("   data: %f,\n", entry.numberData);
		}

		printf("};\n");
	}
	printf("\n");
}

void Interpreter::addFunction(string &name, InstructionReturn output) {
	auto index = this->functionCount;
	InstructionContainer* container = new InstructionContainer(output.first);
	this->nameToIndex[string(name)] = index;
	this->indexToFunction[index] = container;
}