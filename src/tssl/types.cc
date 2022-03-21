#include "types.h"

#include "../util/cloneString.h"
#include "../util/numberToString.h"
#include "../util/stringToNumber.h"

Entry* ts::sl::toNumber(Engine* engine, unsigned int argc, Entry* args) {
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

			case entry::MATRIX: {
				return nullptr;
			}

			case entry::OBJECT: {
				return nullptr;
			}
		}
	}
	return nullptr;
}

Entry* ts::sl::toString(Engine* engine, unsigned int argc, Entry* args) {
	if(argc == 1) {
		switch(args[0].type) {
			case entry::EMPTY: {
				return nullptr;
			}
			
			case entry::NUMBER: {
				ts::String* result = nullptr;
				## cd ../interpreter && python type_conversion.py args[0] result NUMBER STRING ""
				if(result == nullptr) {
					return nullptr;
				}
				return new Entry(result);
			}

			case entry::STRING: {
				return new Entry(new ts::String(args[0].stringData));
			}

			case entry::MATRIX: {
				ts::String* result = nullptr;
				## cd ../interpreter && python type_conversion.py args[0] result MATRIX STRING ""
				if(result == nullptr) {
					return nullptr;
				}
				
				return new Entry(result);
			}

			case entry::OBJECT: {
				return nullptr;
			}
		}
	}
	return nullptr;
}
