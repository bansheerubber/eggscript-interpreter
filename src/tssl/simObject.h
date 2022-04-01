#pragma once

#include "../interpreter/entry.h"
#include "../interpreter/object.h"

namespace ts {
	class Engine;

	namespace sl {
		Entry* SimObject__test(Engine* engine, unsigned int argc, Entry* args);
		Entry* SimObject__onAdd(Engine* engine, unsigned int argc, Entry* args);
		Entry* SimObject__getId(Engine* engine, unsigned int argc, Entry* args);
		Entry* SimObject__delete(Engine* engine, unsigned int argc, Entry* args);
		Entry* SimObject__isMethod(Engine* engine, unsigned int argc, Entry* args);
	}
}