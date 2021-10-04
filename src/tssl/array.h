#pragma once

#include <fstream>
#include <string>

#include "../util/dynamicArray.h"
#include "../interpreter/entry.h"
#include "../interpreter/object.h"

using namespace std;

namespace ts {
	class Engine;

	namespace sl {
		void arrayInitEntry(class Array* owner, Entry* entry);
		
		class Array {
			public:
				DynamicArray<Entry, Array> array = DynamicArray<Entry, Array>(this, 16, arrayInitEntry, nullptr);
		};

		void Array__constructor(ObjectWrapper* wrapper);
		Entry* Array__push(Engine* engine, unsigned int argc, Entry* args);
		Entry* Array__size(Engine* engine, unsigned int argc, Entry* args);
	}
}