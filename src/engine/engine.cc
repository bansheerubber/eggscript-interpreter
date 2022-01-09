#include "engine.h"

#include <mutex>

#include "../compiler/compiler.h"
#include "../tssl/define.h"
#include "../interpreter/instruction.h"
#include "shell.h"
#include "../components/sourceFile.h"

using namespace ts;

void ts::initPackagedFunctionList(Engine* engine, PackagedFunctionList** list) {
	*list = nullptr;
}

void ts::initMethodTree(Engine* engine, MethodTree** tree) {
	*tree = nullptr;
}

Engine::Engine(ParsedArguments args, bool isParallel) {
	ts::sl::define(this);
	
	this->args = args;
	this->tokenizer = new Tokenizer(this, args);
	this->parser = new Parser(this, args);
	this->interpreter = new Interpreter(this, args, isParallel);

	this->setRandomSeed(time(0));
}

Engine::~Engine() {
	delete this->tokenizer;
	delete this->parser;
	delete this->interpreter;

	for(uint64_t i = 0; i < this->functions.head; i++) {
		delete this->functions[i];
	}

	for(uint64_t i = 0; i < this->methodTrees.head; i++) {
		delete this->methodTrees[i];
	}
}

// lock for executing files, shells, etc since we share the same tokenizer/parser
std::mutex& execLock() {
	static std::mutex m;
	return m;
}

void Engine::execFile(string fileName, bool forceExecution) {
	if(!this->interpreter->isParallel || forceExecution) {
		if(!this->tokenizer->tokenizeFile(fileName)) {
			return;
		}
		
		if(!this->parser->startParse()) {
			return;
		}

		// compile
		InstructionReturn result = parser->getSourceFile()->compile(this, {
			loop: nullptr,
			scope: nullptr,
		});
		
		if(this->interpreter->startTime == 0) {
			this->interpreter->startInterpretation(result.first);
		}
		else {
			InstructionContainer* container = new InstructionContainer(result.first);
			this->interpreter->pushFunctionFrame(container);
			this->interpreter->interpret();
			delete container;
		}
	}
	else if(this->interpreter->isParallel) {
		this->fileQueue.push(fileName);
	}
}

void Engine::execFileContents(string fileName, string contents) {
	if(!this->tokenizer->tokenizePiped(contents)) {
		return;
	}

	if(!this->parser->startParse()) {
		return;
	}

	// compile
	InstructionReturn result = parser->getSourceFile()->compile(this, {
		loop: nullptr,
		scope: nullptr,
	});
	this->interpreter->pushFunctionFrame(new InstructionContainer(result.first), nullptr, -1, nullptr, -1, 0, 0, fileName);
	this->interpreter->interpret();
}

void Engine::execPiped(string piped) {
	if(!this->tokenizer->tokenizePiped(piped)) {
		return;
	}

	if(!this->parser->startParse()) {
		return;
	}

	// compile
	InstructionReturn result = parser->getSourceFile()->compile(this, {
		loop: nullptr,
		scope: nullptr,
	});

	this->interpreter->startInterpretation(result.first);	
}

void Engine::execShell(string shell, bool forceExecution) {
	if(!this->interpreter->isParallel || forceExecution) {
		if(!this->tokenizer->tokenizePiped(shell)) {
			return;
		}

		if(!this->parser->startParse()) {
			return;
		}

		// compile
		InstructionReturn result = parser->getSourceFile()->compile(this, {
			loop: nullptr,
			scope: nullptr,
		});
		this->interpreter->pushFunctionFrame(new InstructionContainer(result.first));
		this->interpreter->interpret();
	}
	else {
		this->shellQueue.push(shell);
	}
}

void Engine::defineTSSLMethodTree(MethodTree* tree) {
	string nameSpace = tree->name;
	if(this->namespaceToMethodTreeIndex.find(nameSpace) == this->namespaceToMethodTreeIndex.end()) {
		this->namespaceToMethodTreeIndex[nameSpace] = this->methodTrees.head;
		tree->index = this->methodTrees.head;
		this->methodTrees[this->methodTrees.head] = tree;
		this->methodTrees.pushed();
	}
}

