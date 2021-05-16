#pragma once

#include <chrono>
#include <stack>
#include <vector>

#include "entry.h"
#include "instruction.h"
#include "../io.h"
#include "stack.h"
#include "variableContext.h"
#include "instructionContainer.h"

using namespace std;

namespace ts {
	class Interpreter {
		public:
			Interpreter();
			~Interpreter();

			void startInterpretation(Instruction* head);
			
			void printInstruction(Instruction &instruction);
			void printStack();
		
		private:
			void interpret(); // interprets the next instruction
			
			void push(Entry &entry);
			void push(double number);
			void pop();

			int ranInstructions = 0;
			chrono::high_resolution_clock::time_point startTime;

			Entry stack[1024];
			stack_location stackPointer = 0; // points to the next valid location on the stack (stackPointer - 1 == top of stack)
			
			StackFrame frames[1024];
			unsigned int framePointer = 0;
			StackFrame* topFrame;

			void pushVariableContext();
			void popVariableContext();
			VariableContext &getTopVariableContext();
			VariableContext contexts[256];
			VariableContext* topContext;
			int contextPointer = 0;
			friend string VariableContext::computeVariableString(Instruction &instruction, string &variable);

			InstructionContainer* instructions;
			unsigned long instructionPointer = 0;
	};
}