#include "scope.h"

#include "../util/allocateString.h"
#include "../engine/engine.h"

using namespace ts;

BoundVariable& Scope::allocateVariable(string &variableName, bool isArgument) {
	if(this->variables.find(toLower(variableName)) == this->variables.end()) {
		this->variables[toLower(variableName)] = (BoundVariable){
			stackIndex: this->stackIndex++,
			name: toLower(variableName),
			isArgument: isArgument,
		};
	}
	return this->variables[toLower(variableName)];
}

size_t Scope::allocatedSize() {
	return this->variables.size();
}
