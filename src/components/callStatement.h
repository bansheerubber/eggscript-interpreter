#pragma once

#include <vector>

#include "component.h"
#include "../engine/engine.h"
#include "../parser/parser.h"
#include "../compiler/scope.h"
#include "../tokenizer/token.h"
#include "../tokenizer/tokenizer.h"

using namespace std;

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

		bool shouldPushToStack(Component* child) {
			return true;
		}

		unsigned short getCharacterNumber() {
			return this->token.characterNumber;
		}

		unsigned int getLineNumber() {
			return this->token.lineNumber;
		}

		ts::InstructionReturn compile(ts::Engine* engine, ts::CompilationContext context);

		string print();
		string printJSON();
		static bool ShouldParse(ts::Engine* engine);
		static CallStatement* Parse(Component* parent, ts::Engine* engine);
		
		pair<
			vector<CallElement>::iterator,
			vector<CallElement>::iterator
		> getElements();

		CallElement &getElement(uint64_t index);

		uint64_t getElementCount();
	
	private:
		Token token;
		
		// can be literals, a mathematical statement, access statements, etc
		vector<CallElement> elements;
};
