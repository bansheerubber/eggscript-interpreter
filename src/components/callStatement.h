#pragma once

#include <vector>

#include "component.h"
#include "../parser/parser.h"
#include "../tokenizer/token.h"
#include "../tokenizer/tokenizer.h"

using namespace std;

// forward declare interpreter
namespace ts {
	class Interpreter;
}

struct CallElement {
	Component* component;
	bool isComma;
};

// parses the [...] part of an access
class CallStatement : public Component {
	public:
		using Component::Component;
		
		ComponentType getType() {
			return CALL_STATEMENT;
		}

		bool requiresSemicolon(Component* child) {
			return false;
		}

		ts::InstructionReturn compile(ts::Interpreter* interpreter);

		string print();
		static bool ShouldParse(Tokenizer* tokenizer, Parser* parser);
		static CallStatement* Parse(Component* parent, Tokenizer* tokenizer, Parser* parser);
		
		pair<
			vector<CallElement>::iterator,
			vector<CallElement>::iterator
		> getElements();

		unsigned int getElementCount();
	
	private:
		// can be literals, a mathematical statement, access statements, etc
		vector<CallElement> elements;
};