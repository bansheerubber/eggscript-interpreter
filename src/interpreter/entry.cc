#include "entry.h"

#include "../util/cloneString.h"
#include "../util/getEmptyString.h"
#include "interpreter.h"
#include "../util/isInteger.h"
#include "../util/numberToString.h"
#include "object.h"
#include "../util/stringToNumber.h"

using namespace ts;

Entry::Entry() {
	this->type = entry::INVALID;
	this->stringData = nullptr;
}

Entry::Entry(const Entry &source) {
	this->type = entry::INVALID;
	copyEntry(source, *this);
}

Entry::Entry(double value) {
	this->type = entry::NUMBER;
	this->numberData = value;
}

Entry::Entry(char* value) {
	this->type = entry::STRING;
	this->stringData = value;
}

Entry::Entry(Matrix* value) {
	this->type = entry::MATRIX;
	this->matrixData = value;
}

Entry::Entry(ObjectReference* value) {
	this->type = entry::OBJECT;
	this->objectData = value;
}

Entry::~Entry() {
	if(this->type == entry::STRING && this->stringData != nullptr) {
		delete[] this->stringData;
		this->stringData = nullptr;
	}

	if(this->type == entry::OBJECT && this->objectData != nullptr) {
		delete this->objectData;
		this->objectData = nullptr;
	}
}

namespace std {
	template<>
	void swap<Entry>(Entry &entry1, Entry &entry2) noexcept {
		using std::swap;
		swap(entry1.type, entry2.type);
		swap(entry1.stringData, entry2.stringData);
	}
}

void Entry::setNumber(double value) {
	this->erase();
	this->type = entry::NUMBER;
	this->numberData = value;
}

void Entry::setString(char* value) {
	this->erase();
	this->type = entry::STRING;
	this->stringData = value;
}

void Entry::setString(string value) {
	this->erase();
	this->type = entry::STRING;
	this->stringData = stringToChars(value);
}

void Entry::setMatrix(Matrix* matrix) {
	this->erase();
	this->type = entry::MATRIX;
	this->matrixData = matrix;
}

void Entry::setObject(ObjectReference* object) {
	this->erase();
	this->type = entry::OBJECT;
	this->objectData = object;
}

## entry_debug.py

const char* Entry::typeToString() const {
	return EntryTypeDebug[this->type];
}

void Entry::print(int tabs) const {
	string space;
	for(int i = 0; i < tabs; i++) {
		space += "   ";
	}
	printf("%sENTRY {\n", space.c_str());

	printf("%s   type: %s,\n", space.c_str(), this->typeToString());

	if(this->type == entry::STRING) {
		printf("%s   data: \"%s\",\n", space.c_str(), this->stringData);
	}
	else if(this->type == entry::NUMBER) {
		printf("%s   data: %f,\n", space.c_str(), this->numberData);
	}
	else if(this->type == entry::OBJECT) {
		printf("%s   data: 0x%lX,\n", space.c_str(), (long)this->objectData->objectWrapper);
		if(this->objectData->objectWrapper != nullptr) {
			printf("%s   variables:\n", space.c_str());
			this->objectData->objectWrapper->object->properties.printWithTab(2 + tabs);
		}
	}
	else if(this->type == entry::MATRIX) {
		printf("%s   data: 0x%lX,\n", space.c_str(), (long)this->matrixData);
		if(this->matrixData != nullptr) {
			printf("%s   rows: %u,\n", space.c_str(), this->matrixData->rows);
			printf("%s   columns: %u,\n", space.c_str(), this->matrixData->columns);
		}
	}
	else {
		printf("%s   data: no data,\n", space.c_str());
	}

	printf("%s};\n", space.c_str());
}

void ts::copyEntry(const Entry &source, Entry &destination) {
	destination.erase();
	
	destination.type = source.type;
	switch(destination.type) {
		case entry::INVALID: {
			break;
		}
		
		case entry::NUMBER: {
			destination.numberData = source.numberData;
			break;
		}

		case entry::STRING: {
			destination.stringData = cloneString(source.stringData);
			break;
		}

		case entry::MATRIX: {
			destination.matrixData = cloneMatrix(source.matrixData);
			break;
		}

		case entry::OBJECT: {
			destination.objectData = new ObjectReference(source.objectData);
			break;
		}
	}
}

void ts::greedyCopyEntry(Entry &source, Entry &destination) {
	destination.erase();
	
	destination.type = source.type;
	switch(destination.type) {
		case entry::INVALID: {
			break;
		}
		
		case entry::NUMBER: {
			destination.numberData = source.numberData;
			break;
		}

		case entry::STRING: {
			destination.stringData = source.stringData;
			source.stringData = nullptr;
			break;
		}

		case entry::MATRIX: {
			destination.matrixData = source.matrixData;
			source.matrixData = nullptr;
			break;
		}

		case entry::OBJECT: {
			destination.objectData = source.objectData;
			source.objectData = nullptr;
			break;
		}
	}

	source.type = entry::INVALID;
}

void ts::convertToType(Interpreter* interpreter, Entry &source, entry::EntryType type) {
	if(source.type == type) {
		return;
	}

	switch(type) {
		case entry::NUMBER: {
			## type_conversion.py source source.numberData STRING_OBJECT_MATRIX NUMBER "" interpreter
			break;
		}

		case entry::OBJECT: {
			ObjectWrapper* objectWrapper = nullptr;
			## type_conversion.py source objectWrapper NUMBER_STRING_MATRIX OBJECT "" interpreter
			source.objectData = new ObjectReference(objectWrapper);
			break;
		}

		case entry::MATRIX: {
			## type_conversion.py source source.matrixData NUMBER_OBJECT_STRING MATRIX "" interpreter
			break;
		}

		case entry::STRING: {
			## type_conversion.py source source.stringData NUMBER_OBJECT_MATRIX STRING "" interpreter
			break;
		}
	}

	source.type = type;
}

bool ts::isEntryEqual(const Entry &source, const Entry &destination) {
	if(source.type != destination.type) {
		return false;
	}

	switch(source.type) {
		case entry::NUMBER:
			return source.numberData == destination.numberData;
		
		case entry::STRING:
			return strcmp(source.stringData, destination.stringData) == 0;
		
		case entry::OBJECT:
			return source.objectData->objectWrapper == destination.objectData->objectWrapper;
	}

	return false;
}

void ts::initEntry(class Interpreter* interpreter, Entry* location) {;
	new((void*)location) Entry();
}