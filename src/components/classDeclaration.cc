#include "classDeclaration.h"

#include "accessStatement.h"
#include "assignStatement.h"
#include "booleanLiteral.h"
#include "functionDeclaration.h"
#include "numberLiteral.h"
#include "stringLiteral.h"
#include "../util/stringToChars.h"
#include "sourceFile.h"
#include "symbol.h"

bool ClassDeclaration::ShouldParse(ts::Engine* engine) {
	return engine->tokenizer->peekToken().type == CLASS && engine->tokenizer->peekToken(1).type == SYMBOL;
}

ClassDeclaration* ClassDeclaration::Parse(Component* parent, ts::Engine* engine) {
	if(parent->getType() != SOURCE_FILE) {
		engine->parser->error("cannot declare scoped classes");
	}
	
	ClassDeclaration* output = new ClassDeclaration(engine);

	output->token = engine->parser->expectToken(CLASS);
	
	Token symbol = engine->parser->expectToken(SYMBOL);
	output->name = output->className = symbol.lexeme;

	if(engine->tokenizer->peekToken().type == INHERITS) {
		engine->parser->expectToken(INHERITS);

		Token symbol = engine->parser->expectToken(SYMBOL);
		output->inheritedName = symbol.lexeme;
	}

	((SourceFile*)parent)->classDeclarations.push_back(output);

	// check to see if there's a body to the class delcaration then parse it
	if(engine->tokenizer->peekToken().type != LEFT_BRACKET) {
		return output;
	}

	engine->tokenizer->getToken(); // absorb left bracket

	while(!engine->tokenizer->eof()) {
		if(FunctionDeclaration::ShouldParse(engine)) {
			output->functions.push_back(FunctionDeclaration::Parse(output, engine));
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
				|| access->chainSize() > 1
				|| access->isLocalVariable()
				|| access->isGlobalVariable()
			) {
				engine->parser->error("did not expect complex property assignment '%s' in class declaration", access->print().c_str());
			}

			// now parse the assign statement
			if(!AssignStatement::ShouldParse(access, output, engine)) {
				engine->parser->error("expected property assignment in class declaration");
			}

			AssignStatement* assign = AssignStatement::Parse(access, output, engine);
			output->children.push_back(assign);
			output->declarations.push_back(assign);
			engine->parser->expectToken(SEMICOLON);
		}
		else if(engine->tokenizer->peekToken().type == RIGHT_BRACKET) {
			break;
		}
		else {
			engine->parser->error("expected property assignment in class declaration");
		}
	}

	engine->parser->expectToken(RIGHT_BRACKET);

	return output;
}

string ClassDeclaration::print() {
	return "";
}

string ClassDeclaration::printJSON() {
	return "";
}

ts::InstructionReturn ClassDeclaration::compile(ts::Engine* engine, ts::CompilationContext context) {
	ts::MethodTree* tree = this->engine->getNamespace(this->className);
	if(tree == nullptr) { // do not compile if our class did not parse correctly
		return {};
	}

	this->allocateVariable("this", false, this->getCharacterNumber(), this->getLineNumber());
	
	for(FunctionDeclaration* component: this->functions) {
		component->compile(engine, (ts::CompilationContext){
			loop: nullptr,
			package: nullptr,
			scope: nullptr,
			classContext: this,
		});
	}

	ts::InstructionReturn propertyOutput; // instance variable initialization
	for(Component* component: this->declarations) {
		if(component->getType() == ASSIGN_STATEMENT) {
			AssignStatement* assignStatement = (AssignStatement*)component;

			AccessStatementCompiled c = assignStatement->getLValue()->compileAccess(engine, (ts::CompilationContext){
				loop: nullptr,
				package: nullptr,
				scope: nullptr,
				classContext: this,
			});
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
				propertyOutput.add(assignStatement->getRValue()->compile(engine, (ts::CompilationContext){
					loop: nullptr,
					package: nullptr,
					scope: this,
					classContext: this,
				}));

				if(this->allocatedSize() > 1) {
					engine->parser->error("can not use local variables other than '%%this' in class declaration");
				}

				instruction->objectAssign.fromStack = true;
				instruction->objectAssign.newBodyPatch = 1;
			}

			propertyOutput.add(c.output);
		}
	}

	// property declaration looks like a function to the interpreter
	ts::Instruction* returnInstruction = new ts::Instruction(
		engine,
		this->getCharacterNumber(),
		this->getLineNumber()
	);
	returnInstruction->type = ts::instruction::RETURN_NO_VALUE;
	propertyOutput.add(returnInstruction);

	tree->definePropertyDeclaration(engine, propertyOutput);
	
	return {};
}
