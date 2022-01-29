#include "package.h"

#include "../engine/engine.h"

ts::Package::Package(ts::Engine* engine) {
	this->engine = engine;
}

void ts::Package::addPackageFunction(string name, Function* function) {
	this->nameToFunction[name] = function;
}

void ts::Package::removeFunction(string name) {
	// find the function container that is currently active
	Function* function = this->nameToFunction[name];
	if(function == nullptr) {
		return;
	}

	this->engine->functions[this->engine->nameToFunctionIndex[name]]->removePackageFunction(function);
	delete this->nameToFunction[name];
	this->nameToFunction[name] = nullptr;
}

void ts::Package::addPackageMethod(string nameSpace, string name, Function* function) {
	this->nameToMethod[pair<string, string>(nameSpace, name)] = function;
}

void ts::Package::removeMethod(string nameSpace, string name) {
	// find the function container that is currently active
	pair<string, string> key(nameSpace, name);
	Function* function = this->nameToMethod[key];
	if(function == nullptr) {
		return;
	}

	this->engine->methodTrees[this->engine->namespaceToMethodTreeIndex[nameSpace]]->removePackageMethod(this->engine->methodNameToIndex[name], function);
	delete this->nameToMethod[key];
	this->nameToMethod[key] = nullptr;
}
