#pragma once

#include <unordered_map>
#include <string>
#include <vector>

#include "../interpreter/entry.h"
#include "../util/toLower.h"

using namespace std;

namespace ts {
	class MethodTree;
	class Interpreter;
	class Engine;

	typedef Entry* (*ts_func)(Engine* engine, unsigned int argc, Entry* args);
	
	namespace sl {
		struct Function {
			entry::EntryType returnType;
			string nameSpace;
			string name;
			uint64_t argumentCount;
			ts_func function;
			entry::EntryType* argumentTypes;

			~Function() { 
				delete[] this->argumentTypes;
			}
		};

		extern vector<Function*> functions;
		extern unordered_map<string, uint64_t> nameToIndex;

		extern vector<MethodTree*> methodTrees;
		extern unordered_map<string, uint64_t> methodTreeNameToIndex;

		ts::sl::Function* FUNC_DEF(entry::EntryType returnType, ts_func functionPointer, const char* nameSpace, const char* name, unsigned int argumentCount, entry::EntryType* argumentTypes);
		ts::sl::Function* FUNC_DEF(entry::EntryType returnType, ts_func functionPointer, const char* name, unsigned int argumentCount, entry::EntryType* argumentTypes);

		MethodTree* NAMESPACE_DEF(const char* name);

		Entry* PARENT(Engine* engine, const char* methodName, unsigned int argc, Entry* argv, entry::EntryType* argumentTypes);
		
		void define(Engine* engine);
	}
}