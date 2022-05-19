#pragma once

#include "../interpreter/entry.h"

using namespace std;

namespace ts {
	class Engine;
	
	namespace sl {
		Entry* printCompilationErrors(Engine* engine, unsigned int argc, Entry* args);
		Entry* printStack(Engine* engine, unsigned int argc, Entry* args);
	}
}
