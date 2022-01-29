#pragma once

#include <string>

#include "../util/dynamicArray.h"
#include "instruction.h"
#include "object.h"
#include "../include/robin-map/include/tsl/robin_map.h"

using namespace tsl;
using namespace std;

namespace ts {
	void initMethodTreePackagedFunctionList(struct MethodTreeEntry* tree, class PackagedFunctionList** list);
	void initMethodTree(class MethodTree* self, class MethodTree** tree);
	
	struct MethodTreeEntry {
		DynamicArray<class PackagedFunctionList*, MethodTreeEntry> list = DynamicArray<PackagedFunctionList*, MethodTreeEntry>(this, 18, initMethodTreePackagedFunctionList, nullptr);
		bool hasInitialMethod;
		string name;

		MethodTreeEntry() {
			this->hasInitialMethod = false;
		}
		MethodTreeEntry(class MethodTree* tree, string name);
		~MethodTreeEntry();
	};
	
	class MethodTree {
		public:
			MethodTree();
			MethodTree(string name, uint64_t index);
			~MethodTree();

			void defineInitialMethod(string name, uint64_t nameIndex, class Function* container);
			void addPackageMethod(string name, uint64_t nameIndex, class Function* container);
			void removePackageMethod(uint64_t nameIndex, class Function* container);

			void addParent(MethodTree* parent); // add a parent for this method tree, order matters
			bool hasParent(string nameSpace); // recursively check for parent

			// adds a child to this method tree, order doesn't matter
			void addChild(MethodTree* child);

			void updateMethodTree(string methodName, uint64_t methodNameIndex);

			void definePropertyDeclaration(ts::Engine* engine, ts::InstructionReturn properties);

			// each method gets its own index assigned to it by the interpreter. the method's index is based on its name,
			// so we can have a method with the same name that is defined in several unrelated namespaces but that method
			// still gets the same index as the rest of the methods with the same name
			robin_map<uint64_t, MethodTreeEntry*> methodIndexToEntry;
			string name;

			void print();

			uint64_t index;

			bool isTSSL = false;

			Function* propertyDeclaration = nullptr;

			TS_OBJECT_CONSTRUCTOR(tsslConstructor) = nullptr;
			TS_OBJECT_DECONSTRUCTOR(tsslDeconstructor) = nullptr;
		
		private:
			vector<class PackagedFunctionList*> buildMethodTreeEntryForParents(string methodName, uint64_t methodNameIndex, bool addInitial = true);
			DynamicArray<MethodTree*, MethodTree> parents = DynamicArray<MethodTree*, MethodTree>(this, 5, initMethodTree, nullptr);
			DynamicArray<MethodTree*, MethodTree> children = DynamicArray<MethodTree*, MethodTree>(this, 5, initMethodTree, nullptr);
	};
}
