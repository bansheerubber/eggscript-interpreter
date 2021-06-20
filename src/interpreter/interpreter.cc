#include "interpreter.h"
#include "entry.h"
#include "debug.h"
#include "../util/stringToNumber.h"
#include "../util/numberToString.h"

using namespace ts;

Interpreter::Interpreter(ParsedArguments args) {
	for(int i = 0; i < 256; i++) {
		this->contexts[i].interpreter = this;
	}
	
	this->pushVariableContext();
	this->emptyEntry.setString(new string(""));

	for(sl::Function* function: sl::functions) {
		this->addTSSLFunction(function);
	}

	if(args.arguments["no-warnings"] != "") {
		this->warnings = false;
	}
}

Interpreter::~Interpreter() {
	
}

void Interpreter::addTSSLFunction(sl::Function* function) {
	Function* container = new Function(function);
	
	if(function->nameSpace.length() == 0) {
		this->nameToIndex[toLower(function->name)] = this->functions.size();
		this->functions.push_back(container);
	}
	else {
		if(this->namespaceToIndex.find(toLower(function->nameSpace)) == this->namespaceToIndex.end()) {
			this->namespaceToIndex[toLower(function->nameSpace)] = this->namespaceFunctions.size();

			// add to function data structure
			NamespaceFunctions* functions = new NamespaceFunctions();
			functions->nameToIndex[toLower(function->name)] = functions->functions.size();
			functions->nameToFunction[toLower(function->name)] = container;
			functions->functions.push_back(container);
			this->namespaceFunctions.push_back(functions);
		}
		else {
			NamespaceFunctions* functions = this->namespaceFunctions[this->namespaceToIndex[function->nameSpace]];
			functions->nameToIndex[toLower(function->name)] = functions->functions.size();
			functions->nameToFunction[toLower(function->name)] = container;
			functions->functions.push_back(container);
		}
	}
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
	this->topContext = &this->contexts[this->contextPointer];
	this->contextPointer++;
}

