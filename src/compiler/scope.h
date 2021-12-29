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
	};
	
	class Scope {
		public:
			BoundVariable& allocateVariable(string &variableName, bool isArgument = false);
			uint64_t allocatedSize();
		
		protected:
			uint64_t stackIndex = 0;
			robin_map<string, BoundVariable> variables;
	};
}
