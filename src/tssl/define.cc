#include "define.h"

#include <math.h>

#include "array.h"
#include "echo.h"
#include "../engine/engine.h"
#include "eval.h"
#include "exec.h"
#include "fileObject.h"
#include "getField.h"
#include "getRecord.h"
#include "getWord.h"
#include "../interpreter/interpreter.h"
#include "isObject.h"
#include "math.h"
#include "../interpreter/methodTree.h"
#include "schedule.h"
#include "scriptObject.h"
#include "simObject.h"
#include "string.h"

using namespace ts::sl;

vector<ts::sl::Function*> ts::sl::functions;
unordered_map<string, size_t> ts::sl::nameToIndex;

vector<MethodTree*> ts::sl::methodTrees;
unordered_map<string, size_t> ts::sl::methodTreeNameToIndex;

ts::sl::Function* ts::sl::FUNC_DEF(entry::EntryType returnType, ts_func functionPointer, const char* name, unsigned int argumentCount, entry::EntryType* argumentTypes) {
	ts::sl::Function* function = new ts::sl::Function;
	function->returnType = returnType;
	function->name = string(name);
	function->argumentCount = argumentCount;
	function->function = functionPointer;

	function->argumentTypes = new entry::EntryType[argumentCount];
	for(size_t i = 0; i < argumentCount; i++) {
		function->argumentTypes[i] = argumentTypes[i];
	}

	return function;
}

ts::sl::Function* ts::sl::FUNC_DEF(entry::EntryType returnType, ts_func functionPointer, const char* nameSpace, const char* name, unsigned int argumentCount, entry::EntryType* argumentTypes) {
	ts::sl::Function* function = new ts::sl::Function;
	function->returnType = returnType;
	function->nameSpace = string(nameSpace);
	function->name = string(name);
	function->argumentCount = argumentCount;
	function->function = functionPointer;
	
	function->argumentTypes = new entry::EntryType[argumentCount];
	for(size_t i = 0; i < argumentCount; i++) {
		function->argumentTypes[i] = argumentTypes[i];
	}
	
	return function;
}

MethodTree* ts::sl::NAMESPACE_DEF(const char* name) {
	string nameString(name);
	return new MethodTree(nameString, -1);
}

