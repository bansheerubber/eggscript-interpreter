#include "mathExpression.h"
#include "../interpreter/interpreter.h"

map<TokenType, int> MathExpression::Precedence = MathExpression::CreatePrecedenceMap();

bool MathExpression::IsOperator(TokenType type) {
	return type == PLUS
		|| type == MINUS
		|| type == SLASH
		|| type == ASTERISK
		|| type == MODULUS
		|| type == NOT_EQUAL
		|| type == EQUAL
		|| type == STRING_EQUAL
		|| type == STRING_NOT_EQUAL
		|| type == LESS_THAN_EQUAL
		|| type == GREATER_THAN_EQUAL
		|| type == LESS_THAN
		|| type == GREATER_THAN
		|| type == LOGICAL_AND
		|| type == LOGICAL_OR
		|| type == SHIFT_LEFT
		|| type == SHIFT_RIGHT
		|| type == BITWISE_AND
		|| type == BITWISE_OR
		|| type == BITWISE_XOR
		|| type == APPEND
		|| type == SPC
		|| type == TAB
		|| type == NL;
}

bool MathExpression::ShouldParse(Component* lvalue, Tokenizer* tokenizer, Parser* parser) {
	if(lvalue == nullptr) {
		return (
			(
				(
					NumberLiteral::ShouldParse(tokenizer, parser)
					|| StringLiteral::ShouldParse(tokenizer, parser)
					|| BooleanLiteral::ShouldParse(tokenizer, parser)
				)
				&& MathExpression::IsOperator(tokenizer->peekToken(1).type)
			)
			|| tokenizer->peekToken().type == LEFT_PARENTHESIS
			|| tokenizer->peekToken().type == LOGICAL_NOT
			|| tokenizer->peekToken().type == BITWISE_NOT
			|| tokenizer->peekToken().type == MINUS
		);
	}
	else {
		return MathExpression::IsOperator(tokenizer->peekToken().type);
	}
}

MathExpression* MathExpression::Parse(Component* lvalue, Component* parent, Tokenizer* tokenizer, Parser* parser) {
	MathExpression* output = new MathExpression(parser);
	output->parent = parent;
	
	if(lvalue != nullptr) {
		output->elements.push_back({
			component: lvalue,
		});
		lvalue->setParent(output);
	}

	// if we start with a left parenthesis, make note of it
	bool parenthesis = false;
	if(tokenizer->peekToken().type == LEFT_PARENTHESIS) {
		tokenizer->getToken(); // absorb parenthesis
		output->elements.push_back({
			component: nullptr,
			specialOp: LEFT_PARENTHESIS_OPERATOR,
		});
		parenthesis = true;
	}

	bool expectingOperator = lvalue != nullptr;
	while(!tokenizer->eof()) {
		if(expectingOperator) {
			Token token = tokenizer->getToken();
			if(MathExpression::IsOperator(token.type)) {
				output->elements.push_back({
					component: nullptr,
					op: token,
				});
				expectingOperator = false;
			}
			else if(parenthesis && token.type == RIGHT_PARENTHESIS) {
				output->elements.push_back({
					component: nullptr,
					specialOp: RIGHT_PARENTHESIS_OPERATOR,
				});
				break; // quit out of loop since our statement is now over
			}
			else { // if we see an end to the statement, quit out
				tokenizer->unGetToken();
				break;
			}
		}
		else {
			if(tokenizer->peekToken().type == LEFT_PARENTHESIS) { // nested math expression
				output->elements.push_back({
					component: Component::AfterParse(
						MathExpression::Parse(nullptr, output, tokenizer, parser),
						output,
						tokenizer,
						parser
					),
				});
				expectingOperator = true;
			}
			else if(tokenizer->peekToken().type == LOGICAL_NOT) {
				tokenizer->getToken(); // absorb token
				output->elements.push_back({
					component: nullptr,
					specialOp: LOGICAL_NOT_OPERATOR,
				});
				expectingOperator = false;
			}
			else if(tokenizer->peekToken().type == BITWISE_NOT) {
				tokenizer->getToken(); // absorb token
				output->elements.push_back({
					component: nullptr,
					specialOp: BITWISE_NOT_OPERATOR,
				});
				expectingOperator = false;
			}
			else if(tokenizer->peekToken().type == MINUS) {
				tokenizer->getToken(); // absorb token
				output->elements.push_back({
					component: nullptr,
					specialOp: MINUS_OPERATOR,
				});
				expectingOperator = false;
			}
			// handles literals, accesses, calls, etc
			else if(Component::ShouldParse(output, tokenizer, parser)) {
				output->elements.push_back({
					component: Component::Parse(output, tokenizer, parser),
				});
				expectingOperator = true;
			}
			else {
				break; // quit out since we didn't recognize something
			}
		}
	}

	// under this circumstance, we need to check if there is more math expression to parse
	if(parenthesis && parent->getType() != MATH_EXPRESSION && MathExpression::ShouldParse(output, tokenizer, parser)) {
		return MathExpression::Parse(output, parent, tokenizer, parser);
	}
	else if(parenthesis && output->elements.back().specialOp != RIGHT_PARENTHESIS_OPERATOR) {
		parser->error("unclosed parenthesis");
	}

	if(expectingOperator == false) {
		parser->error("was expecting evaluatable expression, number literal, boolean literal, or string literal");
	}

	return output;
}

