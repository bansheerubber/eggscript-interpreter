#include "echo.h"

#include "../engine/engine.h"

namespace ts {
	namespace sl {
		string mockStdout = string();
		bool useMockStdout = false;
		
		Entry* echo(Engine* engine, unsigned int argc, Entry* args) {
			if(argc >= 1) {
				if(useMockStdout) {
					mockStdout += string(args[0].stringData->string, args[0].stringData->size);
					mockStdout += '\n';
				}
				else {
					(*engine->printFunction)("%.*s\n", args[0].stringData->size, args[0].stringData->string);
				}
			}
			return nullptr;
		}

		Entry* error(Engine* engine, unsigned int argc, Entry* args) {
			if(argc >= 1) {
				if(useMockStdout) {
					mockStdout += string(args[0].stringData->string, args[0].stringData->size);
					mockStdout += '\n';
				}
				else {
					(*engine->errorFunction)("%.*s\n", args[0].stringData->size, args[0].stringData->string);
				}
			}
			return nullptr;
		}
	}
}
