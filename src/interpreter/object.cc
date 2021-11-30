#include "object.h"

#include "interpreter.h"
#include "methodTree.h"

ObjectWrapper* ts::CreateObject(
	class ts::Interpreter* interpreter,
	bool inhibitInterpret,
	string nameSpace,
	string inheritedName,
	MethodTree* methodTree,
	MethodTree* typeMethodTree,
	void* data
) {
	Object* object = new Object(interpreter, nameSpace, inheritedName, methodTree, typeMethodTree);
	ObjectWrapper* wrapper = new ObjectWrapper(object, data);
	object->wrapper = wrapper;
	interpreter->objects[object->id] = wrapper;

	if(data == nullptr) {
		if(typeMethodTree->tsslConstructor) {
			(*typeMethodTree->tsslConstructor)(wrapper);
		}
	}
	else {
		wrapper->data = data;
	}

	interpreter->garbageHeap.insert(wrapper);

	ts::ObjectReference* reference = new ObjectReference(wrapper);
	ts::Entry* entry = new Entry(reference);
	interpreter->callMethod(reference, "onAdd", entry, 1, inhibitInterpret);

	return wrapper;
}

ObjectWrapper::~ObjectWrapper() {
	// if we weren't garbage collected, we need to remove ourselves from the garbage collection heap
	if(this->object->properties.interpreter->garbageHeap.array[0] != this) {
		this->referenceCount = -10000;
		this->object->properties.interpreter->garbageHeap.updateDown(this->heapIndex);
		this->object->properties.interpreter->garbageHeap.pop();
	}

	if(this->object->methodTree->tsslDeconstructor != nullptr) {
		(*this->object->methodTree->tsslDeconstructor)(this);
	}
	else {
		delete this->data;
	}
	delete this->object;
}

Object::Object(ts::Interpreter* interpreter, string nameSpace, string inheritedName, MethodTree* methodTree, MethodTree* typeMethodTree) {
	this->properties.interpreter = interpreter;

	if(inheritedName.length() != 0) {
		// TODO hash
		auto objectIterator = interpreter->stringToObject.find(inheritedName);
		if(objectIterator != interpreter->stringToObject.end()) {
			this->inherit(objectIterator->second->object);
		}
	}

	this->id = interpreter->highestObjectId++;

	string id = "id";
	Entry entry;
	entry.setNumber(this->id);
	this->properties.setVariableEntry(id, entry);

	this->nameSpace = nameSpace;
	this->methodTree = methodTree;
	this->typeMethodTree = typeMethodTree;
}

Object::~Object() {
	ObjectReference* reference = this->list;
	while(reference != nullptr) {
		reference->objectWrapper = nullptr;
		reference = reference->next;
	}
	
	this->properties.interpreter->deleteObjectName(this->name);
	this->properties.interpreter->objects.erase(this->id);
}

void Object::inherit(Object* parent) {
	this->properties.inherit(parent->properties);
}

void Object::addReference(ObjectReference* reference) {
	if(this->list == nullptr) {
		this->list = reference;
	}
	else {
		this->top->next = reference;
		reference->previous = this->top;
	}

	this->top = reference;

	this->wrapper->referenceCount++;
	this->properties.interpreter->garbageHeap.updateUp(this->wrapper->heapIndex);
}

void Object::removeReference(ObjectReference* reference) {
	if(this->list == reference && this->top == reference) { // clear the list if we only have one reference
		this->list = nullptr;
	}
	else if(this->list == reference) {
		this->list = reference->next;
		reference->next->previous = nullptr;
	}
	else if(this->top == reference) {
		this->top = reference->previous;
		reference->previous->next = nullptr;
	}
	else {
		reference->previous->next = reference->next;
		reference->next->previous = reference->previous;
	}

	this->wrapper->referenceCount--;
	this->properties.interpreter->garbageHeap.updateDown(this->wrapper->heapIndex);
}

void Object::setName(string &name) {
	this->name = name;
}
