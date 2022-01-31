#include "map.h"

namespace ts {
	namespace sl {
		void Map__constructor(ObjectWrapper* wrapper) {
			wrapper->object->dataStructure = MAP;
			wrapper->data = new Map();
		}

		Entry* Map__onAdd(Engine* engine, unsigned int argc, Entry* args) {
			return nullptr;
		}
	}
}
