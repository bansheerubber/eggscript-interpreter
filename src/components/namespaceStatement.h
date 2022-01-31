#pragma once

#include "component.h"
#include "../engine/engine.h"
#include "../parser/parser.h"
#include "../compiler/scope.h"
#include "../tokenizer/tokenizer.h"
#include "../tokenizer/token.h"

#include "callStatement.h"
#include "symbol.h"

class NamespaceStatement : public Component {
	public:
		using Component::Component;
		
		ComponentType getType() {
			return PARENT_STATEMENT;
		}

		bool requiresSemicolon(Component* child) {
			return false;
		}

		bool shouldPushToStack(Component* child) {
			return false;
		}

		unsigned short getCharacterNumber() {
			if(this->name == nullptr) {
				return this->parentToken.characterNumber;
			}
			return this->name->getCharacterNumber();
		}

		unsigned int getLineNumber() {
			if(this->name == nullptr) {
				return this->parentToken.lineNumber;
			}
			return this->name->getLineNumber();
		}

		ts::InstructionReturn compile(ts::Engine* engine, ts::CompilationContext context);

		string print();
		string printJSON();
		static bool ShouldParse(ts::Engine* engine);
		static NamespaceStatement* Parse(Component* parent, ts::Engine* engine);
	
	private:
		Token parentToken;
		Symbol* name = nullptr;
		Symbol* operation = nullptr;
		CallStatement* call = nullptr;
};
