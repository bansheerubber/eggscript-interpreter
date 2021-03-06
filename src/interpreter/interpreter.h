#pragma once

#include <chrono>
#include <cstring>
#include <thread>
#include <queue>
#include <vector>

#include "../args.h"
#include "../util/dynamicArray.h"
#include "entry.h"
#include "function.h"
#include "../util/garbageCollectionHeap.h"
#include "../io.h"
#include "instruction.h"
#include "instructionContainer.h"
#include "../lib/libSymbols.h"
#include "methodTree.h"
#include "../util/minHeap.h"
#include "object.h"
#include "../compiler/package.h"
#include "packagedFunctionList.h"
#include "../include/robin-map/include/tsl/robin_map.h"
#include "schedule.h"
#include "objectReference.h"
#include "variableContext.h"

#define TS_INTERPRETER_PREFIX true

using namespace std;

namespace ts {
	struct FunctionFrame {
		InstructionContainer* container;
		uint64_t instructionPointer;
		uint64_t stackPointer;
		uint64_t stackPopCount;
		PackagedFunctionList* packagedFunctionList;
		int packagedFunctionListIndex;
		MethodTreeEntry* methodTreeEntry;
		int methodTreeEntryIndex;
		bool earlyQuit;
		bool isTSSL;
		string* fileName;
		instruction::PushType pushType;
	};

	void initFunctionFrame(Interpreter* interpreter, FunctionFrame* frame);
	void onFunctionFrameRealloc(Interpreter* interpreter);
	void initSchedule(Interpreter* interpreter, Schedule** schedule);

	typedef void (*instruction_function)(class Interpreter*, Instruction&);
	
	/*
		the virtual machine manages the stack, variables, and objects. things like functions/classes/etc are handled via the engine
		class, which serves as the global state for the tokenizer/parser/interpreter system
	*/
	class Interpreter {
		friend class Engine;
		friend void onFunctionFrameRealloc(Interpreter* interpreter);
		friend VariableContext;
		friend Object;
		friend ObjectWrapper;
		friend void convertToType(Interpreter* interpreter, Entry &source, entry::EntryType type);
		friend ObjectWrapper* CreateObject(
			class ts::Interpreter* interpreter,
			bool inhibitInterpret,
			string nameSpace,
			MethodTree* methodTree,
			void* data
		);
		friend Entry* ts::sl::PARENT(Engine* engine, const char* methodName, unsigned int argc, Entry* argv, entry::EntryType* argumentTypes);
		friend bool esTick(esEnginePtr engine);
		
		public:
			Interpreter();
			~Interpreter();
			Interpreter(class Engine* engine, ParsedArguments args, bool isParallel);

			void startInterpretation(Instruction* head);
			
			void printStack();
			void warning(Instruction* instruction, const char* format, ...);

			void addSchedule(uint64_t, string functionName, Entry* arguments, uint64_t argumentCount, ObjectReference* object = nullptr);

			bool tick();
			void setTickRate(int64_t tickRate);
			void garbageCollect(unsigned int amount);
			unsigned int probeGarbage(string className);

			Entry* callFunction(string functionName, Entry* arguments, uint64_t argumentCount);
			Entry* callMethod(ObjectReference* objectReference, string methodName, Entry* arguments, uint64_t argumentCount, bool inhibitInterpret = false);

			string& getTopFileNameFromFrame();

			#ifdef TS_PUSH_SOURCE_DEBUG
			const InstructionDebug* getSourceFromEntry(Entry* entry);
			#endif

			Entry emptyEntry;

			uint64_t highestObjectId = 1;

			bool testing = false;

			class Engine* engine = nullptr;

			bool isParallel = false;
		
		#ifdef TS_INSTRUCTIONS_AS_FUNCTIONS
		public:
		#else
		private:
		#endif
			GarbageCollectionHeap<ObjectWrapper*> garbageHeap;
			
			void interpret(); // interprets the next instruction

			void enterParallel();

			bool warnings = true;
			bool showTime = false;
			
			void push(Entry &entry, instruction::PushType type, bool greedy) __attribute__((always_inline));
			void push(Entry &entry, instruction::PushType type) __attribute__((always_inline));
			void push(double number, instruction::PushType type) __attribute__((always_inline));
			void push(char* data, instruction::PushType type) __attribute__((always_inline));
			void push(Matrix* matrix, instruction::PushType type) __attribute__((always_inline));
			void push(ObjectReference* data, instruction::PushType);
			void pushEmpty(instruction::PushType type)  __attribute__((always_inline));
			void pop() __attribute__((always_inline)) {
				// TODO does this fuck everything??
				// this->stack[this->stack.head - 1].erase();
				this->stack.popped();
			};

			uint64_t ranInstructions = 0;
			uint64_t startTime = 0;

			instruction_function instructionToFunction[instruction::MATRIX_SET + 1];

			// stacks
			DynamicArray<Entry, Interpreter, true> stack = DynamicArray<Entry, Interpreter, true>(this, 100000, initEntry, nullptr);
			DynamicArray<FunctionFrame, Interpreter> frames = DynamicArray<FunctionFrame, Interpreter>(this, 100, initFunctionFrame, onFunctionFrameRealloc);
			InstructionContainer* topContainer; // the current container we're executing code from, taken from frames
			uint64_t* instructionPointer; // the current instruction pointer, taken from frames
			uint64_t stackFramePointer; // the current frame pointer
			Entry returnRegister;
			VariableContext globalContext;

			void declareObjectProperties(Function* function);
			void pushFunctionFrame(InstructionContainer* container);
			void pushFunctionFrame(
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
			);
			void popFunctionFrame() __attribute__((always_inline));
			void pushTSSLFunctionFrame(MethodTreeEntry* methodTreeEntry, int methodTreeEntryIndex);

			Entry* handleTSSLParent(string &name, unsigned int argc, Entry* argv, entry::EntryType* argumentTypes);

			// used to lookup objects
			robin_map<uint64_t, ObjectWrapper*> objects;

			// keep track of schedules
			MinHeap<Schedule*, Interpreter> schedules = MinHeap<Schedule*, Interpreter>(this, initSchedule, nullptr);

			#ifdef TS_INSTRUCTIONS_AS_FUNCTIONS
			void initInstructionToFunction();
			#endif

			#ifdef TS_PUSH_SOURCE_DEBUG
			robin_map<Entry*, const InstructionDebug*> entryToSource;
			#endif

			// parallel stuff
			thread tickThread;
			int64_t tickRate = 4;
	};
}