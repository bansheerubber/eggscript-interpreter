#include "methodTree.h"

#include "function.h"
#include "packagedFunctionList.h"

using namespace ts;

void ts::initMethodTreePackagedFunctionList(MethodTreeEntry* tree, PackagedFunctionList** list) {
	*list = nullptr;
}

void ts::initMethodTree(MethodTree* self, MethodTree** tree) {
	*tree = nullptr;
}

MethodTree* MethodTreeInheritanceIterator::operator*() {
	if(this->index >= this->list.size()) {
		return nullptr;
	}
	return this->list[this->index];
}

MethodTreeInheritanceIterator& MethodTreeInheritanceIterator::operator++() {
	this->index++;
	return *this;
}

MethodTreeEntry::MethodTreeEntry(MethodTree* tree, string name) {
	this->name = name;
	this->list[0] = new PackagedFunctionList(name, tree->name);
	this->list[0]->owner = tree;
	this->list.pushed();
	this->hasInitialMethod = false;
}

MethodTreeEntry::~MethodTreeEntry() {
	delete this->list[0]; // only delete our initial method
}

MethodTree::MethodTree() {

}

MethodTree::MethodTree(string name, uint64_t index) {
	this->name = name;
	this->index = index;
}

MethodTree::~MethodTree() {
	for(const auto& it: this->methodIndexToEntry) {
		delete it.second;
	}
}

void MethodTree::defineInitialMethod(string name, uint64_t nameIndex, Function* container) {
	MethodTreeEntry* entry;
	if(this->methodIndexToEntry.find(nameIndex) == this->methodIndexToEntry.end()) {
		entry = this->methodIndexToEntry[nameIndex] = new MethodTreeEntry(this, name);
	}
	else {
		entry = this->methodIndexToEntry[nameIndex];
	}

	PackagedFunctionList* list = entry->list[0];

	bool hadInitialMethod = entry->hasInitialMethod;
	entry->hasInitialMethod = true;
	list->defineInitialFunction(container);

	if(!hadInitialMethod) {
		this->updateMethodTree(name, nameIndex);

		for(uint64_t i = 0; i < this->childrenList.head; i++) {
			this->childrenList[i]->updateMethodTree(name, nameIndex);
		}
	}
}

void MethodTree::addPackageMethod(string name, uint64_t nameIndex, Function* container) {
	MethodTreeEntry* entry;
	if(this->methodIndexToEntry.find(nameIndex) == this->methodIndexToEntry.end()) {
		entry = this->methodIndexToEntry[nameIndex] = new MethodTreeEntry(this, name);
	}
	else {
		entry = this->methodIndexToEntry[nameIndex];
	}

	entry->list[0]->addPackageFunction(container);
}

void MethodTree::removePackageMethod(uint64_t nameIndex, class Function* container) {
	MethodTreeEntry* entry = this->methodIndexToEntry[nameIndex];
	entry->list[0]->removePackageFunction(container);
}

void MethodTree::updateMethodTree(string methodName, uint64_t methodNameIndex) {
	if(this->methodIndexToEntry.find(methodNameIndex) == this->methodIndexToEntry.end()) {
		this->methodIndexToEntry[methodNameIndex] = new MethodTreeEntry(this, methodName);
	}
	
	vector<PackagedFunctionList*> list = this->buildMethodTreeEntryForParents(methodName, methodNameIndex, false);
	MethodTreeEntry* entry = this->methodIndexToEntry[methodNameIndex];
	entry->list.head = 1;
	for(PackagedFunctionList* functionList: list) {
		entry->list[entry->list.head] = functionList;
		entry->list.pushed();
	}

	// recursively update the method tree
	for(uint64_t i = 0; i < this->childrenList.head; i++) {
		this->childrenList[i]->updateMethodTree(methodName, methodNameIndex);
	}
}

void MethodTree::definePropertyDeclaration(ts::Engine* engine, ts::InstructionReturn properties) {
	this->propertyDeclaration = new Function(engine, properties.first, 0, 0, "");
}

MethodTreeInheritanceIterator MethodTree::parents() {
	MethodTreeInheritanceIterator iterator;
	MethodTree* current = this;
	while(current != nullptr && current->parentsList.head > 0) {
		iterator.list.push_back(current->parentsList[0]);
		current = current->parentsList[0];
	}
	return iterator;
}

MethodTreeInheritanceIterator MethodTree::parentsReverse() {
	MethodTreeInheritanceIterator iterator;
	MethodTree* current = this;
	while(current != nullptr && current->parentsList.head > 0) {
		iterator.list.push_back(current->parentsList[0]);
		current = current->parentsList[0];
	}
	reverse(iterator.list.begin(), iterator.list.end());
	return iterator;
}

vector<PackagedFunctionList*> MethodTree::buildMethodTreeEntryForParents(string methodName, uint64_t methodNameIndex, bool addInitial) {
	MethodTreeEntry* entry;
	if(this->methodIndexToEntry.find(methodNameIndex) == this->methodIndexToEntry.end()) {
		return vector<PackagedFunctionList*>();
	}
	else {
		entry = this->methodIndexToEntry[methodNameIndex];
	}
	
	vector<PackagedFunctionList*> list;
	if(addInitial && entry->hasInitialMethod) {
		list.push_back(entry->list[0]);
	}

	for(uint64_t i = 0; i < this->parentsList.head; i++) {
		vector<PackagedFunctionList*> inheritedList = this->parentsList[i]->buildMethodTreeEntryForParents(methodName, methodNameIndex);
		list.insert(list.end(), inheritedList.begin(), inheritedList.end());
	}
	return list;
}

void MethodTree::addParent(MethodTree* parent) {
	this->parentsList[this->parentsList.head] = parent;
	this->parentsList.pushed();

	parent->addChild(this);
}

bool MethodTree::hasParent(string nameSpace) {
	for(unsigned int i = 0; i < this->parentsList.head; i++) {
		if(this->parentsList[i]->name == nameSpace) {
			return true;
		}
		else if(this->parentsList[i]->hasParent(nameSpace)) {
			return true;
		}
	}
	return false;
}

void MethodTree::addChild(MethodTree* child) {
	this->childrenList[this->childrenList.head] = child;
	this->childrenList.pushed();

	for(auto &[index, entry]: this->methodIndexToEntry) {
		child->updateMethodTree(entry->name, index);
	}
}

void MethodTree::print() {
	printf("%s methods:\n", this->name.c_str());
	for(auto &[_, entry]: this->methodIndexToEntry) {
		printf("   %s:\n", entry->name.c_str());
		
		if(!entry->hasInitialMethod) {
			printf("      0: no initial method\n");
		}

		for(uint64_t i = entry->hasInitialMethod ? 0 : 1; i < entry->list.head; i++) {
			PackagedFunctionList* list = entry->list[i];
			printf("      %ld: %s::%s\n", i, list->functionNamespace.c_str(), list->functionName.c_str());
		}
	}
	printf("\n");
}
