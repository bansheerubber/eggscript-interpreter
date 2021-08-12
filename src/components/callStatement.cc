#include "callStatement.h"
#include "../interpreter/interpreter.h"

bool CallStatement::ShouldParse(Tokenizer* tokenizer, Parser* parser) {
	return tokenizer->peekToken().type == LEFT_PARENTHESIS;
}

CallStatement* CallStatement::Parse(Component* parent, Tokenizer* tokenizer, Parser* parser) {
	CallStatement* output = new CallStatement(parser);
	output->parent = parent;
	
	parser->expectToken(LEFT_PARENTHESIS);

	bool expectingComma = false;
	while(!tokenizer->eof()) {
		if(!expectingComma) {
			if(Component::ShouldParse(output, tokenizer, parser)) {
				output->elements.push_back((CallElement){
					component: Component::Parse(output, tokenizer, parser),
				});
				expectingComma = true;
			}
			else if(tokenizer->peekToken().type == RIGHT_PARENTHESIS) { // there's no arguments
				tokenizer->getToken(); // absorb right parenthesis
				break;
			}
			else {
				parser->error("could not parse component in argument list");
			}
		}
		else {
			Token token = parser->expectToken(COMMA, RIGHT_PARENTHESIS);
			if(token.type == COMMA) {
				output->elements.push_back((CallElement){
					isComma: true,
				});
				expectingComma = false;
			}
			else { // we got the right parenthesis, quit out
				break;
			}
		}
	}
	
	return output;
}

string CallStatement::print() {
	string output = "(";
	for(CallElement element: this->elements) {
		if(element.component != nullptr) {
			output += element.component->print();
		}
		else if(element.isComma) {
			output += "," + this->parser->space;
		}
	}
	output += ")";
	return output;
}

string CallStatement::printJSON() {
	string output = "{\"type\":\"CALL_STATEMENT\",\"arguments\":[";
	for(CallElement element: this->elements) {
		if(element.component != nullptr) {
			output += element.component->printJSON();
		}
		else if(element.isComma) {
			output += ",";
		}
	}

	output += "]}";
	return output;
}

pair<
	vector<CallElement>::iterator,
	vector<CallElement>::iterator
> CallStatement::getElements() {
	return make_pair(this->elements.begin(), this->elements.end());
}

CallElement &CallStatement::getElement(size_t index) {
	return this->elements[index];
}

size_t CallStatement::getElementCount() {
	size_t count = 0;
	for(CallElement element: this->elements) {
		if(element.component != nullptr) {
			count++;
		}
	}
	return count;
}

// upon compilation, push values of the variables
ts::InstructionReturn CallStatement::compile(ts::Interpreter* interpreter, ts::CompilationContext context) {
	ts::InstructionReturn output;
	for(CallElement &element: this->elements) {
		if(!element.isComma) {
			output.add(element.component->compile(interpreter, context));
		}
	}
	return output;
}