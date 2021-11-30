#pragma once

#include "component.h"
#include "../engine/engine.h"
#include "../parser/parser.h"
#include "../compiler/scope.h"
#include "../tokenizer/tokenizer.h"
#include "../tokenizer/token.h"

class ClassDeclaration : public Component {
	friend class Parser;
	
	public:
		using Component::Component;
		
		ComponentType getType() {
			return CLASS_DECLARATION;
		}

		bool requiresSemicolon(Component* child) {
			return false;
		}

		bool shouldPushToStack(Component* child) {
			return false;
		}

		ts::InstructionReturn compile(ts::Engine* engine, ts::CompilationContext context);

		string print();
		string printJSON();
		static bool ShouldParse(ts::Engine* engine);
		static ClassDeclaration* Parse(Component* parent, ts::Engine* engine);
	
	private:
		string className;
		string inheritedName = "";
};

