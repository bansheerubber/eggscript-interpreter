#pragma once

#include <vector>

#include "component.h"
#include "../engine/engine.h"
#include "../parser/parser.h"
#include "../compiler/scope.h"
#include "../tokenizer/token.h"
#include "../tokenizer/tokenizer.h"

using namespace std;

// parses the [...] part of an access
class ArrayStatement : public Component {
	public:
		using Component::Component;
		
		ComponentType getType() {
			return ARRAY_STATEMENT;
		}

		bool requiresSemicolon(Component* child) {
			return false;
		}

		bool shouldPushToStack(Component* child) {
			return true;
		}

		ts::InstructionReturn compile(ts::Engine* engine, ts::CompilationContext context);

		string print();
		string printJSON();
		static bool ShouldParse(ts::Engine* engine);
		static ArrayStatement* Parse(Component* parent, ts::Engine* engine);
	
	private:
		Component* component = nullptr;
};
