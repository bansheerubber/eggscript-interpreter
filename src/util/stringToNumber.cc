#include "stringToNumber.h"

#include <stdio.h>
#include <stdlib.h>

#include "../interpreter/string.h"

using namespace std;

double stringToNumber(ts::String* value) {
	double output = 0.0;
	
	uint16_t i = 0;
	while(i < value->size && value->string[i] != '.') {
		if(!(value->string[i] >= '0' && value->string[i] <= '9')) {
			printf("easy out 1\n");
			return 0;
		}

		output = output * 10 + (value->string[i] - '0');
		i++;
	}

	if(value->string[i] == '.') {
		i++;
	}
	else {
		return output;
	}

	uint16_t e = 0;
	while(i < value->size) {
		if(!(value->string[i] >= '0' && value->string[i] <= '9')) {
			printf("easy out 2\n");
			return 0;
		}

		output = output * 10 + (value->string[i] - '0');
		i++;
		e++;
	}

	for(uint16_t i = 0; i < e; i++) {
		output /= 10;
	}

	return output;
}
