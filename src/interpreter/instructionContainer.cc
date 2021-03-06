#include "instructionContainer.h"
#include "debug.h"

using namespace ts;

InstructionContainer::InstructionContainer(Engine* engine) {
	this->engine = engine;
}

InstructionContainer::InstructionContainer(Engine* engine, Instruction* head) {
	this->engine = engine;
	
	// flatten instructions into array so CPU can cache instruction data types (improves performance by 20%)
	int count = 0;
	Instruction* instruction = head;
	while(instruction != nullptr) {
		Instruction* temp = instruction;
		instruction = instruction->next;
		temp->index = count; // convert next properties to flat array index
		count++;
	}

	this->array = new Instruction[count + 1]; // allocate an empty slot at the end b/c of how we do bounds checking
	this->array[count].type = instruction::INVALID_INSTRUCTION;
	this->size = count;

	instruction = head;
	count = 0;
	while(instruction != nullptr) {
		copyInstruction(this->engine, *instruction, this->array[count]);
		
		// convert jump instruction pointers to indices for flat array
		switch(instruction->type) {
			case instruction::JUMP:
			case instruction::JUMP_IF_TRUE_THEN_POP:
			case instruction::JUMP_IF_TRUE:
			case instruction::JUMP_IF_FALSE_THEN_POP:
			case instruction::JUMP_IF_FALSE: {
				this->array[count].jump.index = this->array[count].jump.instruction->index;
				break;
			}

			default: {
				break;
			}
		}

		count++;
		instruction = instruction->next;
	}

	// delete the linked list
	instruction = head;
	while(instruction != nullptr) {
		Instruction* temp = instruction;
		instruction = instruction->next;
		delete temp;
	}

	// this->print();
}

void InstructionContainer::print() {
	for(uint64_t i = 0; i < this->size; i++) {
		printf("%ld: ", i);
		PrintInstruction(this->array[i]);
	}
	printf("-----------------------------------------------------------------\n");
}

InstructionContainer::~InstructionContainer() {
	if(this->array != nullptr) {
		delete[] this->array;
		this->array = nullptr;
	}
}
