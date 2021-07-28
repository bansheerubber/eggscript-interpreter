#pragma once

extern "C" {	
	typedef struct tsInterpreter {
		const unsigned long id;
		const int isParallel;
	} tsInterpreter;
	
	tsInterpreter tsCreateInterpreter(char isParallel);

	void tsTick(tsInteprreter interpreter);
	void tsExecFile(tsInterpreter interpreter, const char* filename);
}
