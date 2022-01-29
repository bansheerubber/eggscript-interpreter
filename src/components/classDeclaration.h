#pragma once

#include "component.h"
#include "body.h"
#include "../engine/engine.h"
#include "../parser/parser.h"
#include "../compiler/scope.h"
#include "../tokenizer/tokenizer.h"
#include "../tokenizer/token.h"

class ClassDeclaration : public Body, public ClassContext, public ts::Scope {
	friend class Parser;
	
	public:
		using Body::Body;
		
		ComponentType getType() {
			return CLASS_DECLARATION;
		}

		bool requiresSemicolon(Component* child) {
			return false;
		}

		bool shouldPushToStack(Component* child) {
			return false;
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
		static ClassDeclaration* Parse(Component* parent, ts::Engine* engine);
	
	private:
		Token token;
		string className;
		string inheritedName = "";

		vector<class AssignStatement*> declarations;
		vector<class FunctionDeclaration*> functions;
};

