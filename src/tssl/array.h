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

		void Array__constructor(ObjectWrapper* wrapper);
		Entry* Array__push(Engine* engine, unsigned int argc, Entry* args);
		Entry* Array__size(Engine* engine, unsigned int argc, Entry* args);
		Entry* Array__insert(Engine* engine, unsigned int argc, Entry* args);
		Entry* Array__remove(Engine* engine, unsigned int argc, Entry* args);
		Entry* Array__index(Engine* engine, unsigned int argc, Entry* args);
		
		class Array {
			friend Entry* Array__insert(Engine* engine, unsigned int argc, Entry* args);
			friend Entry* Array__remove(Engine* engine, unsigned int argc, Entry* args);
			
			public:
				DynamicArray<Entry, Array> array = DynamicArray<Entry, Array>(this, 16, arrayInitEntry, nullptr);
				void push(Entry* entries, long amount);
			
			protected:
				void shift(long index, long amount, bool fill = false); // shifts the whole array over to the right by n starting from a index
		};
	}
}