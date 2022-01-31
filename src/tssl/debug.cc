#include "debug.h"

#include "../engine/engine.h"

namespace ts {
	namespace sl {
		Entry* printCompilationErrors(Engine* engine, unsigned int argc, Entry* args) {
			engine->printUnlinkedInstructions();
			return nullptr;
		}
	}
}
