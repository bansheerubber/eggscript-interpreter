#include "newStatement.h"
#include "../interpreter/interpreter.h"

#include "accessStatement.h"
#include "../util/allocateString.h"
#include "assignStatement.h"
#include "booleanLiteral.h"
#include "callStatement.h"
#include "../util/cloneString.h"
#include "mathExpression.h"
#include "numberLiteral.h"
#include "stringLiteral.h"
#include "../util/stringToChars.h"
#include "../util/toLower.h"
#include "symbol.h"

bool NewStatement::ShouldParse(ts::Engine* engine) {
	return (
		engine->tokenizer->peekToken().type == NEW
		&& (
			engine->tokenizer->peekToken(1).type == SYMBOL
			|| engine->tokenizer->peekToken(1).type == LEFT_PARENTHESIS
		)
	);
}

NewStatement* NewStatement::Parse(Component* parent, ts::Engine* engine) {
	NewStatement* output = new NewStatement(engine);
	output->parent = parent;

	engine->parser->expectToken(NEW);

	// parse a symbol
	if(Symbol::ShouldParse(engine)) {
		output->className = Symbol::Parse(output, engine);
	}
	else {
		engine->parser->error("invalid new object name");
	}

	if(CallStatement::ShouldParse(engine)) {
		output->arguments = CallStatement::Parse(output, engine);
	}
	else {
		engine->parser->error("invalid new object argument list");
	}

	if(output->arguments->getElementCount() > 0) {
		engine->parser->error("arguments not supported for new object creation yet"); // TODO support this
	}

	// expect something that is not a left bracket if we got no arguments in the body of the new object statement
	if(engine->tokenizer->peekToken().type != LEFT_BRACKET) {
		return output;
	}

	engine->parser->expectToken(LEFT_BRACKET);

	// parse property declaration
	while(!engine->tokenizer->eof()) {
		// new statements can be nested, apparently
		if(NewStatement::ShouldParse(engine)) {
			output->children.push_back(NewStatement::Parse(output, engine));
			engine->parser->expectToken(SEMICOLON);
		}
		else if(
			Symbol::ShouldParse(engine)
			|| Symbol::ShouldParseAlphabeticToken(engine)
		) {
			Symbol* symbol = Symbol::Parse(output, engine);
			AccessStatement* access = AccessStatement::Parse(symbol, output, engine, true);
			symbol->parent = access;
			if(
				access->hasChain()
				|| access->hasCall()
				|| access->chainSize() > 2
				|| access->isLocalVariable()
				|| access->isGlobalVariable()
			) {
				engine->parser->error("did not expect complex property assignment '%s' in new object", access->print().c_str());
			}

			// now parse the assign statement
			if(!AssignStatement::ShouldParse(access, output, engine)) {
				engine->parser->error("expected property assignment in new object");
			}

			AssignStatement* assign = AssignStatement::Parse(access, output, engine);
			output->children.push_back(assign);
			engine->parser->expectToken(SEMICOLON);
		}
		else if(engine->tokenizer->peekToken().type == RIGHT_BRACKET) {
			break;
		}
		else {
			engine->parser->error("expected property assignment in new object");
		}
	}

	engine->parser->expectToken(RIGHT_BRACKET);

	return output;
}

string NewStatement::print() {
	string output = "new " + this->className->print() + this->arguments->print();
	if(this->children.size() != 0) {
		output += this->engine->parser->space + "{" + this->engine->parser->newLine;
		output += this->printBody();
		output += "}";
	}

	if(this->parent->requiresSemicolon(this)) {
		output += ";";
	}
	return output;
}

string NewStatement::printJSON() {
	if(this->children.size() == 0) {
		return "{\"type\":\"NEW_STATEMENT\",\"className\":" + this->className->printJSON() + ",\"arguments\":" + this->arguments->printJSON() + "}";
	}
	else {
		return "{\"type\":\"NEW_STATEMENT\",\"className\":" + this->className->printJSON() + ",\"arguments\":" + this->arguments->printJSON() + ",\"body\":" + this->printJSONBody() + "}";
	}
}

ts::InstructionReturn NewStatement::compile(ts::Engine* engine, ts::CompilationContext context) {
	ts::InstructionReturn output;

	ts::Instruction* createObject = new ts::Instruction(
		engine,
		this->getCharacterNumber(),
		this->getLineNumber()
	);
	createObject->type = ts::instruction::CREATE_OBJECT;
	createObject->createObject.canCreate = true;
	createObject->createObject.typeName = cloneString(this->className->print().c_str());

	ts::MethodTree* typeCheck = engine->getNamespace(this->className->print());
	if(typeCheck != nullptr) {
		ts::MethodTree* tree = engine->createMethodTreeFromNamespaces(this->className->print());
		createObject->createObject.methodTree = tree;
		createObject->createObject.isCached = true;
	}
	else {
		createObject->createObject.isCached = false;
	}

	output.add(createObject);

	for(Component* component: this->children) {
		if(component->getType() == ASSIGN_STATEMENT) {
			AssignStatement* assignStatement = (AssignStatement*)component;

			AccessStatementCompiled c = assignStatement->getLValue()->compileAccess(engine, context);
			ts::Instruction* instruction = c.lastAccess;

			if(assignStatement->getRValue()->getType() == NUMBER_LITERAL) {
				instruction->objectAssign.entry.setNumber(((NumberLiteral*)assignStatement->getRValue())->getNumber());
			}
			else if(assignStatement->getRValue()->getType() == BOOLEAN_LITERAL) {
				instruction->objectAssign.entry.setNumber(((BooleanLiteral*)assignStatement->getRValue())->getBoolean());
			}
			else if(assignStatement->getRValue()->getType() == STRING_LITERAL) {
				string literal = ((StringLiteral*)assignStatement->getRValue())->getString();
				instruction->objectAssign.entry = ts::Entry();
				instruction->objectAssign.entry.setString(stringToChars(literal));
			}
			else if(assignStatement->getRValue()->getType() == EMPTY_LITERAL) {
				instruction->objectAssign.entry = ts::Entry();
			}
			else if(
				assignStatement->getRValue()->getType() == MATH_EXPRESSION
				|| assignStatement->getRValue()->getType() == ACCESS_STATEMENT
				|| assignStatement->getRValue()->getType() == ASSIGN_STATEMENT
				|| assignStatement->getRValue()->getType() == NEW_STATEMENT
				|| assignStatement->getRValue()->getType() == INLINE_CONDITIONAL
				|| assignStatement->getRValue()->getType() == MATRIX_CREATION_STATEMENT
			) {
				output.add(assignStatement->getRValue()->compile(engine, context)); // TODO this is bugged out
				instruction->objectAssign.fromStack = true;
			}

			output.add(c.output);
		}
	}

	// pop from stack if needed
	if(!this->parent->shouldPushToStack(this)) {
		ts::Instruction* pop = new ts::Instruction(
			engine,
			this->getCharacterNumber(),
			this->getLineNumber()
		);
		pop->type = ts::instruction::POP;
		output.add(pop);
	}

	return output;
}
