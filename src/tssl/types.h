#pragma once

#include "../interpreter/entry.h"
#include "../interpreter/object.h"

namespace ts {
	class Engine;
	
	namespace sl {
		Entry* number(Engine* engine, unsigned int argc, Entry* args);
	};
};
