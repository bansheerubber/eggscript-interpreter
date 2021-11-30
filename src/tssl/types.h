#pragma once

#include "../interpreter/entry.h"
#include "../interpreter/object.h"

namespace ts {
	class Engine;
	
	namespace sl {
		Entry* toNumber(Engine* engine, unsigned int argc, Entry* args);
		Entry* toString(Engine* engine, unsigned int argc, Entry* args);
	};
};
