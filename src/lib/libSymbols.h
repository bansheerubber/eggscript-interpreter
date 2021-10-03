#pragma once

#include <stdarg.h>

extern "C" {	
	typedef void* esEnginePtr;
	#define esPrintFunction(name)		int (*name)(const char* format, ...)
	#define esVPrintFunction(name)		int (*name)(const char* format, va_list args)

	enum esEntryType {
		ES_ENTRY_INVALID,
		ES_ENTRY_NUMBER,
		ES_ENTRY_STRING,
		ES_ENTRY_OBJECT,
	};
	
	typedef struct esObjectWrapper {
		void* object;
		void* data;
	} esObjectWrapper;
	typedef esObjectWrapper* esObjectWrapperPtr;

	typedef struct esObjectReference {
		esObjectWrapperPtr objectWrapper;
		unsigned long id = 0;
		void* __pad1;
		void* __pad2;
	} esObjectReference;
	typedef esObjectReference* esObjectReferencePtr;

	typedef struct esEntry {
		esEntryType type;
		union {
			double numberData;
			char* stringData;
			esObjectReferencePtr objectData;
		};
	} esEntry;
	typedef esEntry* esEntryPtr;
	
	typedef esEntryPtr (*esFunctionPtr)(esEnginePtr engine, unsigned int argc, esEntryPtr args);
	
	esEnginePtr esCreateEngine(char isParallel);

	bool esTick(esEnginePtr engine);
	void esSetTickRate(esEnginePtr engine, long tickRate);
	void esExecFile(esEnginePtr engine, const char* filename);
	void esEval(esEnginePtr engine, const char* shell);
	void esSetPrintFunction(esPrintFunction(print), esPrintFunction(warning), esPrintFunction(error));
	void esSetVPrintFunction(esVPrintFunction(print), esVPrintFunction(warning), esVPrintFunction(error));
	void esRegisterNamespace(esEnginePtr engine, const char* nameSpace);
	void esNamespaceInherit(esEnginePtr engine, const char* parent, const char* child);
	esObjectReferencePtr esCreateObject(esEnginePtr engine, const char* nameSpace, void* data);
	void esDeleteObject(esObjectReferencePtr objectReference);
	const char* esGetNamespaceFromObject(esObjectReferencePtr object);
	int esCompareNamespaceToObject(esObjectReferencePtr object, const char* nameSpace);
	void esRegisterFunction(esEnginePtr engine, esEntryType returnType, esFunctionPtr function, const char* name, unsigned int argumentCount, esEntryType* argTypes);
	void esRegisterMethod(esEnginePtr engine, esEntryType returnType, esFunctionPtr function, const char* nameSpace, const char* name, unsigned int argumentCount, esEntryType* argTypes);
	esEntryPtr esCallFunction(esEnginePtr engine, const char* functionName, unsigned int argumentCount, esEntryPtr arguments);
	esEntryPtr esCallMethod(esEnginePtr engine, esObjectReferencePtr object, const char* functionName, unsigned int argumentCount, esEntryPtr arguments);
}
