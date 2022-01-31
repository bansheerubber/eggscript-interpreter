#pragma once

#include "../include/robin-map/include/tsl/robin_map.h"
#include <fstream>
#include <string>

#include "../interpreter/entry.h"
#include "../interpreter/object.h"
#include "../interpreter/variableContext.h"

using namespace std;

namespace ts {
	class Engine;

	namespace sl {
		void Map__constructor(ObjectWrapper* wrapper);
		Entry* Map__onAdd(Engine* engine, unsigned int argc, Entry* args);
		
		class Map {
			public:
				tsl::robin_map<string, Entry> map;
		};
	}
}
