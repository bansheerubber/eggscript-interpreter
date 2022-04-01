#include "interpreter.h"

#include "../util/allocateString.h"
#include "../tssl/array.h"
#include "../util/cloneString.h"
#include "../compiler/compiler.h"
#include "debug.h"
#include "../tssl/define.h"
#include "../engine/engine.h"
#include "entry.h"
#include "../util/getEmptyString.h"
#include "../util/isInteger.h"
#include "object.h"
#include "../util/numberToString.h"
#include "../parser/parser.h"
#include "../tssl/map.h"
#include "stack.h"
#include "../util/stringCompare.h"
#include "../util/stringToNumber.h"
#include "../util/time.h"
#include "../tokenizer/tokenizer.h"

using namespace ts;

void ts::initFunctionFrame(Interpreter* interpreter, FunctionFrame* frame) {
	frame->container = nullptr;
	frame->instructionPointer = 0;
	frame->stackPointer = 0;
	frame->stackPopCount = 0;
}

void ts::onFunctionFrameRealloc(Interpreter* interpreter) {
	FunctionFrame &frame = interpreter->frames[interpreter->frames.head - 1];
	interpreter->instructionPointer = &frame.instructionPointer;
	interpreter->stackFramePointer = frame.stackPointer;
}

void ts::initSchedule(Interpreter* interpreter, Schedule** schedule) {
	*schedule = nullptr;
}

Interpreter::Interpreter(Engine* engine, ParsedArguments args, bool isParallel) {
	this->engine = engine;

	if(args.arguments["no-warnings"] != "") {
		this->warnings = false;
	}

	if(args.arguments["time"] != "") {
		this->showTime = true;
	}

	this->globalContext = VariableContext(this);

	if(this->isParallel) {
		this->enterParallel();
		this->startTime = getMicrosecondsNow();
	}

	#ifdef TS_INSTRUCTIONS_AS_FUNCTIONS
	this->initInstructionToFunction();
	#endif
}

Interpreter::~Interpreter() {
	
}

void Interpreter::enterParallel() { 
	this->tickThread = thread(&Interpreter::tick, this);
	this->isParallel = true;
}

void Interpreter::declareObjectProperties(Function* function) {
	FunctionFrame &frame = this->frames[this->frames.head];
	frame.container = function;
	frame.instructionPointer = 0;
	frame.stackPointer = this->stack.head - 1;
	frame.stackPopCount = 1;
	frame.packagedFunctionList = nullptr;
	frame.packagedFunctionListIndex = 0;
	frame.methodTreeEntry = nullptr;
	frame.methodTreeEntryIndex = 0;
	frame.earlyQuit = true;
	frame.isTSSL = false;
	frame.fileName = nullptr;

	this->topContainer = frame.container;
	this->instructionPointer = &frame.instructionPointer;
	this->stackFramePointer = frame.stackPointer;
	
	this->frames.pushed();

	this->interpret();
}

void Interpreter::pushFunctionFrame(InstructionContainer* container) {
	this->pushFunctionFrame(container, nullptr, -1, nullptr, -1, 0, 0, nullptr, false, instruction::STACK);
}

void Interpreter::pushFunctionFrame(
	InstructionContainer* container,
	PackagedFunctionList* list,
	int packagedFunctionListIndex,
	MethodTreeEntry* methodTreeEntry,
	int methodTreeEntryIndex,
	uint64_t argumentCount,
	uint64_t popCount,
	string* fileName,
	bool earlyQuit,
	instruction::PushType type
) {
	if(this->frames.head == 0) {
		this->startTime = getMicrosecondsNow();
	}
	
	FunctionFrame &frame = this->frames[this->frames.head];
	frame.container = container;
	frame.instructionPointer = 0;
	frame.stackPointer = this->stack.head - argumentCount;
	frame.stackPopCount = popCount;
	frame.packagedFunctionList = list;
	frame.packagedFunctionListIndex = packagedFunctionListIndex;
	frame.methodTreeEntry = methodTreeEntry;
	frame.methodTreeEntryIndex = methodTreeEntryIndex;
	frame.earlyQuit = earlyQuit;
	frame.isTSSL = false;
	frame.fileName = fileName;
	frame.pushType = type;

	this->topContainer = frame.container;
	this->instructionPointer = &frame.instructionPointer;
	this->stackFramePointer = frame.stackPointer;
	
	this->frames.pushed();
}

void Interpreter::popFunctionFrame() {
	this->frames.popped();

	for(uint64_t i = 0; i < this->frames[this->frames.head].stackPopCount; i++) {
		this->pop();
	}

	if(this->frames.head == 0) {
		this->topContainer = nullptr;
		this->instructionPointer = nullptr;
		this->stackFramePointer = 0;
		this->frames[this->frames.head].stackPopCount = 0; // don't pop any extra stuff
	}
	else {
		FunctionFrame &frame = this->frames[this->frames.head - 1];
		this->topContainer = frame.container;
		this->instructionPointer = &frame.instructionPointer;
		this->stackFramePointer = frame.stackPointer;
	}
}

void Interpreter::pushTSSLFunctionFrame(MethodTreeEntry* methodTreeEntry, int methodTreeEntryIndex) {
	FunctionFrame &frame = this->frames[this->frames.head];
	frame.earlyQuit = false;
	frame.isTSSL = true;
	frame.stackPopCount = 0;
	frame.methodTreeEntry = methodTreeEntry;
	frame.methodTreeEntryIndex = methodTreeEntryIndex;
	frame.fileName = nullptr;
	this->frames.pushed();
}

string& Interpreter::getTopFileNameFromFrame() {
	static string empty = "";
	for(int i = this->frames.head - 1; i >= 0; i--) {
		if(this->frames[i].fileName != nullptr) {
			return *this->frames[i].fileName;
		}
	}
	return empty;
}

// push an entry onto the stack
void Interpreter::push(Entry &entry, instruction::PushType type) {
	if(type < 0) {
		copyEntry(entry, this->stack[this->stack.head]);
		this->stack.pushed();
	}
	else {
		copyEntry(entry, this->returnRegister);
	}
}

// push an entry onto the stack, greedily
void Interpreter::push(Entry &entry, instruction::PushType type, bool greedy) {
	if(type < 0) {
		greedyCopyEntry(entry, this->stack[this->stack.head]);
		this->stack.pushed();
	}
	else {
		greedyCopyEntry(entry, this->returnRegister);
	}
}

