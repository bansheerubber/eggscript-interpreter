#pragma once

#include "entry.h"
#include "stack.h"

namespace ts {
	struct InstructionSource {
		string fileName;
	};
	
	struct InstructionDebug {
		InstructionSource* commonSource = nullptr;
		unsigned short character;
		unsigned int line;
	};

	namespace instruction {
		enum PushType : short {
			STACK = -1,
			RETURN_REGISTER,
		};

		// if we're a load instruction, store, jump, call, etc
		enum InstructionSubType {
			SUBTYPE_NOOP,
			SUBTYPE_STACK,
			SUBTYPE_MATH,
			SUBTYPE_ASSIGN,
			SUBTYPE_ACCESS,
			SUBTYPE_BRANCH,
			SUBTYPE_JUMP,
			SUBTYPE_CALL,
			SUBTYPE_RETURN,
			SUBTYPE_END = SUBTYPE_RETURN,
		};
		
		enum InstructionType : unsigned short {
			INVALID_INSTRUCTION, // an instruction with an invalid type will cause the interpreter to stop
			NOOP,
			PUSH, // push a literal value onto the stack, specifies type
			POP, // pop the top of the stack
			JUMP, // jump to a particular instruction
			JUMP_IF_TRUE, // jump to particular insturction if top element on stack is true, pops the element
			JUMP_IF_TRUE_THEN_POP,
			JUMP_IF_FALSE, // jump to particular insturction if top element on stack is false, pops the element
			JUMP_IF_FALSE_THEN_POP,
			MATH_ADDITION,
			MATH_SUBTRACT,
			MATH_MULTIPLY,
			MATH_DIVISION,
			MATH_MODULUS,
			MATH_SHIFT_LEFT,
			MATH_SHIFT_RIGHT,
			MATH_EQUAL,
			MATH_NOT_EQUAL,
			MATH_LESS_THAN_EQUAL,
			MATH_GREATER_THAN_EQUAL,
			MATH_LESS_THAN,
			MATH_GREATER_THAN,
			MATH_BITWISE_AND,
			MATH_BITWISE_OR,
			MATH_BITWISE_XOR,
			MATH_APPEND,
			MATH_SPC,
			MATH_TAB,
			MATH_NL,
			MATH_DOT_PRODUCT,
			MATH_CROSS_PRODUCT,
			UNARY_BITWISE_NOT,
			UNARY_LOGICAL_NOT,
			UNARY_NEGATE,
			LOCAL_ASSIGN_EQUAL,
			LOCAL_ASSIGN_INCREMENT,
			LOCAL_ASSIGN_DECREMENT,
			LOCAL_ASSIGN_PLUS,
			LOCAL_ASSIGN_MINUS,
			LOCAL_ASSIGN_ASTERISK,
			LOCAL_ASSIGN_SLASH,
			LOCAL_ASSIGN_MODULUS,
			LOCAL_ASSIGN_SHIFT_LEFT,
			LOCAL_ASSIGN_SHIFT_RIGHT,
			LOCAL_ASSIGN_BITWISE_AND,
			LOCAL_ASSIGN_BITWISE_XOR,
			LOCAL_ASSIGN_BITWISE_OR,
			GLOBAL_ASSIGN_EQUAL,
			GLOBAL_ASSIGN_INCREMENT,
			GLOBAL_ASSIGN_DECREMENT,
			GLOBAL_ASSIGN_PLUS,
			GLOBAL_ASSIGN_MINUS,
			GLOBAL_ASSIGN_ASTERISK,
			GLOBAL_ASSIGN_SLASH,
			GLOBAL_ASSIGN_MODULUS,
			GLOBAL_ASSIGN_SHIFT_LEFT,
			GLOBAL_ASSIGN_SHIFT_RIGHT,
			GLOBAL_ASSIGN_BITWISE_AND,
			GLOBAL_ASSIGN_BITWISE_XOR,
			GLOBAL_ASSIGN_BITWISE_OR,
			OBJECT_ASSIGN_EQUAL,
			OBJECT_ASSIGN_INCREMENT,
			OBJECT_ASSIGN_DECREMENT,
			OBJECT_ASSIGN_PLUS,
			OBJECT_ASSIGN_MINUS,
			OBJECT_ASSIGN_ASTERISK,
			OBJECT_ASSIGN_SLASH,
			OBJECT_ASSIGN_MODULUS,
			OBJECT_ASSIGN_SHIFT_LEFT,
			OBJECT_ASSIGN_SHIFT_RIGHT,
			OBJECT_ASSIGN_BITWISE_AND,
			OBJECT_ASSIGN_BITWISE_XOR,
			OBJECT_ASSIGN_BITWISE_OR,
			ARRAY_ASSIGN_EQUAL,
			ARRAY_ASSIGN_INCREMENT,
			ARRAY_ASSIGN_DECREMENT,
			ARRAY_ASSIGN_PLUS,
			ARRAY_ASSIGN_MINUS,
			ARRAY_ASSIGN_ASTERISK,
			ARRAY_ASSIGN_SLASH,
			ARRAY_ASSIGN_MODULUS,
			ARRAY_ASSIGN_SHIFT_LEFT,
			ARRAY_ASSIGN_SHIFT_RIGHT,
			ARRAY_ASSIGN_BITWISE_AND,
			ARRAY_ASSIGN_BITWISE_XOR,
			ARRAY_ASSIGN_BITWISE_OR,
			LOCAL_ACCESS, // gets the value of a local variable and puts it on the stack
			GLOBAL_ACCESS,
			OBJECT_ACCESS,
			ARRAY_ACCESS,
			CALL_FUNCTION_UNLINKED,
			CALL_FUNCTION, // call a globally scoped function
			CALL_NAMESPACE_FUNCTION_UNLINKED,
			CALL_NAMESPACE_FUNCTION,
			CALL_PARENT,
			CALL_OBJECT_UNLINKED,
			CALL_OBJECT, // call a function on an object
			RETURN_NO_VALUE,
			MOVE_THEN_RETURN, // move the last element from the stack onto the return register, then push it back once the frame is cleared
			RETURN, // return from a function without returning a value
			POP_ARGUMENTS, // pop x arguments from the stack, x being obtained from the top of the stack
			CREATE_OBJECT_UNLINKED,
			CREATE_OBJECT, // create an object
			MATRIX_CREATE,
			MATRIX_SET,
		};

