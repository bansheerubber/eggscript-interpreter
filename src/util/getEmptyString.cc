#include "getEmptyString.h"

#include "../interpreter/string.h"

ts::String* getEmptyString() {
	return new ts::String((uint16_t)0);
}