// push a number onto the stack
void Interpreter::push(double number, instruction::PushType type) {
	if(type < 0) {
		// manually inline this b/c for some reason it doesn't want to by itself
		Entry &entry = this->stack[this->stack.head];
		entry.type = entry::NUMBER;
		entry.numberData = number;
		this->stack.pushed();	
	}
	else {
		this->returnRegister.type = entry::NUMBER;
		this->returnRegister.numberData = number;
	}
}

// push a string onto the stack
void Interpreter::push(char* value, instruction::PushType type) {
	if(type < 0) {
		this->stack[this->stack.head].setString(value);
		this->stack.pushed();
	}
	else {
		this->returnRegister.setString(value);
	}
}

// push a matrix onto the stack
void Interpreter::push(Matrix* value, instruction::PushType type) {
	if(type < 0) {
		this->stack[this->stack.head].setMatrix(value);
		this->stack.pushed();
	}
	else {
		this->returnRegister.setMatrix(value);
	}
}

// push an object onto the stack
void Interpreter::push(ObjectReference* value, instruction::PushType type) {
	if(type < 0) {
		this->stack[this->stack.head].setObject(value);
		this->stack.pushed();
	}
	else {
		this->returnRegister.setObject(value);
	}
}

void Interpreter::pushEmpty(instruction::PushType type) {
	if(type < 0) {
		this->stack[this->stack.head].erase();
		this->stack.pushed();
	}
	else {
		this->returnRegister.erase();
	}
}

void Interpreter::startInterpretation(Instruction* head) {
	InstructionContainer* container = new InstructionContainer(this->engine, head);
	this->pushFunctionFrame(container); // create the instructions
	this->startTime = getMicrosecondsNow();
	this->interpret();
	delete container;
}

Entry* Interpreter::handleTSSLParent(string &name, unsigned int argc, Entry* argv, entry::EntryType* argumentTypes) {
	FunctionFrame &frame = this->frames[this->frames.head - 1];
	MethodTreeEntry* methodTreeEntry = frame.methodTreeEntry;
	int methodTreeEntryIndex = frame.methodTreeEntryIndex + 1; // always go up in the method tree
	PackagedFunctionList* list;
	int packagedFunctionListIndex;

	if((uint64_t)methodTreeEntryIndex < methodTreeEntry->list.head) {
		list = methodTreeEntry->list[methodTreeEntryIndex];
		packagedFunctionListIndex = list->topValidIndex;

		Function* foundFunction = (*list)[packagedFunctionListIndex];
		if(foundFunction->isTSSL) {
			sl::Function* function = foundFunction->function;
			this->pushTSSLFunctionFrame(methodTreeEntry, methodTreeEntryIndex);
			Entry* returnValue = function->function(this->engine, argc, argv);
			this->popFunctionFrame();
			return returnValue;
		}
		else {
			// push arguments onto the stack
			for(uint64_t i = 0; i < argc; i++) {
				this->push(argv[i], instruction::STACK);
			}

			this->push((double)argc, instruction::STACK);
			
			this->pushFunctionFrame(
				foundFunction,
				list,
				packagedFunctionListIndex,
				methodTreeEntry,
				methodTreeEntryIndex,
				argc + 1,
				foundFunction->variableCount,
				nullptr,
				false,
				instruction::RETURN_REGISTER
			);
			this->interpret();

			return new Entry(this->returnRegister);
		}
	}

	return new Entry();
}

void Interpreter::warning(Instruction* instruction, const char* format, ...) {
	if(this->warnings) {
		string formatString(format);

		if(instruction != nullptr) {
			InstructionDebug debug = this->engine->getInstructionDebug(instruction);
			if(debug.commonSource != nullptr) {
				formatString = debug.commonSource->fileName + ":" + to_string(debug.line) + ":" + to_string(debug.character) + ": " + formatString;
			}
		}
		
		va_list argptr;
		va_start(argptr, format);
		(*this->engine->vWarningFunction)(formatString.c_str(), argptr);
		va_end(argptr);
	}
}

bool Interpreter::tick() {
	start_tick:
	
	uint64_t time = getMicrosecondsNow();

	Schedule* schedule = this->schedules.top();
	while(this->schedules.array.head > 0 && time > schedule->end) {
		if(schedule->object != nullptr) {
			this->callMethod(schedule->object, schedule->functionName, schedule->arguments, schedule->argumentCount);
		}
		else {
			this->callFunction(schedule->functionName, schedule->arguments, schedule->argumentCount);
		}

		this->schedules.pop();
		schedule = this->schedules.top();
	}

	// process queued files for parallel mode
	if(this->isParallel) {
		while(this->engine->fileQueue.size() != 0) {
			string filename = this->engine->fileQueue.front();
			this->engine->fileQueue.pop();

			this->engine->execFile(filename, true);
		}

		while(this->engine->shellQueue.size() != 0) {
			string shell = this->engine->shellQueue.front();
			this->engine->shellQueue.pop();

			this->engine->execShell(shell, true);
		}

		this_thread::sleep_for(chrono::milliseconds(this->tickRate));
		goto start_tick;
	}

	this->garbageCollect(10000);

	// return false if we have schedules left, return true if there's none left
	return this->schedules.array.head == 0;
}

void Interpreter::setTickRate(int64_t tickRate) {
	this->tickRate = tickRate;
}

void Interpreter::garbageCollect(unsigned int amount) {
	for(uint64_t i = 0; i < amount && 0 < this->garbageHeap.array.head && this->garbageHeap.array[0]->referenceCount <= 0; i++) {
		delete this->garbageHeap.array[0];
		this->garbageHeap.pop();
	}
}

unsigned int Interpreter::probeGarbage(string className) {
	unsigned int count = 0;
	for(uint64_t i = 0; i < this->garbageHeap.array.head; i++) {
		if(this->garbageHeap.array[i]->object->methodTree->name == className) {
			count++;
		}
	}
	return count;
}

