#include "array.h"

#include "../engine/engine.h"
#include "../util/getEmptyString.h"

namespace ts {
	namespace sl {
		void arrayInitEntry(Array* owner, Entry* entry) {
			entry->type = entry::EMPTY;
		}
		
		void Array__constructor(ObjectWrapper* wrapper) {
			wrapper->object->dataStructure = ARRAY;
			wrapper->data = new Array();
		}

		void Array::push(Entry* entries, long amount) {
			for(long i = 0; i < amount; i++) {
				copyEntry(entries[i], this->array[this->array.head]);
				this->array.pushed();
			}
		}

		// TODO: if amount < 0 and |amount| > index(?) then its going to be screwy
		void Array::shift(long index, long amount, bool fill) {
			size_t end = this->array.head;

			for(int i = 0; i < amount; i++) {
				this->array.pushed(); // allocate space
			}

			// start from the end for shifting right
			if(amount >= 0) {
				for(int i = end - 1; i >= index; i--) {
					this->array[i + amount] = std::move(this->array[i]); // move
				}
			}
			else {
				for(int i = index; i < end; i++) {
					this->array[i + amount] = std::move(this->array[i]); // move
				}
			}

			for(int i = index; i < index + amount; i++) {
				// re-initialize entries
				new((void*)&this->array[i]) ts::Entry();
			}

			if(amount < 0) { // pop for shift lefts
				for(int i = end - 1; i >= end + amount; i--) {
					new((void*)&this->array[i]) ts::Entry();
					this->array.popped();
				}
			}
		}

		Entry* Array__push(Engine* engine, unsigned int argc, Entry* args) {
			if(argc < 1 || args[0].objectData->objectWrapper->object->typeMethodTree->name != "Array") {
				return nullptr;
			}

			Array* array = (Array*)args[0].objectData->objectWrapper->data;
			array->push(&args[1], argc - 1);
			
			return nullptr;
		}

		Entry* Array__size(Engine* engine, unsigned int argc, Entry* args) {
			if(argc != 1 || args[0].objectData->objectWrapper->object->typeMethodTree->name != "Array") {
				return nullptr;
			}
			
			return new Entry(((Array*)args[0].objectData->objectWrapper->data)->array.head);
		}

		Entry* Array__insert(Engine* engine, unsigned int argc, Entry* args) {
			if(argc != 3 || args[0].objectData->objectWrapper->object->typeMethodTree->name != "Array") {
				return nullptr;
			}

			Array* array = (Array*)args[0].objectData->objectWrapper->data;
			array->shift((long)args[1].numberData, 1);
			copyEntry(args[2], array->array[(long)args[1].numberData]);

			return nullptr;
		}

		Entry* Array__remove(Engine* engine, unsigned int argc, Entry* args) {
			if(argc != 2 || args[0].objectData->objectWrapper->object->typeMethodTree->name != "Array") {
				return nullptr;
			}

			Array* array = (Array*)args[0].objectData->objectWrapper->data;
			array->shift((long)args[1].numberData + 1, -1);
			return nullptr;
		}

		Entry* Array__index(Engine* engine, unsigned int argc, Entry* args) {
			if(argc != 2 || args[0].objectData->objectWrapper->object->typeMethodTree->name != "Array") {
				return new Entry(-1);
			}

			Array* array = (Array*)args[0].objectData->objectWrapper->data;
			for(size_t i = 0; i < array->array.head; i++) {
				if(isEntryEqual(args[1], array->array[i])) {
					return new Entry(i);
				}
			}

			return new Entry(-1);
		}
	}
}
