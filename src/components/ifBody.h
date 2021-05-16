#pragma once

#include <string>
#include <sstream>
#include <vector>

#include "component.h"
#include "body.h"
#include "../parser/parser.h"
#include "../tokenizer/token.h"
#include "../tokenizer/tokenizer.h"

using namespace std;

// forward declare interpreter
namespace ts {
	class Interpreter;
}

class IfBody : public Body {
	public:
		using Body::Body;
		
		ComponentType getType() {
			return IF_STATEMENT;
		}

		bool requiresSemicolon(Component* child) {
			if(child == conditional) {
				return false;
			}
			else {
				return true;
			}
		}

		ts::InstructionReturn compile(ts::Interpreter* interpreter);

		string print();
		static bool ShouldParse(Tokenizer* tokenizer, class Parser* parser);
		static IfBody* Parse(Body* body, Tokenizer* tokenizer, class Parser* parser);

		Body* next = nullptr;
	
	private:
		Component* conditional = nullptr;
};