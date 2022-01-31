#include "function.h"

using namespace ts;

Function::Function(
	Engine* engine,
	Instruction* head,
	uint64_t argumentCount,
	uint64_t variableCount,
	string functionName,
	string namespaceName
) : InstructionContainer(engine, head)
{
	this->functionName = functionName;
	this->namespaceName = namespaceName;
	this->argumentCount = argumentCount;
	this->variableCount = variableCount;
}

Function::Function(Engine* engine, sl::Function* tsslFunction) : InstructionContainer(engine) {
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
