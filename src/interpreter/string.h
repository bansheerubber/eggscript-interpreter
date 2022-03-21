#pragma once

#include <cstdint>
#include <string>
#include <string.h>

namespace ts {
	struct String {
		char* string = nullptr;
		uint16_t size = 0;

		// copy string into our string buffer
		String(const char* input, uint16_t size) {
			this->string = new char[size];
			memcpy(this->string, input, size);
			this->size = size;
		}

		String(std::string input) {
			if(input.size() < ((1 << 16) - 1)) {
				this->string = new char[input.size()];
				memcpy(this->string, input.c_str(), input.size());
				this->size = input.size();
			}
		}

		String(String &string) {
			this->string = new char[string.size];
			memcpy(this->string, string.string, string.size);
			this->size = string.size;
		}

		String(const String* string) {
			this->string = new char[string->size];
			memcpy(this->string, string->string, string->size);
			this->size = string->size;
		}

		String(uint16_t size) {
			this->size = size;
			this->string = new char[size];
		}
	};
};
