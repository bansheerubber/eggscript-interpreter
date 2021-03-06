#pragma once

#include <cstddef>

#include "../interpreter/entry.h"

using namespace std;

namespace ts {
	class Engine;
	
	namespace sl {
		Entry* isObject(Engine* engine, unsigned int argc, Entry* args);
	}
}
