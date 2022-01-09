#include "scope.h"

#include "../util/allocateString.h"
#include "../engine/engine.h"

using namespace ts;

BoundVariable& Scope::allocateVariable(string &variableName, bool isArgument) {
	if(this->variables.find(variableName) == this->variables.end()) {
		this->variables[variableName] = (BoundVariable){
			stackIndex: this->stackIndex++,
			name: variableName,
			isArgument: isArgument,
		};
	}
	return this->variables[variableName];
}

uint64_t Scope::allocatedSize() {
	return this->variables.size();
}
