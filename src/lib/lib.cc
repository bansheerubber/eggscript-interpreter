#include "libSymbols.h"

#include <stdio.h>
#include <string>

#include "../tssl/array.h"
#include "../args.h"
#include "../compiler/compiler.h"
#include "../tssl/define.h"
#include "../engine/engine.h"
#include "../interpreter/interpreter.h"
#include "../tssl/map.h"
#include "../parser/parser.h"
#include "../tokenizer/tokenizer.h"

using namespace std;

esEnginePtr esCreateEngine(char isParallel) {
	ParsedArguments args = {};
	return new Engine(args, isParallel);
}

bool esTick(esEnginePtr engine) {
	// only tick if we're not in parallel mode
	if(!((ts::Engine*)engine)->interpreter->isParallel) {
		return ((ts::Engine*)engine)->interpreter->tick();
	}
	return false;
}

void esSetTickRate(esEnginePtr engine, int64_t tickRate) {
	((ts::Engine*)engine)->interpreter->setTickRate(tickRate);
}

void esExecFile(esEnginePtr engine, const char* filename) {
	((ts::Engine*)engine)->execFile(string(filename));
}

void esExecVirtualFile(esEnginePtr engine, const char* fileName, const char* contents) {
	((ts::Engine*)engine)->execVirtualFile(string(fileName), string(contents));
}

void esEval(esEnginePtr engine, const char* shell) {
	((ts::Engine*)engine)->execShell(string(shell));
}

const char* esGetLastExecFileName(esEnginePtr engine) {
	return ((ts::Engine*)engine)->interpreter->getTopFileNameFromFrame().c_str();
}

void esSetPrintFunction(esEnginePtr engine, esPrintFunction(print), esPrintFunction(warning), esPrintFunction(error)) {
	((ts::Engine*)engine)->printFunction = print;
	((ts::Engine*)engine)->warningFunction = warning;
	((ts::Engine*)engine)->errorFunction = error;
}

void esSetVPrintFunction(esEnginePtr engine, esVPrintFunction(print), esVPrintFunction(warning), esVPrintFunction(error)) {
	((ts::Engine*)engine)->vPrintFunction = print;
	((ts::Engine*)engine)->vWarningFunction = warning;
	((ts::Engine*)engine)->vErrorFunction = error;
}

void esSetInstructionDebug(esEnginePtr engine, bool enabled) {
	((ts::Engine*)engine)->setInstructionDebugEnabled(enabled);
}

void esRegisterNamespace(esEnginePtr engine, const char* nameSpace) {
	ts::MethodTree* methodTree = ((ts::Engine*)engine)->createMethodTreeFromNamespace(nameSpace);
	methodTree->isTSSL = true;
	((ts::Engine*)engine)->defineTSSLMethodTree(methodTree);
}

void esSetNamespaceConstructor(esEnginePtr engine, const char* nameSpace, void (*constructor)(esObjectWrapperPtr wrapper)) {
	ts::MethodTree* methodTree = ((ts::Engine*)engine)->getNamespace(nameSpace);
	methodTree->tsslConstructor = (void (*)(ObjectWrapper* wrapper))constructor;
}

void esSetNamespaceDeconstructor(esEnginePtr engine, const char* nameSpace, void (*deconstructor)(esObjectWrapperPtr wrapper)) {
	ts::MethodTree* methodTree = ((ts::Engine*)engine)->getNamespace(nameSpace);
	methodTree->tsslDeconstructor = (void (*)(ObjectWrapper* wrapper))deconstructor;
}

void esNamespaceInherit(esEnginePtr engine, const char* parent, const char* child) {
	ts::MethodTree* methodTree = ((ts::Engine*)engine)->getNamespace(child);
	methodTree->addParent(((ts::Engine*)engine)->getNamespace(parent));
}

esObjectReferencePtr esInstantiateObject(esEnginePtr engine, const char* nameSpace, void* data) {
	ts::MethodTree* methodTree = ((ts::Engine*)engine)->getNamespace(nameSpace);
	if(methodTree == nullptr) {
		return nullptr;
	}
	else {
		return (esObjectReferencePtr)new ts::ObjectReference(
			CreateObject(((ts::Engine*)engine)->interpreter, false, nameSpace, "", methodTree, methodTree, data)
		);
	}
}