		enum AssignOperations : unsigned short {
			INVALID_ASSIGN,
			EQUALS,
			INCREMENT,
			DECREMENT,
			PLUS_EQUALS,
			MINUS_EQUALS,
			ASTERISK_EQUALS,
			SLASH_EQUALS,
			MODULUS_EQUALS,
			SHIFT_LEFT_EQUALS,
			SHIFT_RIGHT_EQUALS,
			BITWISE_AND_EQUALS,
			BITWISE_XOR_EQUALS,
			BITWISE_OR_EQUALS,
		};
	}
	
	// instructions form a linked list
	struct Instruction {
		instruction::InstructionType type;
		instruction::PushType pushType;
		Instruction* next; // next instruction in linked list
		uint64_t index; // instruction's index in flat array

		// TODO-if: stuff that we can do to reduce the amount of if statements in the VM, involves separating out functionality into seperate instructions
		// function call notes: replace un-cached functions with warning message instruction?
		//   - call instructions that do not have a valid linked entry use a `function_call_error` instruction that prints out a warning message
		//   - keep track of the function_call_error instructions in a list
		//   - every time we execute new code that has function definitions, look at the list and determine if we can finally link the instructions
		//   - if we can, then replace the tracked instruction's type with a proper `function_call` instruction type
		union {
			struct {
				Entry entry;
			} push;

			struct {
				union {
					Instruction* instruction;
					uint64_t index;
				};
			} jump;

			// TODO-if maybe seperate instructions for each combination of l/r entry/stack indices? probably bad idea
			struct {
				Entry lvalueEntry;
				Entry rvalueEntry;
				int lvalueStackIndex;
				int rvalueStackIndex;
			} mathematics;

			struct {
				int stackIndex;
			} unaryMathematics;

			struct {
				const char* destination;
				uint64_t hash;
				bool fromStack;
				bool pushResult;
				Entry entry;
				int stackIndex;
			} localAssign;

			struct {
				const char* destination;
				uint64_t hash;
				bool fromStack;
				bool pushResult;
				Entry entry;
			} globalAssign;

			struct {
				const char* destination;
				uint64_t hash;
				bool fromStack;
				bool pushResult;
				Entry entry;
				bool popObject;
				char newBodyPatch; // TODO figure out a better way to fix the stack behavior
			} objectAssign;

			struct {
				char blank[sizeof(const char*) + sizeof(uint64_t)];
				bool fromStack;
				bool pushResult;
				Entry entry;
			} arrayAssign;

			struct {
				const char* source;
				uint64_t hash;
				int stackIndex;
			} localAccess;

