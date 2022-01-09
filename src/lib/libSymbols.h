#pragma once

#include <stdarg.h>
#include <cstdint>

extern "C" {	
	typedef void* esEnginePtr;
	#define esPrintFunction(name)		int (*name)(const char* format, ...)
	#define esVPrintFunction(name)		int (*name)(const char* format, va_list args)

	enum esEntryType {
		ES_ENTRY_INVALID = 0,
		ES_ENTRY_EMPTY,
		ES_ENTRY_NUMBER,
		ES_ENTRY_STRING,
		ES_ENTRY_OBJECT,
		ES_ENTRY_MATRIX,
	};

	typedef void* esObjectPtr;
	
	typedef struct esObjectWrapper {
		esObjectPtr object;
		void* data;
		int referenceCount;
		int64_t heapIndex;
	} esObjectWrapper;
	typedef esObjectWrapper* esObjectWrapperPtr;

	typedef struct esObjectReference {
		esObjectWrapperPtr objectWrapper;
		uint64_t id = 0;
		void* __pad1;
		void* __pad2;
	} esObjectReference;
	typedef esObjectReference* esObjectReferencePtr;

	struct esEntry;
	typedef struct esMatrix {
		esEntry** data;
		unsigned int rows;
		unsigned int columns;
	} esMatrix;
	typedef esMatrix* esMatrixPtr;

	typedef struct esEntry {
		esEntryType type;
		union {
			double numberData;
			char* stringData;
			esObjectReferencePtr objectData;
			esMatrixPtr matrixData;
		};
	} esEntry;
	typedef esEntry* esEntryPtr;
	
	typedef esEntryPtr (*esFunctionPtr)(esEnginePtr engine, unsigned int argc, esEntryPtr args);
	
	esEnginePtr esCreateEngine(char isParallel);

	bool esTick(esEnginePtr engine);
	void esSetTickRate(esEnginePtr engine, int64_t tickRate);
	void esExecFile(esEnginePtr engine, const char* filename);
	void esExecVirtualFile(esEnginePtr, const char* fileName, const char* contents);
	void esEval(esEnginePtr engine, const char* shell);
	const char* esGetLastExecFileName(esEnginePtr engine);
	void esSetPrintFunction(esEnginePtr engine, esPrintFunction(print), esPrintFunction(warning), esPrintFunction(error));
	void esSetVPrintFunction(esEnginePtr engine, esVPrintFunction(print), esVPrintFunction(warning), esVPrintFunction(error));
	void esRegisterNamespace(esEnginePtr engine, const char* nameSpace);
	void esSetNamespaceConstructor(esEnginePtr engine, const char* nameSpace, void (*constructor)(esObjectWrapperPtr wrapper));
	void esSetNamespaceDeconstructor(esEnginePtr engine, const char* nameSpace, void (*deconstructor)(esObjectWrapperPtr wrapper));
	void esNamespaceInherit(esEnginePtr engine, const char* parent, const char* child);
	esObjectReferencePtr esInstantiateObject(esEnginePtr engine, const char* nameSpace, void* data);
	esObjectReferencePtr esCloneObjectReference(esObjectReferencePtr reference);
	void esDeleteObject(esObjectReferencePtr objectReference);
	const char* esGetNamespaceFromObject(esObjectReferencePtr object);
	int esCompareNamespaceToObject(esObjectReferencePtr object, const char* nameSpace);
	int esCompareNamespaceToObjectParents(esObjectReferencePtr object, const char* nameSpace);
	void esRegisterFunction(esEnginePtr engine, esEntryType returnType, esFunctionPtr function, const char* name, unsigned int argumentCount, esEntryType* argTypes);
	void esRegisterMethod(esEnginePtr engine, esEntryType returnType, esFunctionPtr function, const char* nameSpace, const char* name, unsigned int argumentCount, esEntryType* argTypes);
	esEntryPtr esCallFunction(esEnginePtr engine, const char* functionName, unsigned int argumentCount, esEntryPtr arguments);
	esEntryPtr esCallMethod(esEnginePtr engine, esObjectReferencePtr object, const char* functionName, unsigned int argumentCount, esEntryPtr arguments);

	void esDeleteEntry(esEntryPtr entry);
	esEntryPtr esCreateNumber(double number);
	esEntryPtr esCreateString(char* string);
	esEntryPtr esCreateVector(unsigned int size, ...);
	esEntryPtr esCreateMatrix(unsigned int rows, unsigned int columns, ...);
	esEntryPtr esCreateObject(esObjectReferencePtr reference);
	esObjectReferencePtr esCreateArray(esEnginePtr engine);
	esObjectReferencePtr esCreateMap(esEnginePtr engine);

	esEntryPtr esCreateNumberAt(esEntryPtr entry, double number);
	esEntryPtr esCreateStringAt(esEntryPtr entry, char* string);
	esEntryPtr esCreateVectorAt(esEntryPtr entry, unsigned int size, ...);
	esEntryPtr esCreateMatrixAt(esEntryPtr entry, unsigned int rows, unsigned int columns, ...);
	esEntryPtr esCreateObjectAt(esEntryPtr entry, esObjectReferencePtr reference);

	void esSetObjectProperty(esObjectReferencePtr object, const char* variable, esEntryPtr property);
	esEntryPtr esGetObjectProperty(esObjectReferencePtr object, const char* variable);

	double esGetNumberFromEntry(esEntryPtr entry);

	void esArrayPush(esObjectReferencePtr array, esEntryPtr entry); // greedy copies entry and deletes it
	void esMapInsert(esObjectReferencePtr map, const char* key, esEntryPtr entry); // greedy copies entry and deletes it
	const esEntryPtr esMapGet(esObjectReferencePtr map, const char* key);

	unsigned int esProbeGarbage(esEnginePtr engine, const char* className);
}
