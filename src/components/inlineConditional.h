#pragma once

#include <string>

#include "component.h"
#include "../parser/parser.h"
#include "../tokenizer/token.h"
#include "../tokenizer/tokenizer.h"

// forward declare interpreter
namespace ts {
	class Interpreter;
}

class InlineConditional : public Component {
	public:
		using Component::Component;
		
		ComponentType getType() {
			return INLINE_CONDITIONAL;
		}

		bool requiresSemicolon(Component* child) {
			return false;
		}

		ts::InstructionReturn compile(ts::Interpreter* interpreter);

		string print();
		static bool ShouldParse(Tokenizer* tokenizer, Parser* parser);
		static InlineConditional* Parse(Component* leftHandSide, Component* parent, Tokenizer* tokenizer, Parser* parser);
	
	private:
		Component* leftHandSide = nullptr;
		Component* ifTrue = nullptr;
		Component* ifFalse = nullptr;
};