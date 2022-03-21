#include "isInteger.h"

#include "../interpreter/string.h"

bool isInteger(ts::String* string) {
	for(uint16_t i = 0; i < string->size; i++) {
		if(!(string->string[i] >= '0' && string->string[i] <= '9')) {
			return false;
		}
	}
	return true;
}