void Engine::defineTSSLFunction(sl::Function* function) {
	Function* container = new Function(function);
	
	if(function->nameSpace.length() == 0) {
		PackagedFunctionList* list;
		if(this->nameToFunctionIndex.find(function->name) == this->nameToFunctionIndex.end()) {
			// add the function to the function-specific datastructure
			this->nameToFunctionIndex[function->name] = this->functions.head;
			list = new PackagedFunctionList(function->name);
			list->isTSSL = true;
			this->functions[this->functions.head] = list;
			this->functions.pushed();
		}
		else {
			list = this->functions[this->nameToFunctionIndex[function->name]];
		}

		// create the packaged function list
		list->defineInitialFunction(container);
	}
	else {
		MethodTree* tree = this->methodTrees[this->namespaceToMethodTreeIndex[function->nameSpace]];

		// associate the method name with an index
		uint64_t index = 0;
		if(this->methodNameToIndex.find(function->name) == this->methodNameToIndex.end()) {
			this->methodNameToIndex[function->name] = index = this->currentMethodNameIndex;
			this->currentMethodNameIndex++;
		}
		else {
			index = this->methodNameToIndex[function->name];
		}

		tree->defineInitialMethod(function->name, index, container);
	}
}

void Engine::defineFunction(string &name, InstructionReturn output, uint64_t argumentCount, uint64_t variableCount) {
	// create the function container which we will use to execute the function at runtime
	Function* container = new Function(output.first, argumentCount, variableCount, name);
	
	PackagedFunctionList* list;
	if(this->nameToFunctionIndex.find(name) == this->nameToFunctionIndex.end()) {
		// add the function to the function-specific datastructure
		this->nameToFunctionIndex[name] = this->functions.head;
		list = new PackagedFunctionList(name);
		this->functions[this->functions.head] = list;
		this->functions.pushed();
	}
	else {
		list = this->functions[this->nameToFunctionIndex[name]];
	}

	// create the packaged function list
	list->defineInitialFunction(container);
}

void Engine::defineMethod(string &nameSpace, string &name, InstructionReturn output, uint64_t argumentCount, uint64_t variableCount) {
	Function* container = new Function(output.first, argumentCount, variableCount, name, nameSpace);

	// define the method tree if we don't have one yet
	MethodTree* tree;
	if(this->namespaceToMethodTreeIndex.find(nameSpace) == this->namespaceToMethodTreeIndex.end()) {
		this->namespaceToMethodTreeIndex[nameSpace] = this->methodTrees.head;
		tree = new MethodTree(nameSpace, this->methodTrees.head);
		this->methodTrees[this->methodTrees.head] = tree;
		this->methodTrees.pushed();
	}
	else {
		tree = this->methodTrees[this->namespaceToMethodTreeIndex[nameSpace]];
	}

	// associate the method name with an index
	uint64_t index = 0;
	if(this->methodNameToIndex.find(name) == this->methodNameToIndex.end()) {
		this->methodNameToIndex[name] = index = this->currentMethodNameIndex;
		this->currentMethodNameIndex++;
	}
	else {
		index = this->methodNameToIndex[name];
	}

	tree->defineInitialMethod(name, index, container);
}

Package* Engine::createPackage(PackageContext* package) {
	if(this->nameToPackage[package->name] == nullptr) {
		this->nameToPackage[package->name] = new Package(this);
	}
	return this->nameToPackage[package->name];
}

