#pragma once

#include "component.h"
#include "../engine/engine.h"
#include "../parser/parser.h"
#include "../compiler/scope.h"
#include "../tokenizer/token.h"
#include "../tokenizer/tokenizer.h"

struct MatrixElement {
	Component* component;
	bool isRowDelimiter;
};

class MatrixExpression : public Component {
	public:
		using Component::Component;
		
		ComponentType getType() {
			return MATRIX_CREATION_STATEMENT;
		}

		bool requiresSemicolon(Component* child) {
			return false;
		}

		bool shouldPushToStack(Component* child) {
			return true;
		}

		unsigned short getCharacterNumber() {
			if(this->elements.size() == 0 || this->elements[0].component == nullptr) {
				return 0;
			}
			return this->elements[0].component->getCharacterNumber();
		}

		unsigned int getLineNumber() {
			if(this->elements.size() == 0 || this->elements[0].component == nullptr) {
				return 0;
			}
			return this->elements[0].component->getLineNumber();
		}

		ts::InstructionReturn compile(ts::Engine* engine, ts::CompilationContext context);

		string print();
		string printJSON();
		static bool ShouldParse(ts::Engine* engine);
		static MatrixExpression* Parse(Component* parent, ts::Engine* engine);
	
	private:
		unsigned int rows = 0;
		unsigned int columns = 0;
		vector<MatrixElement> elements;
};