void Interpreter::popVariableContext() {
	this->topContext->clear();
	this->contextPointer--;
	this->topContext = &this->contexts[this->contextPointer - 1];
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

// push an object onto the stack
void Interpreter::push(ObjectReference* value) {
	this->stack[this->stackPointer].setObject(value);
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

void Interpreter::warning(const char* format, ...) {
	if(this->warnings) {
		va_list argptr;
		va_start(argptr, format);
		printWarning(format, argptr);
		va_end(argptr);
	}
}

void Interpreter::interpret() {
	if(*this->instructionPointer >= this->topContainer->size) { // quit once we run out of instructions
		long int elapsed = (chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - this->startTime)).count();
		printf("ran %d instructions in %lu us\n", this->ranInstructions, elapsed);
		this->topContext->print();
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

		// generate code for math instructions
		## math_generator.py

		// generate code for assignment instructions
		## assignments_generator.py
		
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
			Entry &entry = this->stack[this->stackPointer - 1];
			if(
				(
					entry.type == entry::NUMBER
					&& entry.numberData != 0
				)
				|| (
					entry.type == entry::STRING
					&& entry.stringData->length() != 0
				)
				|| (
					entry.type == entry::OBJECT
				)
			) {
				*this->instructionPointer = instruction.jumpIfTrue.index;
			}

			if(instruction.jumpIfTrue.pop) {
				this->pop();
			}
			break;
		}

		case instruction::JUMP_IF_FALSE: { // jump to an instruction
			Entry &entry = this->stack[this->stackPointer - 1];
			if(
				(
					entry.type == entry::NUMBER
					&& entry.numberData == 0
				)
				|| (
					entry.type == entry::STRING
					&& entry.stringData->length() == 0
				)
			) {
				*this->instructionPointer = instruction.jumpIfFalse.index;
			}

			if(instruction.jumpIfFalse.pop) {
				this->pop();
			}
			break;
		}

		case instruction::UNARY_MATHEMATICS: {
			Entry &value = this->stack[this->stackPointer - 1];
			double valueNumber = 0;
			if(value.type == entry::NUMBER) {
				valueNumber = value.numberData;
			}
			else {
				valueNumber = stringToNumber(*value.stringData);
			}
			
			this->pop();

			double result = 0.0;
			switch(instruction.unaryMathematics.operation) {
				case instruction::BITWISE_NOT: {
					result = ~((int)valueNumber);
					break;
				}

				case instruction::LOGICAL_NOT: {
					result = !((int)valueNumber);
					break;
				}

				case instruction::NEGATE: {
					result = -((int)valueNumber);
					break;
				}
			}
			this->push(result);
			break;
		}

		case instruction::ARGUMENT_ASSIGN: { // assign argument a value from the stack
			int actualArgumentCount = (int)this->stack[this->stackPointer - 1].numberData; // get the amount of arguments used
			int delta = instruction.argumentAssign.argc - actualArgumentCount;
			relative_stack_location location = this->stackPointer - 1 - instruction.argumentAssign.offset + delta;

			if((int)instruction.argumentAssign.offset <= delta) {
				this->topContext->setVariableEntry(
					instruction,
					instruction.argumentAssign.destination,
					instruction.argumentAssign.hash,
					this->emptyEntry
				);
			}
			else {
				this->topContext->setVariableEntry(
					instruction,
					instruction.argumentAssign.destination,
					instruction.argumentAssign.hash,
					this->stack[location]
				);
			}
			break;
		}
		
		case instruction::LOCAL_ACCESS: { // push local variable to stack
			Entry &entry = this->topContext->getVariableEntry(
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

		case instruction::OBJECT_ACCESS: { // push object property to stack
			Entry &objectEntry = this->stack[this->stackPointer - 1 - instruction.localAssign.dimensions];
			ObjectReference* object = objectEntry.objectData;

			// if the object is not alive anymore, push nothing to the stack
			if(objectEntry.type != entry::OBJECT || object->object == nullptr) {
				this->pop(); // pop the object
				this->push(this->emptyEntry);

				this->warning("trying to access deleted object\n");
				break;
			}
			
			Entry &entry = object->object->properties.getVariableEntry(
				instruction,
				instruction.localAccess.source,
				instruction.localAssign.hash
			);

			for(int i = 0; i < instruction.localAccess.dimensions; i++) {
				this->pop(); // pop the dimensions if we have any
			}

			this->pop(); // pop the object

			this->push(entry);

			break;
		}

		case instruction::CALL_FUNCTION: { // jump to a new instruction container
			if(!instruction.callFunction.isCached) {
				bool found = false;
				if(
					instruction.callFunction.nameSpace.length() != 0
					&& this->namespaceToIndex.find(toLower(instruction.callFunction.nameSpace)) != this->namespaceToIndex.end()
				) {
					int namespaceIndex = this->namespaceToIndex[toLower(instruction.callFunction.nameSpace)];
					if(
						this->namespaceFunctions[namespaceIndex]->nameToIndex.find(toLower(instruction.callFunction.name))
							!= this->namespaceFunctions[namespaceIndex]->nameToIndex.end()
					) {
						instruction.callFunction.cachedIndex = this->namespaceFunctions[namespaceIndex]->nameToIndex[toLower(instruction.callFunction.name)];
						instruction.callFunction.cachedNamespaceIndex = namespaceIndex;
						instruction.callFunction.isCached = true;
						instruction.callFunction.isNamespaceCached = true;

						if(this->namespaceFunctions[namespaceIndex]->functions[instruction.callFunction.cachedIndex]->isTSSL) {
							instruction.callFunction.isTSSL = true;
						}

						found = true;
					}
				}
				else { // find non-namespace function
					if(this->nameToIndex.find(toLower(instruction.callFunction.name)) != this->nameToIndex.end()) {
						instruction.callFunction.cachedIndex = this->nameToIndex[toLower(instruction.callFunction.name)];
						instruction.callFunction.isCached = true;

						if(this->functions[instruction.callFunction.cachedIndex]->isTSSL) {
							instruction.callFunction.isTSSL = true;
						}

						found = true;
					}
				}

				// print warning if function was not defined
				if(found == false) {
					this->warning("could not find function with name '%s'\n", instruction.callFunction.name.c_str());
				
					// pop arguments that we didn't use
					Entry &numberOfArguments = this->stack[this->stackPointer - 1];
					int number = (int)numberOfArguments.numberData;
					for(int i = 0; i < number + 1; i++) {
						this->pop();
					}

					this->push(this->emptyEntry);
					break;
				}
			}

			Function* foundFunction;
			if(instruction.callFunction.isNamespaceCached) {
				foundFunction = this->namespaceFunctions[instruction.callFunction.cachedNamespaceIndex]->functions[instruction.callFunction.cachedIndex];
			}
			else {
				foundFunction = this->functions[instruction.callFunction.cachedIndex];
			}

			## call_generator.py

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

		case instruction::CREATE_OBJECT: {
			if(
				!instruction.createObject.isCached
				&& this->namespaceToIndex.find(instruction.createObject.type) != this->namespaceToIndex.end()
			) {
				instruction.createObject.namespaceIndex = this->namespaceToIndex[instruction.createObject.type];
				instruction.createObject.isCached = true;
			}
			
			Object* object = new Object(this, instruction.createObject.type, instruction.createObject.namespaceIndex);
			this->push(new ObjectReference(object));
			break;
		}

		case instruction::CALL_OBJECT: {
			Entry &numberOfArguments = this->stack[this->stackPointer - 1];
			int argumentCount = (int)numberOfArguments.numberData;
			
			// pull the object from the stack
			Entry &objectEntry = this->stack[this->stackPointer - 1 - argumentCount];
			ObjectReference* object = objectEntry.objectData;

			if(objectEntry.type != entry::OBJECT || object->object == nullptr) {
				this->warning("trying to call a deleted object\n");
				
				// pop arguments that we didn't use
				Entry &numberOfArguments = this->stack[this->stackPointer - 1];
				int number = (int)numberOfArguments.numberData;
				for(int i = 0; i < number + 1; i++) {
					this->pop();
				}

				this->push(this->emptyEntry);
				break;
			}

			auto search = this->namespaceFunctions[object->object->namespaceIndex]->nameToFunction.find(toLower(instruction.callObject.name));
			if(search != this->namespaceFunctions[object->object->namespaceIndex]->nameToFunction.end()) {
				this->printStack();
				Function* foundFunction = search->second;
				## call_generator.py
			}
			else {
				this->warning("could not find function with name '%s::%s'\n", object->object->nameSpace.c_str(), instruction.callFunction.name.c_str());

				// pop arguments that we didn't use
				Entry &numberOfArguments = this->stack[this->stackPointer - 1];
				int number = (int)numberOfArguments.numberData;
				for(int i = 0; i < number + 1; i++) {
					this->pop();
				}

				this->push(this->emptyEntry);
			}
			
			break;
		}

		default: {
			printf("DID NOT EXECUTE INSTRUCTION.\n");
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

		printf("#%d ", i);
		entry.print();
	}
	printf("\n");
}

void Interpreter::addFunction(string &name, InstructionReturn output) {
	Function* container = new Function(output.first, name);
	this->nameToIndex[toLower(name)] = this->functions.size();
	this->functions.push_back(container);
}

void Interpreter::addFunction(string &nameSpace, string &name, InstructionReturn output) {
	Function* container = new Function(output.first, name, nameSpace);

	if(this->namespaceToIndex.find(toLower(nameSpace)) == this->namespaceToIndex.end()) {
		this->namespaceToIndex[toLower(nameSpace)] = this->namespaceFunctions.size();

		// add to function data structure
		NamespaceFunctions* functions = new NamespaceFunctions();
		functions->nameToIndex[toLower(name)] = functions->functions.size();
		functions->nameToFunction[toLower(name)] = container;
		functions->functions.push_back(container);
		this->namespaceFunctions.push_back(functions);
	}
	else {
		NamespaceFunctions* functions = this->namespaceFunctions[this->namespaceToIndex[nameSpace]];
		functions->nameToIndex[toLower(name)] = functions->functions.size();
		functions->nameToFunction[toLower(name)] = container;
		functions->functions.push_back(container);
	}
}