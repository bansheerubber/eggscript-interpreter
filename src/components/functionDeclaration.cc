#include "functionDeclaration.h"
#include "../interpreter/interpreter.h"

#include "accessStatement.h"
#include "../util/getEmptyString.h"

bool FunctionDeclaration::ShouldParse(ts::Engine* engine) {
	return engine->tokenizer->peekToken().type == FUNCTION;
}

FunctionDeclaration* FunctionDeclaration::Parse(Body* parent, ts::Engine* engine) {
	if(parent->getType() != SOURCE_FILE && parent->getType() != PACKAGE_DECLARATION) {
		engine->parser->error("cannot declare scoped functions");
	}
	
	FunctionDeclaration* output = new FunctionDeclaration(engine);
	output->parent = parent;
	engine->parser->expectToken(FUNCTION);

	// parse a symbol
	if(!Symbol::ShouldParse(engine)) {
		engine->parser->error("invalid function name");
	}
	output->name1 = Symbol::Parse(output, engine);

	if(engine->tokenizer->peekToken().type == NAMESPACE) {
		engine->parser->expectToken(NAMESPACE);

		// parse a symbol
		if(!Symbol::ShouldParse(engine)) {
			engine->parser->error("invalid function name");
		}
		output->name2 = Symbol::Parse(output, engine);
	}

	// parse the argument list
	if(!CallStatement::ShouldParse(engine)) {
		engine->parser->error("expected arguments for function");
	}
	output->args = CallStatement::Parse(output, engine);

	// find errors in args
	auto it = output->args->getElements();
	for (; it.first != it.second; it.first++) {
		Component* component = (*(it.first)).component;
		if(
			component != nullptr
			&& (
				component->getType() != ACCESS_STATEMENT
				|| !((AccessStatement*)component)->isLocalVariable()
				|| ((AccessStatement*)component)->chainSize() != 1
			)
		) {
			engine->parser->error("unexpected argument '%s' for function declaration", component->print().c_str());
		}
	}

	engine->parser->expectToken(LEFT_BRACKET);
	
	Component::ParseBody(output, engine);

	engine->parser->expectToken(RIGHT_BRACKET);

	return output;
}

string FunctionDeclaration::print() {
	string output = "function " + this->name1->print() + this->args->print() + this->engine->parser->space + "{" + this->engine->parser->newLine;
	if(this->name2 != nullptr) {
		output = "function " + this->name1->print() + "::" + this->name2->print() + this->args->print() + this->engine->parser->space + "{" + this->engine->parser->newLine;
	}

	output += this->printBody();
	output += "}" + this->engine->parser->newLine;
	return output;
}

string FunctionDeclaration::printJSON() {
	if(this->name2 == nullptr) {
		return "{\"type\":\"FUNCTIONAL_DECLARATION\",\"name1\":" + this->name1->printJSON() + ",\"arguments\":" + this->args->printJSON() + ",\"body\":" + this->printJSONBody() + "}";
	}
	else {
		return "{\"type\":\"FUNCTIONAL_DECLARATION\",\"name1\":" + this->name1->printJSON() + ",\"name2\":" + this->name2->printJSON() + ",\"arguments\":" + this->args->printJSON() + ",\"body\":" + this->printJSONBody() + "}";
	}
}

ts::InstructionReturn FunctionDeclaration::compile(ts::Engine* engine, ts::CompilationContext context) {
	ts::InstructionReturn output;

	// loop through the arguments and assign them from values on the stack
	auto it = this->args->getElements();
	int argumentCount = this->args->getElementCount();
	for (; it.first != it.second; it.first++) {
		AccessStatement* component = (AccessStatement*)(*(it.first)).component; // we know these are local variables
		if(component != nullptr) {
			// allocate the variable in our scope
			string name = component->getVariableName();
			this->allocateVariable(
				name,
				true,
				component->getCharacterNumber(),
				component->getLineNumber()
			);
		}
	}

	// compile the body of the function
	for(Component* component: this->children) {
		output.add(component->compile(engine, (ts::CompilationContext){
			loop: nullptr,
			package: context.package,
			scope: this,
		}));
	}

	// push variables
	for(auto const& [key, value]: this->variables) {
		if(!value.isArgument) {
			ts::Instruction* push = new ts::Instruction(
				engine,
				this->getCharacterNumber(),
				this->getLineNumber()
			);
			push->type = ts::instruction::PUSH;
			push->push.entry = ts::Entry();
			output.addFirst(push);
		}
	}

	// tell the interpreter to pop values from the stack that were pushed as arguments
	ts::Instruction* instruction = new ts::Instruction(
		engine,
		this->getCharacterNumber(),
		this->getLineNumber()
	);
	instruction->type = ts::instruction::POP_ARGUMENTS;
	instruction->popArguments.argumentCount = argumentCount;
	output.addFirst(instruction);

	// add a return statement that exits out from our function
	ts::Instruction* returnInstruction = new ts::Instruction(
		engine,
		this->getCharacterNumber(),
		this->getLineNumber()
	);
	returnInstruction->type = ts::instruction::RETURN_NO_VALUE;
	output.add(returnInstruction);

	if(context.package == nullptr) {
		if(this->name2 != nullptr) {
			string nameSpace = this->name1->print();
			string name = this->name2->print();
			engine->defineMethod(nameSpace, name, output, argumentCount, this->allocatedSize());
		}
		else {
			string name = this->name1->print();
			engine->defineFunction(name, output, argumentCount, this->allocatedSize()); // tell the interpreter to add a function under our name
		}
	}
	else {
		if(this->name2 != nullptr) {
			string nameSpace = this->name1->print();
			string name = this->name2->print();
			engine->addPackageMethod(context.package, nameSpace, name, output, argumentCount, this->allocatedSize());
		}
		else {
			string name = this->name1->print();
			engine->addPackageFunction(context.package, name, output, argumentCount, this->allocatedSize()); // tell the interpreter to add a function under our name
		}
	}

	return {}; // do not output anything to the body, functions are stored elsewhere
}
