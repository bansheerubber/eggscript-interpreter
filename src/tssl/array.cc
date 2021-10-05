#include "array.h"

#include "../engine/engine.h"

namespace ts {
	namespace sl {
		void arrayInitEntry(Array* owner, Entry* entry) {
			entry->type = entry::INVALID;
		}
		
		void Array__constructor(ObjectWrapper* wrapper) {
			wrapper->object->dataStructure = ARRAY;
			wrapper->data = new Array();
		}

		Entry* Array__push(Engine* engine, unsigned int argc, Entry* args) {
			if(argc < 1 || args[0].objectData->objectWrapper->object->typeMethodTree->name != "Array") {
				return nullptr;
			}

			Array* array = (Array*)args[0].objectData->objectWrapper->data;
			
			for(unsigned int i = 1; i < argc; i++) {
				copyEntry(args[i], array->array[array->array.head]);
				array->array.pushed();
			}
			
			return nullptr;
		}

		Entry* Array__size(Engine* engine, unsigned int argc, Entry* args) {
			if(argc != 1 || args[0].objectData->objectWrapper->object->typeMethodTree->name != "Array") {
				return nullptr;
			}
			
			return new Entry(((Array*)args[0].objectData->objectWrapper->data)->array.head);
		}
	}
}
