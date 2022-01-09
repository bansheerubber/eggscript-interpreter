#include "instruction.h"

#include "../util/allocateString.h"
#include "../engine/engine.h"
#include "../tokenizer/tokenizer.h"

ts::Instruction::Instruction(Engine* engine, unsigned short character, unsigned int line) {
	this->type = instruction::INVALID_INSTRUCTION;
	this->pushType = instruction::STACK;
	this->next = nullptr;

	engine->addInstructionDebug(this, engine->tokenizer->symbolicFileName, character, line);
}

void ts::copyInstruction(Engine* engine, Instruction &source, Instruction &destination) {
	destination.type = source.type;
	destination.pushType = source.pushType;
	destination.index = source.index;

	engine->swapInstructionDebug(&source, &destination);

	switch(source.type) {
		## instruction_generator.py instruction.cc

		default: {
			break;
		}
	}
}