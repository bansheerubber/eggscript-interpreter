#include "stringToChars.h"

char* stringToChars(string &input) {
	uint64_t length = input.length();
	char* output = new char[length + 1];
	for(uint64_t i = 0; i < length; i++) {
		output[i] = input[i];
	}
	output[length] = '\0';
	return output;
}