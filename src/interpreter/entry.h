#pragma once

#include <string>

#include "linearAlgebra.h"
#include "objectReference.h"

using namespace std;

namespace ts {
	namespace entry {
		enum EntryType {
			EMPTY,
			NUMBER,
			STRING,
			OBJECT,
			MATRIX,
		};
	}
	
	struct Entry {
		entry::EntryType type;
		union {
			double numberData;

			/*
				strings are freed every once in a while depending on what the interpreter is doing.
				- strings are always deleted when their parent entries are popped from the stack
				- strings are deleted from the destination during a `copyEntry` operation
				- strings are overwritten and deleted when using `setString` on an entry with string type
				these happen in some bizzare places due to optimization problems. freeing strings every
				time we did an entry type conversion proved to be too slow while this method is faster.
				i'm trying to keep number operations as quick as possible, and this was the way i found
				to preserve performance
			*/
			char* stringData;
			ObjectReference* objectData;
			Matrix* matrixData;
		};

		Entry();
		Entry(const Entry &entry);
		Entry(double value);
		Entry(char* value);
		Entry(Matrix* value);
		Entry(ObjectReference* value);
		~Entry();
		void setNumber(double value);
		void setString(char* value);
		void setString(string value);
		void setMatrix(Matrix* value);
		void setObject(ObjectReference* value);

		inline void erase() {
			if(this->type == entry::STRING && this->stringData != nullptr) {
				delete[] this->stringData;
				this->stringData = nullptr;
			}

			if(this->type == entry::OBJECT && this->objectData != nullptr) {
				delete this->objectData;
				this->objectData = nullptr;
			}

			if(this->type == entry::MATRIX && this->matrixData != nullptr) {
				delete this->matrixData;
				this->matrixData = nullptr;
			}

			this->type = entry::EMPTY;
		}
		
		void print(int tabs = 0) const;
		const char* typeToString() const;
	};

	void copyEntry(const Entry &source, Entry &destination);
	void greedyCopyEntry(Entry &source, Entry &destination);
	void convertToType(class Interpreter* interpreter, Entry &source, entry::EntryType type);
	void initEntry(class Interpreter* interpreter, Entry* location);
	bool isEntryEqual(const Entry &source, const Entry &destination);
	bool isEntryTruthy(const Entry &source);
}

namespace std {
	template<> // specialization
	void swap<ts::Entry>(ts::Entry &entry1, ts::Entry &entry2) noexcept;
}
