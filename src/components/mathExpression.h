#pragma once

#include <deque>
#include <stack>
#include <string>

#include "component.h"
#include "../engine/engine.h"
#include "../parser/parser.h"
#include "../compiler/scope.h"
#include "../tokenizer/token.h"
#include "../tokenizer/tokenizer.h"

#include "../interpreter/stack.h"

enum SpecialOperator {
	INVALID_OPERATOR,
	LEFT_PARENTHESIS_OPERATOR,
	RIGHT_PARENTHESIS_OPERATOR,
	LOGICAL_NOT_OPERATOR,
	BITWISE_NOT_OPERATOR,
	MINUS_OPERATOR,
};

struct MathElement {
	Component* component;
	Token op;
	SpecialOperator specialOp;
};

struct LogicalElement {
	vector<MathElement*>* list;
	Token op;
};

struct PostfixElement {
	MathElement* element;
	deque<SpecialOperator> unary;
};

struct Operation { // used for math evaluation algorithm
	MathElement element;
	relative_stack_location stack;
};

class MathExpression : public Component {
	public:
		using Component::Component;
		
		ComponentType getType() {
			return MATH_EXPRESSION;
		}

		bool requiresSemicolon(Component* child) {
			return false;
		}

		bool shouldPushToStack(Component* child) {
			return true;
		}

		unsigned short getCharacterNumber() {
			if(this->elements.size() == 0) {
				return 0;
			}

			if(this->elements[0].component == nullptr) {
				return this->elements[0].op.characterNumber;
			}
			return this->elements[0].component->getCharacterNumber();
		}

		unsigned int getLineNumber() {
			if(this->elements.size() == 0) {
				return 0;
			}

			if(this->elements[0].component == nullptr) {
				return this->elements[0].op.lineNumber;
			}
			return this->elements[0].component->getLineNumber();
		}

		ts::InstructionReturn compile(ts::Engine* engine, ts::CompilationContext context);

		string print();
		string printJSON();
		static bool ShouldParse(Component* lvalue, ts::Engine* engine);
		static MathExpression* Parse(Component* lvalue, Component* parent, ts::Engine* engine);
		static bool IsOperator(TokenType type);
	
	private:
		vector<MathElement> elements;

		static map<TokenType, int> CreatePrecedenceMap();
		static ts::instruction::InstructionType TypeToOperator(TokenType type);
		static ts::instruction::InstructionType TypeToUnaryOperator(SpecialOperator type);
		static map<TokenType, int> Precedence;

		ts::InstructionReturn compileList(vector<MathElement*>* list, ts::Engine* engine, ts::CompilationContext context);
		vector<PostfixElement> convertToPostfix(vector<MathElement*>* list, bool prefixMod = false);
};
