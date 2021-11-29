#include "types.h"

#include "../util/stringToNumber.h"

Entry* ts::sl::number(Engine* engine, unsigned int argc, Entry* args) {
	if(argc == 1) {
		switch(args[0].type) {
			case entry::EMPTY: {
				return nullptr;
			}
			
			case entry::NUMBER: {
				return new Entry(args[0]);
			}

			case entry::STRING: {
				return new Entry(stringToNumber(args[0].stringData));
			}

			case entry::OBJECT: {
				return nullptr;
			}
		}
	}
	return nullptr;
}
