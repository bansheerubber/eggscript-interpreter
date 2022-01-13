#pragma once

#include <vector>

#include "objectReference.h"
#include "variableContext.h"

using namespace std;
using namespace ts;

namespace ts {
	enum DataStructure {
		NO_DATA_STRUCTURE = 0,
		ARRAY,
		MAP,
	};
	
	#define TS_OBJECT_CONSTRUCTOR(name)		void (*name)(ObjectWrapper* wrapper)
	#define TS_OBJECT_DECONSTRUCTOR(name)		void (*name)(ObjectWrapper* wrapper)

	struct ObjectWrapper* CreateObject(
		class ts::Interpreter* interpreter,
		bool inhibitInterpret,
		string nameSpace,
		class MethodTree* methodTree,
		void* data = nullptr
	);
	
	class Object {
		friend ObjectWrapper* CreateObject(
			Interpreter* interpreter,
			bool inhibitInterpret,
			string nameSpace,
			MethodTree* methodTree,
			void* data
		);
		
		public:
			Object(class ts::Interpreter* interpreter, string nameSpace, class MethodTree* methodTree);
			~Object();

			DataStructure dataStructure = NO_DATA_STRUCTURE;
			VariableContext properties;
			uint64_t id = 0;

			void addReference(ObjectReference* reference);
			void removeReference(ObjectReference* reference);

			void setName(string &name);

			uint64_t referenceCount = 0;
			string nameSpace;
			class MethodTree* methodTree;
		
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
		int64_t heapIndex = -1;

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
