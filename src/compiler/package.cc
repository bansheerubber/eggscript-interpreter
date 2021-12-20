#include "package.h"

#include "../engine/engine.h"
#include "../util/toLower.h"

ts::Package::Package(ts::Engine* engine) {
	this->engine = engine;
}

void ts::Package::addPackageFunction(string name, Function* function) {
	this->nameToFunction[toLower(name)] = function;
}

void ts::Package::removeFunction(string name) {
	name = toLower(name);
	// find the function container that is currently active
	Function* function = this->nameToFunction[name];
	if(function == nullptr) {
		return;
	}

	this->engine->functions[this->engine->nameToFunctionIndex[toLower(name)]]->removePackageFunction(function);
	delete this->nameToFunction[name];
	this->nameToFunction[name] = nullptr;
}

void ts::Package::addPackageMethod(string nameSpace, string name, Function* function) {
	this->nameToMethod[pair<string, string>(toLower(nameSpace), toLower(name))] = function;
}

void ts::Package::removeMethod(string nameSpace, string name) {
	// find the function container that is currently active
	pair<string, string> key(toLower(nameSpace), toLower(name));
	Function* function = this->nameToMethod[key];
	if(function == nullptr) {
		return;
	}

	this->engine->methodTrees[this->engine->namespaceToMethodTreeIndex[toLower(nameSpace)]]->removePackageMethod(this->engine->methodNameToIndex[toLower(name)], function);
	delete this->nameToMethod[key];
	this->nameToMethod[key] = nullptr;
}