esObjectReferencePtr esCloneObjectReference(esObjectReferencePtr reference) {
	return (esObjectReferencePtr)new ts::ObjectReference((ts::ObjectReference*)reference);
}

void esDeleteObject(esObjectReferencePtr objectReference) {
	// only delete the object if its not already deleted
	if(((ObjectReference*)objectReference)->objectWrapper != nullptr) {
		delete ((ObjectReference*)objectReference)->objectWrapper->object;
	}
}

const char* esGetNamespaceFromObject(esObjectReferencePtr object) {
	return ((ts::ObjectWrapper*)object->objectWrapper)->object->typeMethodTree->name.c_str();
}

int esCompareNamespaceToObject(esObjectReferencePtr object, const char* nameSpace) {
	if(object->objectWrapper == nullptr) {
		return 0;
	}
	
	return ((ts::ObjectWrapper*)object->objectWrapper)->object->typeMethodTree->name == string(nameSpace);
}

int esCompareNamespaceToObjectParents(esObjectReferencePtr object, const char* nameSpace) {
	if(object->objectWrapper == nullptr) {
		return 0;
	}

	ts::MethodTree* tree = ((ts::ObjectWrapper*)object->objectWrapper)->object->typeMethodTree;
	if(tree->name == string(nameSpace)) {
		return 1;
	}

	return tree->hasParent(nameSpace);
}

void esRegisterFunction(esEnginePtr engine, esEntryType returnType, esFunctionPtr function, const char* name, unsigned int argumentCount, esEntryType* argTypes) {
	((ts::Engine*)engine)->defineTSSLFunction(
		ts::sl::FUNC_DEF((ts::entry::EntryType)returnType, (ts_func)function, name, argumentCount, (ts::entry::EntryType*)argTypes)
	);
}

void esRegisterMethod(esEnginePtr engine, esEntryType returnType, esFunctionPtr function, const char* nameSpace, const char* name, unsigned int argumentCount, esEntryType* argTypes) {
	((ts::Engine*)engine)->defineTSSLFunction(
		ts::sl::FUNC_DEF((ts::entry::EntryType)returnType, (ts_func)function, nameSpace, name, argumentCount, (ts::entry::EntryType*)argTypes)
	);
}

esEntryPtr esCallFunction(esEnginePtr engine, const char* functionName, unsigned int argumentCount, esEntryPtr arguments) {
	return (esEntryPtr)((ts::Engine*)engine)->interpreter->callFunction(string(functionName), (ts::Entry*)arguments, argumentCount);
}

esEntryPtr esCallMethod(esEnginePtr engine, esObjectReferencePtr object, const char* functionName, unsigned int argumentCount, esEntryPtr arguments) {
	return (esEntryPtr)((ts::Engine*)engine)->interpreter->callMethod((ts::ObjectReference*)object, string(functionName), (ts::Entry*)arguments, argumentCount);
}

void esDeleteEntry(esEntryPtr entry) {
	delete ((ts::Entry*)entry);
}

esEntryPtr esCreateNumber(double number) {
	return (esEntryPtr)(new ts::Entry(number));
}

esEntryPtr esCreateString(char* string) {
	return (esEntryPtr)(new ts::Entry(string));
}

esEntryPtr esCreateVector(unsigned int size, ...) {
	va_list vl;
  va_start(vl, size);

	ts::Matrix* matrix = new ts::Matrix(size, 1);
	for(unsigned int i = 0; i < size; i++) {
		matrix->data[i][0].setNumber(va_arg(vl, double));
	}
	va_end(vl);
	return (esEntryPtr)(new ts::Entry(matrix));
}

esEntryPtr esCreateMatrix(unsigned int rows, unsigned int columns, ...) {
	va_list vl;
  va_start(vl, columns);

	ts::Matrix* matrix = new ts::Matrix(rows, columns);
	for(unsigned int r = 0; r < rows; r++) {
		for(unsigned int c = 0; c < columns; c++) {
			matrix->data[r][c].setNumber(va_arg(vl, double));
		}
	}
	va_end(vl);
	return (esEntryPtr)(new ts::Entry(matrix));
}

