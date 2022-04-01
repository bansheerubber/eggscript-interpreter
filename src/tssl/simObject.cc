#include "simObject.h"

#include "../engine/engine.h"

namespace ts {
	namespace sl {
		Entry* SimObject__test(Engine* engine, unsigned int argc, Entry* args) {
			if(argc >= 2) {
				printf("attemtping print because argc high\n");
				printf("sim object says: %s\n", args[1].stringData);
			}
			return nullptr;
		}

		Entry* SimObject__onAdd(Engine* engine, unsigned int argc, Entry* args) {
			return nullptr;
		}

		Entry* SimObject__getId(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				return new Entry(args[0].objectData->id);
			}
			return nullptr;
		}

		Entry* SimObject__delete(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1 && args[0].objectData->objectWrapper != nullptr) {
				delete args[0].objectData->objectWrapper;
			}
			return nullptr;
		}

		Entry* SimObject__isMethod(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 2) {
				ObjectWrapper* objectWrapper = args[0].objectData->objectWrapper;
				Object* object = nullptr;
				if(objectWrapper == nullptr) {
					return nullptr;
				}
				object = args[0].objectData->objectWrapper->object;

				auto methodNameIndex = engine->methodNameToIndex.find(string(args[1].stringData));
				if(methodNameIndex != engine->methodNameToIndex.end()) {
					auto methodEntry = object->methodTree->methodIndexToEntry.find(methodNameIndex->second);
					if(methodEntry != object->methodTree->methodIndexToEntry.end()) {
						return new Entry(1.0);
					}
				}
				return new Entry(0.0);
			}
			return nullptr;
		}
	}
}