void Interpreter::interpret() {
	start:
	Instruction &instruction = this->topContainer->array[*this->instructionPointer];
	(*this->instructionPointer)++;

	// PrintInstruction(instruction);
	// this->printStack();

	#ifdef TS_INSTRUCTIONS_AS_FUNCTIONS
	(*this->instructionToFunction[instruction.type])(this, instruction);
	#else
	switch(instruction.type) {
		default: {
			printf("DID NOT EXECUTE INSTRUCTION.\n");
		}
		
		case instruction::INVALID_INSTRUCTION: {
			this->popFunctionFrame();

			if(this->showTime && this->frames.head == 0) {
				(*this->engine->printFunction)("exec time: %lld\n", getMicrosecondsNow() - this->startTime);
			}

			return;	
		}

		// generate code for math instructions
		## math_generator.py

		// generate code for assignment instructions
		## assignments_generator.py
		
		case instruction::NOOP: {
			break;
		}

		case instruction::PUSH: { // push to the stack
			this->push(instruction.push.entry, instruction.pushType);
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

		case instruction::JUMP_IF_TRUE_THEN_POP: {
			Entry &entry = this->stack[this->stack.head - 1];
			if(isEntryTruthy(entry)) {
				*this->instructionPointer = instruction.jump.index;
			}

			this->pop();
			break;
		}

		case instruction::JUMP_IF_TRUE: { // jump to an instruction
			Entry &entry = this->stack[this->stack.head - 1];
			if(isEntryTruthy(entry)) {
				*this->instructionPointer = instruction.jump.index;
			}
			break;
		}

		case instruction::JUMP_IF_FALSE_THEN_POP: {
			Entry &entry = this->stack[this->stack.head - 1];
			if(!isEntryTruthy(entry)) {
				*this->instructionPointer = instruction.jump.index;
			}

			this->pop();
			break;
		}

		case instruction::JUMP_IF_FALSE: { // jump to an instruction
			Entry &entry = this->stack[this->stack.head - 1];
			if(!isEntryTruthy(entry)) {
				*this->instructionPointer = instruction.jump.index;
			}
			break;
		}

		case instruction::UNARY_BITWISE_NOT: {
			Entry* value;
			if(instruction.unaryMathematics.stackIndex < 0) {
				value = &this->stack[this->stack.head - 1];
			}
			else {
				value = &this->stack[instruction.unaryMathematics.stackIndex + this->stackFramePointer];
			}

			if(value->type != entry::NUMBER) {
				this->pushEmpty(instruction.pushType);
				break;
			}

			if(instruction.unaryMathematics.stackIndex < 0) {
				this->pop();
			}

			this->push(~((int)value->numberData), instruction.pushType);
			break;
		}

		case instruction::UNARY_NEGATE: {
			Entry* value;
			if(instruction.unaryMathematics.stackIndex < 0) {
				value = &this->stack[this->stack.head - 1];
			}
			else {
				value = &this->stack[instruction.unaryMathematics.stackIndex + this->stackFramePointer];
			}

			if(value->type != entry::NUMBER) {
				this->pushEmpty(instruction.pushType);
				break;
			}

			if(instruction.unaryMathematics.stackIndex < 0) {
				this->pop();
			}

			this->push(-((int)value->numberData), instruction.pushType);
			break;
		}

		case instruction::UNARY_LOGICAL_NOT: {
			Entry* value;
			if(instruction.unaryMathematics.stackIndex < 0) {
				value = &this->stack[this->stack.head - 1];
			}
			else {
				value = &this->stack[instruction.unaryMathematics.stackIndex + this->stackFramePointer];
			}

			if(instruction.unaryMathematics.stackIndex < 0) {
				this->pop();
			}

			this->push(!isEntryTruthy(*value), instruction.pushType);
			break;
		}

		case instruction::LOCAL_ACCESS: { // push local variable to stack
			this->push(this->stack[instruction.localAccess.stackIndex + this->stackFramePointer], instruction.pushType);
			break;
		}

		case instruction::GLOBAL_ACCESS: { // push local variable to stack
			Entry &entry = this->globalContext.getVariableEntry(
				instruction,
				instruction.globalAccess.source,
				instruction.globalAccess.hash
			);

			this->push(entry, instruction.pushType);

			break;
		}

		case instruction::OBJECT_ACCESS: { // push object property to stack
			Entry &objectEntry = this->stack[this->stack.head - 1];
			ObjectWrapper* objectWrapper = nullptr;

			## type_conversion.py objectEntry objectWrapper ALL OBJECT

			// if the object is not alive anymore, push nothing to the stack
			if(objectWrapper == nullptr) {
				this->pop(); // pop the object
				this->pushEmpty(instruction.pushType);
				break;
			}
			
			Entry &entry = objectWrapper->object->properties.getVariableEntry(
				instruction,
				instruction.localAccess.source,
				instruction.localAssign.hash
			);

			this->pop(); // pop the object

			this->push(entry, instruction.pushType);

			break;
		}

		case instruction::CALL_FUNCTION_UNLINKED: { // if the function is unlinked, then print a warning, push nothing onto the stack, and noop
			this->warning(&instruction, "could not find function with name '%s'\n", instruction.callFunction.name);
				
			// pop arguments that we didn't use
			Entry &numberOfArguments = this->stack[this->stack.head - 1];
			int number = (int)numberOfArguments.numberData;
			for(int i = 0; i < number + 1; i++) {
				this->pop();
			}

			this->pushEmpty(instruction.pushType);
			break;
		}

		case instruction::CALL_FUNCTION: { // jump to a new instruction container
			Function* foundFunction;
			PackagedFunctionList* list;
			int packagedFunctionListIndex = -1;
			MethodTreeEntry* methodTreeEntry = nullptr;
			int methodTreeEntryIndex = -1;
			list = instruction.callFunction.cachedFunctionList;
			packagedFunctionListIndex = list->topValidIndex;
			foundFunction = (*list)[packagedFunctionListIndex];

			## call_generator.py

			break;
		}

		case instruction::CALL_NAMESPACE_FUNCTION_UNLINKED: { // if the function is unlinked, then print a warning, push nothing onto the stack, and noop
			this->warning(
				&instruction,
				"could not find function with name '%s::%s'\n",
				instruction.callNamespaceFunction.nameSpace,
				instruction.callNamespaceFunction.name
			);
				
			// pop arguments that we didn't use
			Entry &numberOfArguments = this->stack[this->stack.head - 1];
			int number = (int)numberOfArguments.numberData;
			for(int i = 0; i < number + 1; i++) {
				this->pop();
			}

			this->pushEmpty(instruction.pushType);
			break;
		}

		case instruction::CALL_NAMESPACE_FUNCTION: {
			int packagedFunctionListIndex = -1;
			MethodTreeEntry* methodTreeEntry = nullptr;
			int methodTreeEntryIndex = -1;
			PackagedFunctionList* list = instruction.callNamespaceFunction.cachedEntry->list[0];
			packagedFunctionListIndex = list->topValidIndex;
			Function* foundFunction = list->topValidFunction;

			## call_generator.py

			break;
		}

		case instruction::RETURN_NO_VALUE: {
			this->popFunctionFrame();

			// if we just ran out of instruction containers, just die here
			if(this->topContainer == nullptr) {
				return;
			}

			this->pushEmpty(this->frames[this->frames.head].pushType);

			// if the current function frame is TSSL, then we're in a C++ PARENT(...) operation and we need to quit
			// here so the original TSSL method can take over
			if(this->frames[this->frames.head - 1].isTSSL || this->frames[this->frames.head].earlyQuit) {
				return;
			}

			break;
		}

		case instruction::MOVE_THEN_RETURN: { // return from a function
			copyEntry(this->stack[this->stack.head - 1], this->returnRegister);
			this->pop();

			this->popFunctionFrame();

			// if we just ran out of instruction containers, just die here
			if(this->topContainer == nullptr) {
				return;
			}

			if(this->frames[this->frames.head].pushType < 0) {
				this->push(this->returnRegister, instruction::STACK, true); // push return register	
			}

			// if the current function frame is TSSL, then we're in a C++ PARENT(...) operation and we need to quit
			// here so the original TSSL method can take over
			if(this->frames[this->frames.head - 1].isTSSL || this->frames[this->frames.head].earlyQuit) {
				return;
			}

			break;
		}

		case instruction::RETURN: { // return from a function
			this->popFunctionFrame();

			// if we just ran out of instruction containers, just die here
			if(this->topContainer == nullptr) {
				return;
			}

			if(this->frames[this->frames.head].pushType < 0) {
				this->push(this->returnRegister, instruction::STACK, true); // push return register	
			}

			// if the current function frame is TSSL, then we're in a C++ PARENT(...) operation and we need to quit
			// here so the original TSSL method can take over
			if(this->frames[this->frames.head - 1].isTSSL || this->frames[this->frames.head].earlyQuit) {
				return;
			}

			break;
		}

		case instruction::POP_ARGUMENTS: {
			Entry &numberOfArguments = this->stack[this->stack.head - 1];
			uint64_t realNumberOfArguments = instruction.popArguments.argumentCount;
			int number = (int)numberOfArguments.numberData - realNumberOfArguments;

			this->pop(); // pop argument count
			for(int i = 0; i < number; i++) {
				this->pop();
			}

			for(int i = 0; i < -number; i++) {
				this->pushEmpty(instruction.pushType);
			}

			break;
		}

		case instruction::CREATE_OBJECT_UNLINKED: {
			this->warning(&instruction, "could not create object with type '%s'\n", instruction.createObject.typeName);
			this->pushEmpty(instruction.pushType);	
			break;
		}

		case instruction::CREATE_OBJECT: {
			string typeName = instruction.createObject.typeName;

			ObjectWrapper* object = ts::CreateObject(
				this,
				true,
				typeName,
				instruction.createObject.methodTree
			);

			this->push(new ObjectReference(object), instruction.pushType);
			break;
		}

		case instruction::CALL_OBJECT_UNLINKED: { // if the method is unlinked, then print a warning, push nothing onto the stack, and noop
			Entry &numberOfArguments = this->stack[this->stack.head - 1];
			int argumentCount = (int)numberOfArguments.numberData;
			
			// pull the object from the stack
			Entry &objectEntry = this->stack[this->stack.head - 1 - argumentCount];
			ObjectWrapper* objectWrapper = nullptr;
			Object* object = nullptr;
			## type_conversion.py objectEntry objectWrapper ALL OBJECT

			if(objectWrapper == nullptr) {
				this->warning(&instruction, "could not find object for method call\n");
				
				// pop arguments that we didn't use
				for(int i = 0; i < argumentCount + 1; i++) {
					this->pop();
				}

				this->pushEmpty(instruction.pushType);
				break;
			}

			object = objectWrapper->object;
			
			this->warning(&instruction, "could not find function with name '%s::%s'\n", object->nameSpace.c_str(), instruction.callObject.name);

			// pop arguments that we didn't use
			for(int i = 0; i < argumentCount + 1; i++) {
				this->pop();
			}

			this->pushEmpty(instruction.pushType);
			break;
		}

		case instruction::CALL_OBJECT: {
			Entry &numberOfArguments = this->stack[this->stack.head - 1];
			int argumentCount = (int)numberOfArguments.numberData;
			
			// pull the object from the stack
			Entry &objectEntry = this->stack[this->stack.head - 1 - argumentCount];
			ObjectWrapper* objectWrapper = nullptr;
			Object* object = nullptr;

			if(objectEntry.type == entry::OBJECT) {
				objectWrapper = objectEntry.objectData->objectWrapper;
			}
			else {
				this->warning(&instruction, "could not call method on non-object\n");
				break;
			}

			if(objectWrapper == nullptr) {
				this->warning(&instruction, "could not find object for method call\n");
				
				// pop arguments that we didn't use
				for(int i = 0; i < argumentCount + 1; i++) {
					this->pop();
				}

				this->pushEmpty(instruction.pushType);
				break;
			}

			object = objectWrapper->object;

			auto methodEntry = object->methodTree->methodIndexToEntry.find(instruction.callObject.cachedIndex);
			if(methodEntry == object->methodTree->methodIndexToEntry.end()) {
				this->warning(&instruction, "could not find function with name '%s::%s'\n", object->nameSpace.c_str(), instruction.callObject.name);

				// pop arguments that we didn't use
				for(int i = 0; i < argumentCount + 1; i++) {
					this->pop();
				}

				this->pushEmpty(instruction.pushType);
				break;
			}

			// look up the method in the method tree
			MethodTreeEntry* methodTreeEntry = methodEntry->second;
			int methodTreeEntryIndex = methodTreeEntry->hasInitialMethod || methodTreeEntry->list[0]->topValidIndex != 0 ? 0 : 1;
			PackagedFunctionList* list = methodTreeEntry->list[methodTreeEntryIndex];
			uint64_t packagedFunctionListIndex = list->topValidIndex;
			Function* foundFunction = list->topValidFunction;
			## call_generator.py
			
			break;
		}

		case instruction::CALL_PARENT: {
			FunctionFrame &frame = this->frames[this->frames.head - 1];
			MethodTreeEntry* methodTreeEntry = frame.methodTreeEntry;
			int methodTreeEntryIndex = frame.methodTreeEntryIndex;
			PackagedFunctionList* list = frame.packagedFunctionList;
			int packagedFunctionListIndex = frame.packagedFunctionList->getNextValidIndex(frame.packagedFunctionListIndex);

			if(packagedFunctionListIndex == -1 && methodTreeEntry != nullptr) { // walk the method tree
				methodTreeEntryIndex++;
				if((uint64_t)methodTreeEntryIndex < methodTreeEntry->list.head) {
					list = methodTreeEntry->list[methodTreeEntryIndex];
					packagedFunctionListIndex = list->topValidIndex;

					Function* foundFunction = (*list)[packagedFunctionListIndex];
					## call_generator.py
				}
				else {
					// pop arguments that we didn't use
					Entry &numberOfArguments = this->stack[this->stack.head - 1];
					int number = (int)numberOfArguments.numberData;
					for(int i = 0; i < number + 1; i++) {
						this->pop();
					}

					this->pushEmpty(instruction.pushType);
					break;
				}
			}
			else if(packagedFunctionListIndex != -1) {
				Function* foundFunction = (*list)[packagedFunctionListIndex];
				## call_generator.py
			}
			else {
				// pop arguments that we didn't use
				Entry &numberOfArguments = this->stack[this->stack.head - 1];
				int number = (int)numberOfArguments.numberData;
				for(int i = 0; i < number + 1; i++) {
					this->pop();
				}

				this->pushEmpty(instruction.pushType);
				break;
			}

			break;
		}
		
		case instruction::ARRAY_ACCESS: {
			Entry &objectEntry = this->stack[this->stack.head - 2];
			Entry &indexEntry = this->stack[this->stack.head - 1];

			if(objectEntry.type == entry::OBJECT) {
				// pull the object from the stack
				ObjectWrapper* objectWrapper = objectEntry.objectData->objectWrapper;
				Object* object = nullptr;

				if(objectWrapper == nullptr || objectWrapper->object->dataStructure == NO_DATA_STRUCTURE) {
					goto quit_array_access;
				}

				object = objectWrapper->object;

				bool failure = false;
				switch(object->dataStructure) {
					case ARRAY: {
						unsigned int index = 0;
						## type_conversion.py indexEntry index ALL NUMBER

						DynamicArray<Entry, sl::Array> &array = ((ts::sl::Array*)objectWrapper->data)->array;

						if(index >= array.head) {
							failure = true;
						}
						else {
							this->pop();
							this->pop();
							this->push(array[index], instruction.pushType);
						}
						break;
					}

					case MAP: {
						const char* key = nullptr;
						bool deleteString = false;
						## type_conversion.py indexEntry key ALL STRING deleteString

						auto map = &(((ts::sl::Map*)objectWrapper->data)->map);
						auto iter = map->find(string(key));
						if(iter == map->end()) {
							failure = true;
						}
						else {
							this->pop();
							this->pop();
							this->push(iter.value(), instruction.pushType);
						}

						if(deleteString && key != nullptr) {
							delete[] key;
						}
						break;
					}
				}

				if(failure) {
					goto quit_array_access;
				}
			}
			else if(objectEntry.type == entry::MATRIX && objectEntry.matrixData != nullptr) {
				unsigned int index = 0;
				## type_conversion.py indexEntry index ALL NUMBER
				if(objectEntry.matrixData->rows == 1) { // treat it like an array (return a scalar)
					if(index >= objectEntry.matrixData->columns) {
						goto quit_array_access;
					}

					copyEntry(objectEntry.matrixData->data[0][index], this->returnRegister);
					this->pop();
					this->pop();
					this->push(this->returnRegister, instruction.pushType);
				}
				else if(objectEntry.matrixData->columns == 1) { // treat it like an array (return a scalar)
					if(index >= objectEntry.matrixData->rows) {
						goto quit_array_access;
					}

					copyEntry(objectEntry.matrixData->data[index][0], this->returnRegister);
					this->pop();
					this->pop();
					this->push(this->returnRegister, instruction.pushType);
				}
				else { // dealing with multiple rows and columns (return a row as a vector)
					if(index >= objectEntry.matrixData->rows) {
						goto quit_array_access;
					}

					Matrix* output = objectEntry.matrixData->cloneRowToVector(index);
					if(output == nullptr) {
						goto quit_array_access;
					}
					else {
						this->pop();
						this->pop();
						this->push(output, instruction.pushType);
					}
				}
			}
			else {
				quit_array_access:
				this->pop();
				this->pop();
				this->pushEmpty(instruction.pushType);
			}
			break;
		}

		case instruction::MATRIX_CREATE: {
			this->push(new Matrix(instruction.matrixCreate.rows, instruction.matrixCreate.columns), instruction.pushType);
			break;
		}

		case instruction::MATRIX_SET: {
			Matrix* matrix = this->stack[this->stack.head - 2].matrixData;
			copyEntry(this->stack[this->stack.head - 1], matrix->data[instruction.matrixSet.row][instruction.matrixSet.column]);
			this->pop();
			break;
		}
	}
	#endif

	// this->printStack();

	goto start;
}

#ifdef TS_INSTRUCTIONS_AS_FUNCTIONS
void Interpreter::initInstructionToFunction() {
	this->instructionToFunction[instruction::INVALID_INSTRUCTION] = instruction_INVALID_INSTRUCTION;
	this->instructionToFunction[instruction::NOOP] = instruction_NOOP;
	this->instructionToFunction[instruction::PUSH] = instruction_PUSH;
	this->instructionToFunction[instruction::POP] = instruction_POP;
	this->instructionToFunction[instruction::JUMP] = instruction_JUMP;
	this->instructionToFunction[instruction::JUMP_IF_TRUE] = instruction_JUMP_IF_TRUE;
	this->instructionToFunction[instruction::JUMP_IF_TRUE_THEN_POP] = instruction_JUMP_IF_TRUE_THEN_POP;
	this->instructionToFunction[instruction::JUMP_IF_FALSE] = instruction_JUMP_IF_FALSE;
	this->instructionToFunction[instruction::JUMP_IF_FALSE_THEN_POP] = instruction_JUMP_IF_FALSE_THEN_POP;
	this->instructionToFunction[instruction::MATH_ADDITION] = instruction_MATH_ADDITION;
	this->instructionToFunction[instruction::MATH_SUBTRACT] = instruction_MATH_SUBTRACT;
	this->instructionToFunction[instruction::MATH_MULTIPLY] = instruction_MATH_MULTIPLY;
	this->instructionToFunction[instruction::MATH_DIVISION] = instruction_MATH_DIVISION;
	this->instructionToFunction[instruction::MATH_MODULUS] = instruction_MATH_MODULUS;
	this->instructionToFunction[instruction::MATH_SHIFT_LEFT] = instruction_MATH_SHIFT_LEFT;
	this->instructionToFunction[instruction::MATH_SHIFT_RIGHT] = instruction_MATH_SHIFT_RIGHT;
	this->instructionToFunction[instruction::MATH_EQUAL] = instruction_MATH_EQUAL;
	this->instructionToFunction[instruction::MATH_NOT_EQUAL] = instruction_MATH_NOT_EQUAL;
	this->instructionToFunction[instruction::MATH_LESS_THAN_EQUAL] = instruction_MATH_LESS_THAN_EQUAL;
	this->instructionToFunction[instruction::MATH_GREATER_THAN_EQUAL] = instruction_MATH_GREATER_THAN_EQUAL;
	this->instructionToFunction[instruction::MATH_LESS_THAN] = instruction_MATH_LESS_THAN;
	this->instructionToFunction[instruction::MATH_GREATER_THAN] = instruction_MATH_GREATER_THAN;
	this->instructionToFunction[instruction::MATH_BITWISE_AND] = instruction_MATH_BITWISE_AND;
	this->instructionToFunction[instruction::MATH_BITWISE_OR] = instruction_MATH_BITWISE_OR;
	this->instructionToFunction[instruction::MATH_BITWISE_XOR] = instruction_MATH_BITWISE_XOR;
	this->instructionToFunction[instruction::MATH_APPEND] = instruction_MATH_APPEND;
	this->instructionToFunction[instruction::MATH_SPC] = instruction_MATH_SPC;
	this->instructionToFunction[instruction::MATH_TAB] = instruction_MATH_TAB;
	this->instructionToFunction[instruction::MATH_NL] = instruction_MATH_NL;
	this->instructionToFunction[instruction::MATH_DOT_PRODUCT] = instruction_MATH_DOT_PRODUCT;
	this->instructionToFunction[instruction::MATH_CROSS_PRODUCT] = instruction_MATH_CROSS_PRODUCT;
	this->instructionToFunction[instruction::UNARY_BITWISE_NOT] = instruction_UNARY_BITWISE_NOT;
	this->instructionToFunction[instruction::UNARY_LOGICAL_NOT] = instruction_UNARY_LOGICAL_NOT;
	this->instructionToFunction[instruction::UNARY_NEGATE] = instruction_UNARY_NEGATE;
	this->instructionToFunction[instruction::LOCAL_ASSIGN_EQUAL] = instruction_LOCAL_ASSIGN_EQUAL;
	this->instructionToFunction[instruction::LOCAL_ASSIGN_INCREMENT] = instruction_LOCAL_ASSIGN_INCREMENT;
	this->instructionToFunction[instruction::LOCAL_ASSIGN_DECREMENT] = instruction_LOCAL_ASSIGN_DECREMENT;
	this->instructionToFunction[instruction::LOCAL_ASSIGN_PLUS] = instruction_LOCAL_ASSIGN_PLUS;
	this->instructionToFunction[instruction::LOCAL_ASSIGN_MINUS] = instruction_LOCAL_ASSIGN_MINUS;
	this->instructionToFunction[instruction::LOCAL_ASSIGN_ASTERISK] = instruction_LOCAL_ASSIGN_ASTERISK;
	this->instructionToFunction[instruction::LOCAL_ASSIGN_SLASH] = instruction_LOCAL_ASSIGN_SLASH;
	this->instructionToFunction[instruction::LOCAL_ASSIGN_MODULUS] = instruction_LOCAL_ASSIGN_MODULUS;
	this->instructionToFunction[instruction::LOCAL_ASSIGN_SHIFT_LEFT] = instruction_LOCAL_ASSIGN_SHIFT_LEFT;
	this->instructionToFunction[instruction::LOCAL_ASSIGN_SHIFT_RIGHT] = instruction_LOCAL_ASSIGN_SHIFT_RIGHT;
	this->instructionToFunction[instruction::LOCAL_ASSIGN_BITWISE_AND] = instruction_LOCAL_ASSIGN_BITWISE_AND;
	this->instructionToFunction[instruction::LOCAL_ASSIGN_BITWISE_XOR] = instruction_LOCAL_ASSIGN_BITWISE_XOR;
	this->instructionToFunction[instruction::LOCAL_ASSIGN_BITWISE_OR] = instruction_LOCAL_ASSIGN_BITWISE_OR;
	this->instructionToFunction[instruction::GLOBAL_ASSIGN_EQUAL] = instruction_GLOBAL_ASSIGN_EQUAL;
	this->instructionToFunction[instruction::GLOBAL_ASSIGN_INCREMENT] = instruction_GLOBAL_ASSIGN_INCREMENT;
	this->instructionToFunction[instruction::GLOBAL_ASSIGN_DECREMENT] = instruction_GLOBAL_ASSIGN_DECREMENT;
	this->instructionToFunction[instruction::GLOBAL_ASSIGN_PLUS] = instruction_GLOBAL_ASSIGN_PLUS;
	this->instructionToFunction[instruction::GLOBAL_ASSIGN_MINUS] = instruction_GLOBAL_ASSIGN_MINUS;
	this->instructionToFunction[instruction::GLOBAL_ASSIGN_ASTERISK] = instruction_GLOBAL_ASSIGN_ASTERISK;
	this->instructionToFunction[instruction::GLOBAL_ASSIGN_SLASH] = instruction_GLOBAL_ASSIGN_SLASH;
	this->instructionToFunction[instruction::GLOBAL_ASSIGN_MODULUS] = instruction_GLOBAL_ASSIGN_MODULUS;
	this->instructionToFunction[instruction::GLOBAL_ASSIGN_SHIFT_LEFT] = instruction_GLOBAL_ASSIGN_SHIFT_LEFT;
	this->instructionToFunction[instruction::GLOBAL_ASSIGN_SHIFT_RIGHT] = instruction_GLOBAL_ASSIGN_SHIFT_RIGHT;
	this->instructionToFunction[instruction::GLOBAL_ASSIGN_BITWISE_AND] = instruction_GLOBAL_ASSIGN_BITWISE_AND;
	this->instructionToFunction[instruction::GLOBAL_ASSIGN_BITWISE_XOR] = instruction_GLOBAL_ASSIGN_BITWISE_XOR;
	this->instructionToFunction[instruction::GLOBAL_ASSIGN_BITWISE_OR] = instruction_GLOBAL_ASSIGN_BITWISE_OR;
	this->instructionToFunction[instruction::LOCAL_ACCESS] = instruction_LOCAL_ACCESS;
	this->instructionToFunction[instruction::GLOBAL_ACCESS] = instruction_GLOBAL_ACCESS;
	this->instructionToFunction[instruction::CALL_FUNCTION_UNLINKED] = instruction_CALL_FUNCTION_UNLINKED;
	this->instructionToFunction[instruction::CALL_FUNCTION] = instruction_CALL_FUNCTION;
	this->instructionToFunction[instruction::CALL_NAMESPACE_FUNCTION_UNLINKED] = instruction_CALL_NAMESPACE_FUNCTION_UNLINKED;
	this->instructionToFunction[instruction::CALL_NAMESPACE_FUNCTION] = instruction_CALL_NAMESPACE_FUNCTION;
	this->instructionToFunction[instruction::CALL_PARENT] = instruction_CALL_PARENT;
	this->instructionToFunction[instruction::RETURN_NO_VALUE] = instruction_RETURN_NO_VALUE;
	this->instructionToFunction[instruction::RETURN] = instruction_RETURN;
	this->instructionToFunction[instruction::POP_ARGUMENTS] = instruction_POP_ARGUMENTS;
	this->instructionToFunction[instruction::CREATE_OBJECT_UNLINKED] = instruction_CREATE_OBJECT_UNLINKED;
	this->instructionToFunction[instruction::CREATE_OBJECT] = instruction_CREATE_OBJECT;
	this->instructionToFunction[instruction::CALL_OBJECT_UNLINKED] = instruction_CALL_OBJECT_UNLINKED;
	this->instructionToFunction[instruction::CALL_OBJECT] = instruction_CALL_OBJECT;
	this->instructionToFunction[instruction::OBJECT_ASSIGN_EQUAL] = instruction_OBJECT_ASSIGN_EQUAL;
	this->instructionToFunction[instruction::OBJECT_ASSIGN_INCREMENT] = instruction_OBJECT_ASSIGN_INCREMENT;
	this->instructionToFunction[instruction::OBJECT_ASSIGN_DECREMENT] = instruction_OBJECT_ASSIGN_DECREMENT;
	this->instructionToFunction[instruction::OBJECT_ASSIGN_PLUS] = instruction_OBJECT_ASSIGN_PLUS;
	this->instructionToFunction[instruction::OBJECT_ASSIGN_MINUS] = instruction_OBJECT_ASSIGN_MINUS;
	this->instructionToFunction[instruction::OBJECT_ASSIGN_ASTERISK] = instruction_OBJECT_ASSIGN_ASTERISK;
	this->instructionToFunction[instruction::OBJECT_ASSIGN_SLASH] = instruction_OBJECT_ASSIGN_SLASH;
	this->instructionToFunction[instruction::OBJECT_ASSIGN_MODULUS] = instruction_OBJECT_ASSIGN_MODULUS;
	this->instructionToFunction[instruction::OBJECT_ASSIGN_SHIFT_LEFT] = instruction_OBJECT_ASSIGN_SHIFT_LEFT;
	this->instructionToFunction[instruction::OBJECT_ASSIGN_SHIFT_RIGHT] = instruction_OBJECT_ASSIGN_SHIFT_RIGHT;
	this->instructionToFunction[instruction::OBJECT_ASSIGN_BITWISE_AND] = instruction_OBJECT_ASSIGN_BITWISE_AND;
	this->instructionToFunction[instruction::OBJECT_ASSIGN_BITWISE_XOR] = instruction_OBJECT_ASSIGN_BITWISE_XOR;
	this->instructionToFunction[instruction::OBJECT_ASSIGN_BITWISE_OR] = instruction_OBJECT_ASSIGN_BITWISE_OR;
	this->instructionToFunction[instruction::OBJECT_ACCESS] = instruction_OBJECT_ACCESS;
	this->instructionToFunction[instruction::ARRAY_ACCESS] = instruction_ARRAY_ACCESS;
	this->instructionToFunction[instruction::ARRAY_ASSIGN_EQUAL] = instruction_ARRAY_ASSIGN_EQUAL;
	this->instructionToFunction[instruction::ARRAY_ASSIGN_INCREMENT] = instruction_ARRAY_ASSIGN_INCREMENT;
	this->instructionToFunction[instruction::ARRAY_ASSIGN_DECREMENT] = instruction_ARRAY_ASSIGN_DECREMENT;
	this->instructionToFunction[instruction::ARRAY_ASSIGN_PLUS] = instruction_ARRAY_ASSIGN_PLUS;
	this->instructionToFunction[instruction::ARRAY_ASSIGN_MINUS] = instruction_ARRAY_ASSIGN_MINUS;
	this->instructionToFunction[instruction::ARRAY_ASSIGN_ASTERISK] = instruction_ARRAY_ASSIGN_ASTERISK;
	this->instructionToFunction[instruction::ARRAY_ASSIGN_SLASH] = instruction_ARRAY_ASSIGN_SLASH;
	this->instructionToFunction[instruction::ARRAY_ASSIGN_MODULUS] = instruction_ARRAY_ASSIGN_MODULUS;
	this->instructionToFunction[instruction::ARRAY_ASSIGN_SHIFT_LEFT] = instruction_ARRAY_ASSIGN_SHIFT_LEFT;
	this->instructionToFunction[instruction::ARRAY_ASSIGN_SHIFT_RIGHT] = instruction_ARRAY_ASSIGN_SHIFT_RIGHT;
	this->instructionToFunction[instruction::ARRAY_ASSIGN_BITWISE_AND] = instruction_ARRAY_ASSIGN_BITWISE_AND;
	this->instructionToFunction[instruction::ARRAY_ASSIGN_BITWISE_XOR] = instruction_ARRAY_ASSIGN_BITWISE_XOR;
	this->instructionToFunction[instruction::ARRAY_ASSIGN_BITWISE_OR] = instruction_ARRAY_ASSIGN_BITWISE_OR;
	this->instructionToFunction[instruction::MATRIX_CREATE] = instruction_MATRIX_CREATE;
	this->instructionToFunction[instruction::MATRIX_SET] = instruction_MATRIX_SET;
}
#endif

void Interpreter::printStack() {
	printf("\nSTACK: %ld\n", this->stack.head);
	for(uint64_t i = 0; i < this->stack.head; i++) {
		Entry &entry = this->stack[i];

		printf("#%ld ", i);
		entry.print();
	}
	printf("\n");
}

void Interpreter::addSchedule(uint64_t time, string functionName, Entry* arguments, uint64_t argumentCount, ObjectReference* object) {
	this->schedules.insert(new Schedule(
		time,
		getMicrosecondsNow(),
		functionName,
		arguments,
		argumentCount,
		object
	));
}

Entry* Interpreter::callFunction(string functionName, Entry* arguments, uint64_t argumentCount) {
	// set up function call frame
	Function* foundFunction;
	PackagedFunctionList* list;
	int packagedFunctionListIndex = -1;
	MethodTreeEntry* methodTreeEntry = nullptr;
	int methodTreeEntryIndex = -1;

	if(this->engine->nameToFunctionIndex.find(functionName) != this->engine->nameToFunctionIndex.end()) {
		list = this->engine->functions[this->engine->nameToFunctionIndex[functionName]];
		packagedFunctionListIndex = list->topValidIndex;
		foundFunction = (*list)[packagedFunctionListIndex];
	}
	else {
		this->warning(nullptr, "could not find function with name '%s'\n", functionName.c_str());
		return new Entry();
	}

	if(foundFunction->isTSSL) { // TODO handle argument type conversion
		sl::Function* function = foundFunction->function;
		this->pushTSSLFunctionFrame(methodTreeEntry, methodTreeEntryIndex);
		Entry* result = function->function(this->engine, argumentCount, arguments);
		this->popFunctionFrame();

		if(result == nullptr) {
			return new Entry();
		}
		return result;
	}
	else {
		// push arguments onto the stack
		for(uint64_t i = 0; i < argumentCount; i++) {
			this->push(arguments[i], instruction::STACK);
		}

		this->push((double)argumentCount, instruction::STACK);

		// handle callback
		this->pushFunctionFrame(
			foundFunction,
			list,
			packagedFunctionListIndex,
			methodTreeEntry,
			methodTreeEntryIndex,
			argumentCount + 1,
			foundFunction->variableCount,
			nullptr,
			true,
			instruction::RETURN_REGISTER
		);
		this->interpret();
	
		return new Entry(this->returnRegister);
	}
}

Entry* Interpreter::callMethod(ObjectReference* objectReference, string methodName, Entry* arguments, uint64_t argumentCount, bool inhibitInterpret) {
	// set up function call frame
	Function* foundFunction;
	PackagedFunctionList* list;
	int packagedFunctionListIndex = -1;
	MethodTreeEntry* methodTreeEntry = nullptr;
	int methodTreeEntryIndex = -1;

	ObjectWrapper* objectWrapper = objectReference->objectWrapper;
	Object* object = nullptr;
	if(objectWrapper == nullptr) {
		return new Entry();
	}

	object = objectWrapper->object;
	
	bool found = false;
	auto methodNameIndex = this->engine->methodNameToIndex.find(methodName);
	if(methodNameIndex != this->engine->methodNameToIndex.end()) {
		auto methodEntry = object->methodTree->methodIndexToEntry.find(methodNameIndex->second);
		if(methodEntry != object->methodTree->methodIndexToEntry.end()) {
			methodTreeEntry = methodEntry->second;
			methodTreeEntryIndex = methodTreeEntry->hasInitialMethod ? 0 : 1;
			list = methodTreeEntry->list[methodTreeEntryIndex];
			packagedFunctionListIndex = list->topValidIndex;
			foundFunction = (*list)[packagedFunctionListIndex];
			found = true;
		}
	}

	if(!found) {
		this->warning(nullptr, "could not find function with name '%s::%s'\n", object->nameSpace.c_str(), methodName.c_str());
		return new Entry();
	}

	if(foundFunction->isTSSL) { // TODO handle argument type conversion
		sl::Function* function = foundFunction->function;
		this->pushTSSLFunctionFrame(methodTreeEntry, methodTreeEntryIndex);
		Entry* output = function->function(this->engine, argumentCount, arguments);
		this->popFunctionFrame();

		if(output == nullptr) {
			return new Entry();
		}
		return output;
	}
	else {
		// push arguments onto the stack
		for(uint64_t i = 0; i < argumentCount; i++) {
			this->push(arguments[i], instruction::STACK);
		}

		this->push((double)argumentCount, instruction::STACK);

		// handle callback
		this->pushFunctionFrame(
			foundFunction,
			list,
			packagedFunctionListIndex,
			methodTreeEntry,
			methodTreeEntryIndex,
			argumentCount + 1,
			foundFunction->variableCount,
			nullptr,
			true,
			instruction::RETURN_REGISTER
		);
		this->interpret();

		// if(this->topContainer != nullptr) {
		// 	this->pop(); // pop the return value from the stack, otherwise its just going to stay there forever
		// }

		return new Entry(this->returnRegister);
	}
}