#pragma once

#include "instruction.h"

namespace ts {
	class InstructionContainer {
		friend class Interpreter;
		
		public:
			InstructionContainer(class Engine* engine);
			InstructionContainer(class Engine* engine, ts::Instruction* head);
			~InstructionContainer();
			void print(); // print all the instructions in this container
		
		protected:
			ts::Instruction* array = nullptr; // pointer to flat array in memory
			uint64_t size;
			class Engine* engine;
	};
}
