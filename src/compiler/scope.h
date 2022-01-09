#pragma once

#include "../include/robin-map/include/tsl/robin_map.h"
#include <string>

#include "../interpreter/instruction.h"

using namespace std;
using namespace tsl;

namespace ts {
	class Engine;

	struct BoundVariable {
		uint64_t stackIndex; // index relative to start of stack frame
		string name;
		bool isArgument;

		// character & line number of where the variable was defined
		unsigned short character;
		unsigned int line;
	};
	
	class Scope {
		public:
			BoundVariable& allocateVariable(string &variableName, bool isArgument, unsigned short character, unsigned int line);
			uint64_t allocatedSize();
		
		protected:
			uint64_t stackIndex = 0;
			robin_map<string, BoundVariable> variables;
	};
}
