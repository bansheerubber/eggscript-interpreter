#pragma once

#include <vector>

#include "objectReference.h"
#include "variableContext.h"

using namespace std;
using namespace ts;

namespace ts {
	#define TS_OBJECT_CONSTRUCTOR(name)		void (*name)(ObjectWrapper* wrapper)
	struct ObjectWrapper* CreateObject(
		class ts::Interpreter* interpreter,
		string nameSpace,
		string inheritedName,
		class MethodTree* methodTree,
		class MethodTree* typeMethodTree,
		void* data = nullptr
	);
	
	class Object {
		friend ObjectWrapper* CreateObject(
			Interpreter* interpreter,
			string nameSpace,
			string inheritedName,
			MethodTree* methodTree,
			MethodTree* typeMethodTree,
			void* data
		);
		
		public:
			Object(class ts::Interpreter* interpreter, string nameSpace, string inheritedName, class MethodTree* methodTree, class MethodTree* typeMethodTree);
			~Object();

			VariableContext properties;
			size_t id = 0;

			void addReference(ObjectReference* reference);
			void removeReference(ObjectReference* reference);

			void setName(string &name);

			size_t referenceCount = 0;
			string nameSpace;
			class MethodTree* methodTree;
			class MethodTree* typeMethodTree;
		
		private:
			void inherit(Object* parent);
			ObjectReference* list = nullptr;
			ObjectReference* top = nullptr;
			string name;
			ObjectWrapper* wrapper;
	};

	struct ObjectWrapper {
		Object* object;
		void* data; // programmer-defined data for lib
		int referenceCount = 0;
		size_t heapIndex = 0;

		friend class Object;

		ObjectWrapper() {}
		ObjectWrapper(Object* object, void* data = nullptr) {
			this->object = object;
			this->data = data;
		}

		~ObjectWrapper();

		int operator<(const ObjectWrapper &other) {
			return this->referenceCount < other.referenceCount;
		}
	};
}