void Engine::addPackageFunction(PackageContext* packageContext, string &name, InstructionReturn output, uint64_t argumentCount, uint64_t variableCount) {
	// create a package if we don't have one
	Package* package = this->createPackage(packageContext);
	package->removeFunction(name);
	
	// create the function container which we will use to execute the function at runtime
	Function* container = new Function(output.first, argumentCount, variableCount, name);
	
	PackagedFunctionList* list;
	if(this->nameToFunctionIndex.find(name) == this->nameToFunctionIndex.end()) {
		// add the function to the function-specific datastructure
		this->nameToFunctionIndex[name] = this->functions.head;
		list = new PackagedFunctionList(name);
		this->functions[this->functions.head] = list;
		this->functions.pushed();
	}
	else {
		list = this->functions[this->nameToFunctionIndex[name]];
	}

	// create the packaged function list
	list->addPackageFunction(container);
	package->addPackageFunction(name, container);
}

void Engine::addPackageMethod(
	PackageContext* packageContext,
	string &nameSpace,
	string &name,
	InstructionReturn output,
	uint64_t argumentCount,
	uint64_t variableCount
) {
	// create the function container which we will use to execute the function at runtime
	Function* container = new Function(output.first, argumentCount, variableCount, name, nameSpace);

	Package* package = this->createPackage(packageContext);
	package->removeMethod(nameSpace, name);
	
	// define the method tree if we don't have one yet
	MethodTree* tree;
	if(this->namespaceToMethodTreeIndex.find(nameSpace) == this->namespaceToMethodTreeIndex.end()) {
		this->namespaceToMethodTreeIndex[nameSpace] = this->methodTrees.head;
		tree = new MethodTree(nameSpace, this->methodTrees.head);
		this->methodTrees[this->methodTrees.head] = tree;
		this->methodTrees.pushed();
	}
	else {
		tree = this->methodTrees[this->namespaceToMethodTreeIndex[nameSpace]];
	}

	// associate the method name with an index
	uint64_t index = 0;
	if(this->methodNameToIndex.find(name) == this->methodNameToIndex.end()) {
		this->methodNameToIndex[name] = index = this->currentMethodNameIndex;
		this->currentMethodNameIndex++;
	}
	else {
		index = this->methodNameToIndex[name];
	}

	tree->addPackageMethod(name, index, container);
	package->addPackageMethod(nameSpace, name, container);
}

MethodTree* Engine::createMethodTreeFromNamespace(string nameSpace) {
	MethodTree* tree;
	auto iterator = this->namespaceToMethodTreeIndex.find(nameSpace);
	if(iterator == this->namespaceToMethodTreeIndex.end()) {
		this->namespaceToMethodTreeIndex[nameSpace] = this->methodTrees.head;
		tree = new MethodTree(nameSpace, this->methodTrees.head);
		this->methodTrees[this->methodTrees.head] = tree;
		this->methodTrees.pushed();
	}
	else {
		tree = this->methodTrees[iterator->second];
	}

	return tree;
}

MethodTree* Engine::getNamespace(string nameSpace) {
	auto iterator = this->namespaceToMethodTreeIndex.find(nameSpace);
	if(iterator == this->namespaceToMethodTreeIndex.end()) {
		return nullptr;
	}
	else {
		return this->methodTrees[iterator->second];
	}
}

MethodTree* Engine::createMethodTreeFromNamespaces(
	string namespace1,
	string namespace2,
	string namespace3,
	string namespace4,
	string namespace5
) {
	string names[] = {
		namespace1,
		namespace2,
		namespace3,
		namespace4,
		namespace5,
	};
	
	string nameSpace = MethodTree::GetComplexNamespace(
		namespace1,
		namespace2,
		namespace3,
		namespace4,
		namespace5
	);

	MethodTree* tree = nullptr;
	auto iterator = this->namespaceToMethodTreeIndex.find(nameSpace);
	if(iterator == this->namespaceToMethodTreeIndex.end()) {
		tree = this->createMethodTreeFromNamespace(nameSpace);
		for(uint64_t i = 0; i < 5; i++) {
			if(names[i].length() != 0 && names[i] != nameSpace) {
				MethodTree* tree2 = this->createMethodTreeFromNamespace(names[i]);
				tree->addParent(tree2);
			}
		}
	}
	else {
		tree = this->methodTrees[iterator->second];
	}

	return tree;
}
