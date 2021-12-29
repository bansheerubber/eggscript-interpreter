#include "function.h"

using namespace ts;

Function::Function(Instruction* head, uint64_t argumentCount, uint64_t variableCount, string functionName, string namespaceName)
	: InstructionContainer(head)
{
	this->functionName = functionName;
	this->namespaceName = namespaceName;
	this->argumentCount = argumentCount;
	this->variableCount = variableCount;
}

Function::Function(sl::Function* tsslFunction) {
	this->isTSSL = true;
	this->function = tsslFunction;
	this->functionName = tsslFunction->name;
	this->namespaceName = "";
}

Function::~Function() {
	if(this->function != nullptr) {
		delete this->function;
	}
}
