#include "variableContext.h"

#include "entry.h"
#include "instruction.h"
#include "interpreter.h"
#include "../util/numberToString.h"
#include "object.h"

using namespace ts;

namespace std {
	template<>
	void swap<VariableContextEntry>(VariableContextEntry &entry1, VariableContextEntry &entry2) noexcept {
		using std::swap;
		swap(entry1.stackIndex, entry2.stackIndex);
		swap(entry1.entry, entry2.entry);
	}
}

VariableContext::VariableContext() {}

VariableContext::VariableContext(Interpreter* interpreter) {
	this->interpreter = interpreter;
}

void VariableContext::clear() {
	this->variableMap.clear();
}

void VariableContext::inherit(VariableContext &parent) {
	this->variableMap = parent.variableMap;
}

Entry& VariableContext::getVariableEntry(Instruction &instruction, const char* name, uint64_t hash) {
	auto value = this->variableMap.find(name, hash);
	if(value == this->variableMap.end()) { // initialize empty string
		this->interpreter->warning(&instruction, "trying to access unassigned variable/property '%s'\n", name);
		
		Entry &entry = this->variableMap[name];
		copyEntry(this->interpreter->emptyEntry, entry);
		return entry;
	}
	else {
		return value.value();
	}
}

void VariableContext::setVariableEntry(Instruction &instruction, const char* name, uint64_t hash, Entry &entry, bool greedy) {
	auto value = this->variableMap.find(name, hash);
	if(value == this->variableMap.end()) { // uninitialized
		Entry &variableEntry = this->variableMap[name];
		greedy ? greedyCopyEntry(entry, variableEntry) : copyEntry(entry, variableEntry);
	}
	else {
		Entry &variableEntry = value.value();
		greedy ? greedyCopyEntry(entry, variableEntry) : copyEntry(entry, variableEntry);
	}
}

void VariableContext::setVariableEntry(const char* name, Entry &entry) {
	auto value = this->variableMap.find(name);
	if(value == this->variableMap.end()) { // uninitialized
		copyEntry(entry, this->variableMap[name]);
	}
	else {
		copyEntry(entry, value.value());
	}
}

Entry& VariableContext::getVariableEntry(const char* name) {
	auto value = this->variableMap.find(name);
	if(value == this->variableMap.end()) { // initialize empty string
		this->interpreter->warning(nullptr, "trying to access unassigned variable/property '%s'\n", name);
		
		Entry &entry = this->variableMap[name];
		copyEntry(this->interpreter->emptyEntry, entry);
		return entry;
	}
	else {
		return value.value();
	}
}

void VariableContext::print() {
	printf("-------------------------------\n");
	for(auto it = this->variableMap.begin(); it != this->variableMap.end(); it++) {
		printf("\"%s\":\n", it->first.c_str());
		it->second.print(1);
		printf("-------------------------------\n");
	}
}

void VariableContext::printWithTab(int tabs) {
	string space;
	for(int i = 0; i < tabs; i++) {
		space += "   ";
	}
	
	for(auto it = this->variableMap.begin(); it != this->variableMap.end(); it++) {
		printf("%s\"%s\":\n", space.c_str(), it->first.c_str());
		it->second.print(tabs + 1);
	}
}

void initVariableContext(VariableContext* location) {
	
}