			struct {
				const char* source;
				uint64_t hash;
			} globalAccess;

			struct {
				const char* source;
				uint64_t hash;
			} objectAccess;

			struct {
				const char* name;
				class PackagedFunctionList* cachedFunctionList;
			} callFunction;

			struct {
				const char* name;
				const char* nameSpace;
				class MethodTreeEntry* cachedEntry;
			} callNamespaceFunction;

			struct {
				const char* name;
				uint64_t cachedIndex;
			} callObject;

			struct {
				const char* typeName;
				class MethodTree* methodTree;
			}	createObject;

			struct {
				uint64_t argumentCount;
			} popArguments;

			struct {
				unsigned int rows;
				unsigned int columns;
			} matrixCreate;

			struct {
				unsigned int row;
				unsigned int column;
			} matrixSet;
		};

		Instruction() {
			this->type = instruction::INVALID_INSTRUCTION;
			this->pushType = instruction::STACK;
			this->next = nullptr;
		}

		Instruction(class Engine* engine, unsigned short character, unsigned int line);

		~Instruction() {
			
		}
	};

	void copyInstruction(class Engine* engine, Instruction &source, Instruction &destination);

	inline instruction::InstructionSubType typeToSubType(instruction::InstructionType type) {
		if(type == instruction::INVALID_INSTRUCTION || type == instruction::NOOP) {
			return instruction::SUBTYPE_NOOP;
		}
		else if(type >= instruction::PUSH && type <= instruction::POP) {
			return instruction::SUBTYPE_STACK;
		}
		else if(type == instruction::JUMP) {
			return instruction::SUBTYPE_JUMP;
		}
		else if(type >= instruction::JUMP_IF_TRUE && type <= instruction::JUMP_IF_FALSE_THEN_POP) {
			return instruction::SUBTYPE_BRANCH;
		}
		else if(type >= instruction::MATH_ADDITION && type <= instruction::UNARY_NEGATE) {
			return instruction::SUBTYPE_MATH;
		}
		else if(type >= instruction::LOCAL_ASSIGN_EQUAL && type <= instruction::ARRAY_ASSIGN_BITWISE_OR) {
			return instruction::SUBTYPE_ASSIGN;
		}
		else if(type >= instruction::LOCAL_ACCESS && type <= instruction::ARRAY_ACCESS) {
			return instruction::SUBTYPE_ACCESS;
		}
		else if(type >= instruction::CALL_FUNCTION_UNLINKED && type <= instruction::CALL_OBJECT) {
			return instruction::SUBTYPE_CALL;
		}
		else if(type >= instruction::RETURN_NO_VALUE && type <= instruction::RETURN) {
			return instruction::SUBTYPE_RETURN;
		}
		else if(type >= instruction::POP_ARGUMENTS && type <= instruction::MATRIX_CREATE) {
			return instruction::SUBTYPE_STACK;
		}
		else if(type == instruction::MATRIX_SET) {
			return instruction::SUBTYPE_ASSIGN;
		}
		return instruction::SUBTYPE_NOOP;
	}

	struct InstructionReturn {
		Instruction* first; // first instruction in mini linked list
		Instruction* last; // last instruction in mini linked list

		InstructionReturn() {
			this->first = nullptr;
			this->last = nullptr;
		}

		InstructionReturn(Instruction* first, Instruction* last) {
			this->first = first;
			this->last = last;
		}

		void add(Instruction* instruction) {
			if(instruction != nullptr) {
				if(this->first == nullptr) {
					this->first = instruction;
					this->last = instruction;
				}
				else {
					this->last->next = instruction;
					this->last = instruction;
				}
			}
		}

		void add(InstructionReturn compiled) {
			if(compiled.first != nullptr && compiled.last != nullptr) {
				if(this->first == nullptr) {
					this->first = compiled.first;
					this->last = compiled.last;
				}
				else {
					this->last->next = compiled.first;
					this->last = compiled.last;
				}
			}
		}

		void addFirst(Instruction* instruction) {
			if(instruction != nullptr) {
				if(this->first == nullptr) {
					this->first = instruction;
					this->last = instruction;
				}
				else {
					instruction->next = this->first;
					this->first = instruction;
				}
			}
		}

		void addFirst(InstructionReturn compiled) {
			if(compiled.first != nullptr && compiled.last != nullptr) {
				if(this->first == nullptr) {
					this->first = compiled.first;
					this->last = compiled.last;
				}
				else {
					compiled.last->next = this->first;
					this->first = compiled.first;
				}
			}
		}
	};
}