esEntryPtr esCreateObject(esObjectReferencePtr reference) {
	return (esEntryPtr)(new ts::Entry(new ts::ObjectReference((ts::ObjectReference*)reference)));
}

esObjectReferencePtr esCreateArray(esEnginePtr engine) {
	return esInstantiateObject(engine, "Array", nullptr);
}

esObjectReferencePtr esCreateMap(esEnginePtr engine) {
	return esInstantiateObject(engine, "Map", nullptr);
}

esEntryPtr esCreateNumberAt(esEntryPtr entry, double number) {
	return (esEntryPtr)new((void*)entry) Entry(number);
}

esEntryPtr esCreateStringAt(esEntryPtr entry, char* string) {
	return (esEntryPtr)new((void*)entry) Entry(string);
}

esEntryPtr esCreateVectorAt(esEntryPtr entry, unsigned int size, ...) {
	va_list vl;
  va_start(vl, size);

	ts::Matrix* matrix = new ts::Matrix(size, 1);
	for(unsigned int i = 0; i < size; i++) {
		matrix->data[i][0].setNumber(va_arg(vl, double));
	}
	va_end(vl);
	return (esEntryPtr)new((void*)entry) Entry(matrix);
}

esEntryPtr esCreateMatrixAt(esEntryPtr entry, unsigned int rows, unsigned int columns, ...) {
	va_list vl;
  va_start(vl, columns);

	ts::Matrix* matrix = new ts::Matrix(rows, columns);
	for(unsigned int r = 0; r < rows; r++) {
		for(unsigned int c = 0; c < columns; c++) {
			matrix->data[r][c].setNumber(va_arg(vl, double));
		}
	}
	va_end(vl);
	return (esEntryPtr)new((void*)entry) Entry(matrix);
}

esEntryPtr esCreateObjectAt(esEntryPtr entry, esObjectReferencePtr reference) {
	return (esEntryPtr)new((void*)entry) Entry(new ts::ObjectReference((ts::ObjectReference*)reference));
}

void esSetObjectProperty(esObjectReferencePtr object, const char* variable, esEntryPtr property) {
	ts::ObjectWrapper* wrapper = ((ts::ObjectReference*)object)->objectWrapper;
	if(wrapper == nullptr) {
		return;
	}

	string name(variable);
	wrapper->object->properties.setVariableEntry(name, *((ts::Entry*)property));
	esDeleteEntry(property); // TODO better way to do this??
}

esEntryPtr esGetObjectProperty(esObjectReferencePtr object, const char* variable) {
	ts::ObjectWrapper* wrapper = ((ts::ObjectReference*)object)->objectWrapper;
	if(wrapper == nullptr) {
		return nullptr;
	}

	string name(variable);
	return (esEntryPtr)(&wrapper->object->properties.getVariableEntry(name));
}

double esGetNumberFromEntry(esEntryPtr entry) {
	if(entry->type == ES_ENTRY_NUMBER) {
		return entry->numberData;
	}
	else {
		return 0.0;
	}
}

void esArrayPush(esObjectReferencePtr array, esEntryPtr entry) {
	if(esCompareNamespaceToObject(array, "Array")) {
		((ts::sl::Array*)((ts::ObjectReference*)array)->objectWrapper->data)->push((ts::Entry*)entry, 1);
	}
}

void esMapInsert(esObjectReferencePtr map, const char* key, esEntryPtr entry) {
	if(esCompareNamespaceToObject(map, "Map")) {
		copyEntry(*(ts::Entry*)entry, ((ts::sl::Map*)((ts::ObjectReference*)map)->objectWrapper->data)->map[string(key)]);
	}
}

const esEntryPtr esMapGet(esObjectReferencePtr map, const char* key) {
	if(esCompareNamespaceToObject(map, "Map")) {
		auto hashmap = ((ts::sl::Map*)((ts::ObjectReference*)map)->objectWrapper->data)->map;
		if(hashmap.find(string(key)) == hashmap.end()) {
			return nullptr;
		}
		return (esEntryPtr)(&hashmap[key]);
	}
	return nullptr;
}

unsigned int esProbeGarbage(esEnginePtr engine, const char* className) {
	return ((ts::Engine*)engine)->interpreter->probeGarbage(string(className));
}