string MathExpression::print() {
	string output;
	MathElement lastElement;
	for(MathElement element: this->elements) {
		if(element.component != nullptr) {
			output += element.component->print();
		}
		else if(element.specialOp == LEFT_PARENTHESIS_OPERATOR) {
			output += "(";
		}
		else if(element.specialOp == RIGHT_PARENTHESIS_OPERATOR) {
			output += ")";
		}
		else if(element.specialOp == LOGICAL_NOT_OPERATOR) {
			output += "!";
		}
		else if(element.specialOp == BITWISE_NOT_OPERATOR) {
			output += "~";
		}
		else if(element.specialOp == MINUS_OPERATOR) {
			if(this-parser->minified && lastElement.op.type == MINUS) {
				output += " -";
			}
			else {
				output += "-";
			}
		}
		else {
			if(this->parser->minified) {
				if(element.op.type == SPC || element.op.type == TAB || element.op.type == NL || element.op.type == MODULUS) {
					output += " " + element.op.lexeme + " ";
					continue;
				}
			}
			output += this->parser->space + element.op.lexeme + this->parser->space;
		}
		lastElement = element;
	}

	if(this->parent->requiresSemicolon(this)) {
		output += ";";
	}

	return output;
}

map<TokenType, int> MathExpression::CreatePrecedenceMap() {
	map<TokenType, int> output;
	output[PLUS] = 1;
	output[MINUS] = 1;
	output[ASTERISK] = 2;
	output[SLASH] = 2;
	return output;
}

ts::instruction::MathematicsOperator MathExpression::TypeToOperator(TokenType type) {
	switch(type) {
		case PLUS:
			return ts::instruction::ADDITION;
		case MINUS:
			return ts::instruction::SUBTRACT;
		case ASTERISK:
			return ts::instruction::MULTIPLY;
		case SLASH:
			return ts::instruction::DIVISION;
		case EQUAL:
			return ts::instruction::EQUAL;
		case STRING_EQUAL:
			return ts::instruction::STRING_EQUAL;
		case STRING_NOT_EQUAL:
			return ts::instruction::STRING_NOT_EQUAL;
		case LESS_THAN_EQUAL:
			return ts::instruction::LESS_THAN_EQUAL;
		case GREATER_THAN_EQUAL:
			return ts::instruction::GREATER_THAN_EQUAL;
		case LESS_THAN:
			return ts::instruction::LESS_THAN;
		case GREATER_THAN:
			return ts::instruction::GREATER_THAN;
		case BITWISE_AND:
			return ts::instruction::BITWISE_AND;
		case BITWISE_OR:
			return ts::instruction::BITWISE_OR;
		case BITWISE_XOR:
			return ts::instruction::BITWISE_XOR;
		case APPEND:
			return ts::instruction::APPEND;
		case SPC:
			return ts::instruction::SPC;
		case TAB:
			return ts::instruction::TAB;

		default:
			return ts::instruction::INVALID_OPERATOR;
	}
}

vector<MathElement*> MathExpression::convertToPostfix() {
	vector<MathElement*> postfix;
	stack<MathElement*> operatorStack;

	for(MathElement &element: this->elements) {
		if(element.specialOp == LEFT_PARENTHESIS_OPERATOR || element.specialOp == RIGHT_PARENTHESIS_OPERATOR) {
			continue;
		}
		
		if(element.component != nullptr) { // if the element is an operand, push it to the stack
			postfix.push_back(&element);
		}
		else {
			if(
				operatorStack.size() == 0
				|| MathExpression::Precedence[operatorStack.top()->op.type] < MathExpression::Precedence[element.op.type]
			) {
				operatorStack.push(&element);
			}
			else {
				// push operators onto the final stack if the operators on the operator stack are greater precedence than
				// the current operator
				while(
					operatorStack.size() != 0
					&& MathExpression::Precedence[operatorStack.top()->op.type] >= MathExpression::Precedence[element.op.type]
				) {
					postfix.push_back(operatorStack.top());
					operatorStack.pop();
				}
				operatorStack.push(&element);
			}
		}
	}

	// push rest of operators onto the stack
	while(operatorStack.size() != 0) {
		postfix.push_back(operatorStack.top());
		operatorStack.pop();
	}

	return postfix;
}

ts::InstructionReturn MathExpression::compile(ts::Interpreter* interpreter) {
	ts::InstructionReturn output;

	vector<MathElement*> postfix = this->convertToPostfix();

	MathElement* lastLValue = nullptr;
	MathElement* lastRValue = nullptr;
	for(MathElement* element: postfix) {
		if(element->component == nullptr) { // handle an operator
			ts::Instruction* instruction = new ts::Instruction();
			instruction->type = ts::instruction::MATHEMATICS;
			instruction->mathematics.lvalueEntry = ts::Entry();
			instruction->mathematics.lvalueEntry.type = ts::entry::INVALID;
			instruction->mathematics.rvalueEntry = ts::Entry();
			instruction->mathematics.rvalueEntry.type = ts::entry::INVALID;
			instruction->mathematics.operation = MathExpression::TypeToOperator(element->op.type);
			
			if(
				lastLValue != nullptr
				&& lastLValue->component != nullptr
				&& lastLValue->component->getType() == NUMBER_LITERAL
			) { // potential literal l-value
				instruction->mathematics.lvalueEntry.setNumber(((NumberLiteral*)lastLValue->component)->getNumber());
			}

			if(
				lastRValue != nullptr && lastRValue->component != nullptr
				&& lastRValue->component->getType() == NUMBER_LITERAL
			) { // potential literal r-value
				instruction->mathematics.rvalueEntry.setNumber(((NumberLiteral*)lastRValue->component)->getNumber());
			}

			output.add(instruction);

			lastLValue = nullptr; // reset lvalues so we can correctly detect literal values later
			lastRValue = nullptr;
		}
		else if(element->component->getType() != NUMBER_LITERAL) { // handle an operand
			output.add(element->component->compile(interpreter));
		}

		lastLValue = lastRValue;
		lastRValue = element;
	}

	return output;
}