#include "eval.h"

#include "../engine/engine.h"

namespace ts {
	namespace sl {
		Entry* eval(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				string shell(args[0].stringData->string, args[0].stringData->size);
				engine->execShell(shell, true);
			}
			
			return nullptr;
		}
	}
}
