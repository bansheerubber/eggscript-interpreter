#include "matrix.h"

bool MatrixExpression::ShouldParse(ts::Engine* engine) {
	Token &token = engine->tokenizer->getToken();
	bool result = token.type == LEFT_BRACKET && Component::ShouldParse(nullptr, engine);
	engine->tokenizer->unGetToken();
	return result;
}

MatrixExpression* MatrixExpression::Parse(Component* parent, ts::Engine* engine) {
	printf("we're parsing the matrix\n");

	MatrixExpression* output = new MatrixExpression(engine);
	output->parent = parent;

	// matrices are created column by column, with rows delimited by semicolons
	engine->parser->expectToken(LEFT_BRACKET);
	unsigned int rows = 0;
	unsigned int columns = 0;
	Token lastToken;
	while(!engine->tokenizer->eof()) {
		Token &token = engine->tokenizer->peekToken();
		if(token.type == COMMA) {
			if(lastToken.type == COMMA || lastToken.type == SEMICOLON) {
				engine->parser->error("no empty entries allowed in matrix");
			}
			
			engine->tokenizer->getToken();
		}
		else if(token.type == SEMICOLON) {
			if(lastToken.type == COMMA || lastToken.type == SEMICOLON) {
				engine->parser->error("no empty entries allowed in matrix");
			}
			
			engine->tokenizer->getToken();
			output->elements.push_back(MatrixElement {
				component: nullptr,
				isRowDelimiter: true,
			});
			rows++;

			if(output->columns == 0) {
				output->columns = columns;
			}
			else if(output->columns != columns) {
				engine->parser->error("number of columns must remain same across entire matrix");
			}
			columns = 0;
		}
		else if(Component::Parse(output, engine)) {
			output->elements.push_back(MatrixElement {
				component: Component::Parse(output, engine),
				isRowDelimiter: false,
			});
			columns++;
		}
		else {
			break;
		}
		lastToken = token;
	}
	output->rows = rows + 1;

	if(lastToken.type == SEMICOLON) {
		engine->parser->error("no trailing semicolons allowed in matrix");
	}
	else if(lastToken.type == COMMA) {
		engine->parser->error("no trailing colons allowed in matrix");
	}

	engine->parser->expectToken(RIGHT_BRACKET);
	exit(1);
	return nullptr;
}

ts::InstructionReturn MatrixExpression::compile(ts::Engine* engine, ts::CompilationContext context) {
	return {};
}

string MatrixExpression::print() {
	return "";
}

string MatrixExpression::printJSON() {
	return "";
}
