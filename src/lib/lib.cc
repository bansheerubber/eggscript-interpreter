#include "libSymbols.h"

#include <stdio.h>
#include <string>

#include "../args.h"
#include "../compiler/compiler.h"
#include "../tssl/define.h"
#include "../engine/engine.h"
#include "../interpreter/interpreter.h"
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

void esSetTickRate(esEnginePtr engine, long tickRate) {
	((ts::Engine*)engine)->interpreter->setTickRate(tickRate);
}

void esExecFile(esEnginePtr engine, const char* filename) {
	((ts::Engine*)engine)->execFile(string(filename));
}

void esExecFileFromContents(esEnginePtr engine, const char* fileName, const char* contents) {
	((ts::Engine*)engine)->execFileContents(string(fileName), string(contents));
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

void esVSetPrintFunction(esEnginePtr engine, esVPrintFunction(print), esVPrintFunction(warning), esVPrintFunction(error)) {
	((ts::Engine*)engine)->vPrintFunction = print;
	((ts::Engine*)engine)->vWarningFunction = warning;
	((ts::Engine*)engine)->vErrorFunction = error;
}

void esRegisterNamespace(esEnginePtr engine, const char* nameSpace) {
	ts::MethodTree* methodTree = ((ts::Engine*)engine)->createMethodTreeFromNamespace(nameSpace);
	methodTree->isTSSL = true;
	((ts::Engine*)engine)->defineTSSLMethodTree(methodTree);
}

void esNamespaceInherit(esEnginePtr engine, const char* parent, const char* child) {
	ts::MethodTree* methodTree = ((ts::Engine*)engine)->getNamespace(child);
	methodTree->addParent(((ts::Engine*)engine)->getNamespace(parent));
}

esObjectReferencePtr esInstantiateObject(esEnginePtr engine, const char* nameSpace, void* data) {
	ts::MethodTree* methodTree = ((ts::Engine*)engine)->getNamespace(nameSpace);
	return (esObjectReferencePtr)new ts::ObjectReference(
		CreateObject(((ts::Engine*)engine)->interpreter, nameSpace, "", methodTree, methodTree, data)
	);
}

esObjectReferencePtr esCloneObjectReference(esObjectReferencePtr reference) {
	return (esObjectReferencePtr)new ts::ObjectReference((ts::ObjectReference*)reference);
}

void esDeleteObject(esObjectReferencePtr objectReference) {
	// only delete the object if its already deleted
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
	
	return string(((ts::ObjectWrapper*)object->objectWrapper)->object->typeMethodTree->name) == string(nameSpace);
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

esEntryPtr esCreateNumber(double number) {
	return (esEntryPtr)(new ts::Entry(number));
}

esEntryPtr esCreateString(char* string) {
	return (esEntryPtr)(new ts::Entry(string));
}

esEntryPtr esCreateVector(unsigned int size, ...) {
	va_list vl;
  va_start(vl, size);

	ts::Matrix* matrix = new ts::Matrix();
	ts::initializeMatrix(matrix, 1, size);
	for(unsigned int i = 0; i < size; i++) {
		matrix->data[0][i].setNumber(va_arg(vl, double));
	}
	va_end(vl);
	return (esEntryPtr)(new ts::Entry(matrix));
}

esEntryPtr esCreateMatrix(unsigned int rows, unsigned int columns, ...) {
	va_list vl;
  va_start(vl, columns);

	ts::Matrix* matrix = new ts::Matrix();
	ts::initializeMatrix(matrix, rows, columns);
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

esEntryPtr esCreateNumberAt(esEntryPtr entry, double number) {
	return (esEntryPtr)new((void*)entry) Entry(number);
}

esEntryPtr esCreateStringAt(esEntryPtr entry, char* string) {
	return (esEntryPtr)new((void*)entry) Entry(string);
}

esEntryPtr esCreateVectorAt(esEntryPtr entry, unsigned int size, ...) {
	va_list vl;
  va_start(vl, size);

	ts::Matrix* matrix = new ts::Matrix();
	ts::initializeMatrix(matrix, 1, size);
	for(unsigned int i = 0; i < size; i++) {
		matrix->data[0][i].setNumber(va_arg(vl, double));
	}
	va_end(vl);
	return (esEntryPtr)new((void*)entry) Entry(matrix);
}

esEntryPtr esCreateMatrixAt(esEntryPtr entry, unsigned int rows, unsigned int columns, ...) {
	va_list vl;
  va_start(vl, columns);

	ts::Matrix* matrix = new ts::Matrix();
	ts::initializeMatrix(matrix, rows, columns);
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