void ts::sl::define(Engine* engine) {
	// define namespaces and their inheristance structure
	vector<MethodTree*> methodTrees;

	MethodTree* SimObject = engine->createMethodTreeFromNamespace("SimObject");
	SimObject->isTSSL = true;
	methodTrees.push_back(SimObject);

	MethodTree* ScriptObject = engine->createMethodTreeFromNamespace("ScriptObject");
	ScriptObject->isTSSL = true;
	methodTrees.push_back(ScriptObject);
	ScriptObject->addParent(SimObject);

	MethodTree* FileObject = engine->createMethodTreeFromNamespace("FileObject");
	FileObject->isTSSL = true;
	methodTrees.push_back(FileObject);
	FileObject->addParent(SimObject);
	FileObject->tsslConstructor = &FileObject__constructor;

	MethodTree* Array = engine->createMethodTreeFromNamespace("Array");
	Array->isTSSL = true;
	methodTrees.push_back(Array);
	Array->tsslConstructor = &Array__constructor;

	for(MethodTree* tree: methodTrees) {
		engine->defineTSSLMethodTree(tree);
	}
	
	// define functions/methods
	vector<ts::sl::Function*> functions;
	entry::EntryType s[1] = { entry::STRING };
	entry::EntryType n[1] = { entry::NUMBER };
	entry::EntryType o[1] = { entry::OBJECT };
	entry::EntryType sn[2] = { entry::STRING, entry::NUMBER };
	entry::EntryType nn[2] = { entry::NUMBER, entry::NUMBER };
	entry::EntryType os[2] = { entry::OBJECT, entry::STRING };
	entry::EntryType on[2] = { entry::OBJECT, entry::NUMBER };
	entry::EntryType ns[2] = { entry::NUMBER, entry::STRING };
	entry::EntryType ss[2] = { entry::STRING, entry::STRING };
	entry::EntryType sss[3] = { entry::STRING, entry::STRING, entry::STRING };
	entry::EntryType snn[3] = { entry::STRING, entry::NUMBER, entry::NUMBER };
	entry::EntryType ssn[3] = { entry::STRING, entry::STRING, entry::NUMBER };
	entry::EntryType sns[3] = { entry::STRING, entry::NUMBER, entry::STRING };
	entry::EntryType ons[3] = { entry::OBJECT, entry::NUMBER, entry::STRING };

	functions.push_back(FUNC_DEF(entry::EMPTY, &echo, "echo", 1, s));
	functions.push_back(FUNC_DEF(entry::EMPTY, &error, "error", 1, s));

	functions.push_back(FUNC_DEF(entry::STRING, &firstWord, "firstWord", 1, s));
	functions.push_back(FUNC_DEF(entry::STRING, &restWords, "restWords", 1, s));
	functions.push_back(FUNC_DEF(entry::STRING, &getWord, "getWord", 2, sn));
	functions.push_back(FUNC_DEF(entry::STRING, &getWords, "getWords", 3, snn));
	functions.push_back(FUNC_DEF(entry::NUMBER, &getWordCount, "getWordCount", 1, s));
	functions.push_back(FUNC_DEF(entry::STRING, &removeWord, "removeWord", 2, sn));
	functions.push_back(FUNC_DEF(entry::STRING, &setWord, "setWord", 3, sns));
	functions.push_back(FUNC_DEF(entry::STRING, &firstField, "firstField", 1, s));
	functions.push_back(FUNC_DEF(entry::STRING, &restFields, "restFields", 1, s));
	functions.push_back(FUNC_DEF(entry::STRING, &getField, "getField", 2, sn));
	functions.push_back(FUNC_DEF(entry::STRING, &getFields, "getFields", 3, snn));
	functions.push_back(FUNC_DEF(entry::NUMBER, &getFieldCount, "getFieldCount", 1, s));
	functions.push_back(FUNC_DEF(entry::STRING, &removeField, "removeField", 2, sn));
	functions.push_back(FUNC_DEF(entry::STRING, &setField, "setField", 3, sns));
	functions.push_back(FUNC_DEF(entry::STRING, &firstRecord, "firstRecord", 1, s));
	functions.push_back(FUNC_DEF(entry::STRING, &restRecords, "restRecords", 1, s));
	functions.push_back(FUNC_DEF(entry::STRING, &getRecord, "getRecord", 2, sn));
	functions.push_back(FUNC_DEF(entry::STRING, &getRecords, "getRecords", 3, snn));
	functions.push_back(FUNC_DEF(entry::NUMBER, &getRecordCount, "getRecordCount", 1, s));
	functions.push_back(FUNC_DEF(entry::STRING, &removeRecord, "removeRecord", 2, sn));
	functions.push_back(FUNC_DEF(entry::STRING, &setRecord, "setRecord", 3, sns));
	functions.push_back(FUNC_DEF(entry::STRING, &strLen, "strLen", 1, s));
	functions.push_back(FUNC_DEF(entry::STRING, &getSubStr, "getSubStr", 3, snn));
	functions.push_back(FUNC_DEF(entry::STRING, &strPos, "strPos", 3, ssn));
	functions.push_back(FUNC_DEF(entry::STRING, &trim, "trim", 1, s));
	functions.push_back(FUNC_DEF(entry::STRING, &ltrim, "ltrim", 1, s));
	functions.push_back(FUNC_DEF(entry::STRING, &rtrim, "rtrim", 1, s));
	functions.push_back(FUNC_DEF(entry::NUMBER, &strCmp, "strcmp", 2, ss));
	functions.push_back(FUNC_DEF(entry::NUMBER, &strICmp, "stricmp", 2, ss));
	functions.push_back(FUNC_DEF(entry::STRING, &strLwr, "strLwr", 1, s));
	functions.push_back(FUNC_DEF(entry::STRING, &strUpr, "strUpr", 1, s));
	functions.push_back(FUNC_DEF(entry::STRING, &strChr, "strChr", 2, ss));
	functions.push_back(FUNC_DEF(entry::STRING, &strPos, "strStr", 2, ss));
	functions.push_back(FUNC_DEF(entry::STRING, &stripChars, "stripChars", 2, ss));
	functions.push_back(FUNC_DEF(entry::STRING, &expandEscape, "expandEscape", 1, s));
	functions.push_back(FUNC_DEF(entry::STRING, &_collapseEscape, "collapseEscape", 1, s));
	functions.push_back(FUNC_DEF(entry::STRING, &strReplace, "strReplace", 3, sss));

	functions.push_back(FUNC_DEF(entry::NUMBER, &mAbs, "mAbs", 1, n));
	functions.push_back(FUNC_DEF(entry::NUMBER, &mACos, "mACos", 1, n));
	functions.push_back(FUNC_DEF(entry::NUMBER, &mASin, "mASin", 1, n));
	functions.push_back(FUNC_DEF(entry::NUMBER, &mATan, "mATan", 2, nn));
	functions.push_back(FUNC_DEF(entry::NUMBER, &mCeil, "mCeil", 1, n));
	functions.push_back(FUNC_DEF(entry::NUMBER, &mFloor, "mFloor", 1, n));
	functions.push_back(FUNC_DEF(entry::NUMBER, &mCos, "mCos", 1, n));
	functions.push_back(FUNC_DEF(entry::NUMBER, &mSin, "mSin", 1, n));
	functions.push_back(FUNC_DEF(entry::NUMBER, &mTan, "mTan", 1, n));
	functions.push_back(FUNC_DEF(entry::NUMBER, &mPow, "mPow", 2, nn));
	functions.push_back(FUNC_DEF(entry::NUMBER, &mSqrt, "mSqrt", 1, n));
	functions.push_back(FUNC_DEF(entry::NUMBER, &mDegToRad, "mDegToRad", 1, n));
	functions.push_back(FUNC_DEF(entry::NUMBER, &mRadToDeg, "mRadToDeg", 1, n));
	functions.push_back(FUNC_DEF(entry::NUMBER, &mLog, "mLog", 1, n));
	functions.push_back(FUNC_DEF(entry::STRING, &mFloatLength, "mFloatLength", 2, nn));

	functions.push_back(FUNC_DEF(entry::EMPTY, &schedule, "schedule", 2, ns));
	functions.push_back(FUNC_DEF(entry::EMPTY, &SimObject__schedule, "SimObject", "schedule", 3, ons));
	functions.push_back(FUNC_DEF(entry::EMPTY, &SimObject__getId, "SimObject", "getId", 1, o));
	functions.push_back(FUNC_DEF(entry::EMPTY, &SimObject__delete, "SimObject", "delete", 1, o));

	functions.push_back(FUNC_DEF(entry::EMPTY, &SimObject__test, "SimObject", "test", 2, os));
	functions.push_back(FUNC_DEF(entry::EMPTY, &ScriptObject__test, "ScriptObject", "test", 2, os));

	functions.push_back(FUNC_DEF(entry::EMPTY, &FileObject__openForRead, "FileObject", "openForRead", 2, os));
	functions.push_back(FUNC_DEF(entry::EMPTY, &FileObject__openForWrite, "FileObject", "openForWrite", 2, os));
	functions.push_back(FUNC_DEF(entry::EMPTY, &FileObject__openForAppend, "FileObject", "openForAppend", 2, os));
	functions.push_back(FUNC_DEF(entry::EMPTY, &FileObject__close, "FileObject", "close", 1, o));
	functions.push_back(FUNC_DEF(entry::STRING, &FileObject__readLine, "FileObject", "readLine", 1, o));
	functions.push_back(FUNC_DEF(entry::EMPTY, &FileObject__writeLine, "FileObject", "writeLine", 2, os));
	functions.push_back(FUNC_DEF(entry::NUMBER, &FileObject__isEOF, "FileObject", "isEOF", 1, o));

	functions.push_back(FUNC_DEF(entry::STRING, &fileBase, "fileBase", 1, s));
	functions.push_back(FUNC_DEF(entry::STRING, &fileExt, "fileExt", 1, s));
	functions.push_back(FUNC_DEF(entry::STRING, &fileName, "fileName", 1, s));
	functions.push_back(FUNC_DEF(entry::STRING, &filePath, "filePath", 1, s));

	functions.push_back(FUNC_DEF(entry::EMPTY, &isObject, "isObject", 1, o));

	functions.push_back(FUNC_DEF(entry::EMPTY, &getRandom, "getRandom", 2, nn));
	functions.push_back(FUNC_DEF(entry::EMPTY, &setRandomSeed, "setRandomSeed", 1, n));
	functions.push_back(FUNC_DEF(entry::NUMBER, &getRandomSeed, "getRandomSeed", 0, nullptr));

	functions.push_back(FUNC_DEF(entry::EMPTY, &eval, "eval", 1, s));
	functions.push_back(FUNC_DEF(entry::EMPTY, &exec, "exec", 1, s));

	// EMPTY
	functions.push_back(FUNC_DEF(entry::EMPTY, &Array__push, "Array", "push", 1, o));
	functions.push_back(FUNC_DEF(entry::NUMBER, &Array__size, "Array", "size", 1, o));
	functions.push_back(FUNC_DEF(entry::EMPTY, &Array__insert, "Array", "insert", 2, on));
	functions.push_back(FUNC_DEF(entry::EMPTY, &Array__remove, "Array", "remove", 2, on));
	functions.push_back(FUNC_DEF(entry::EMPTY, &Array__index, "Array", "index", 1, o));

	for(ts::sl::Function* function: functions) {
		engine->defineTSSLFunction(function);
	}
}

Entry* ts::sl::PARENT(Engine* engine, const char* methodName, unsigned int argc, Entry* argv, entry::EntryType* argumentTypes) {
	string methodNameString(methodName);
	return engine->interpreter->handleTSSLParent(methodNameString, argc, argv, argumentTypes);
}
