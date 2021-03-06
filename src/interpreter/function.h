#pragma once

#include "instructionContainer.h"
#include "../tssl/define.h"

namespace ts {
	class Function : public InstructionContainer {
		public:
			Function(
				class Engine* engine,
				Instruction* head,
				uint64_t argumentCount,
				uint64_t variableCount,
				string functionName,
				string namespaceName = string()
			);
			Function(class Engine* engine, sl::Function* function);
			~Function();

			bool isTSSL = false;
			sl::Function* function = nullptr; // the standard library function to call if we're a standard library function
			string functionName;
			string namespaceName;
			uint64_t argumentCount;
			uint64_t variableCount;
			bool isActive = true;
			bool isPackaged;
	};
}
