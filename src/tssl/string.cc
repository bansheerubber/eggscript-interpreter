#include "simObject.h"

#include <string.h>

#include "../util/cloneString.h"
#include "../util/collapseEscape.h"
#include "../engine/engine.h"
#include "../util/numberToHex.h"

namespace ts {
	namespace sl {
		Entry* strLen(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				return new Entry(args[0].stringData->size);
			}
			return nullptr;
		}

		Entry* getSubStr(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 3) {
				if(args[2].numberData < 0) {
					return nullptr;
				}
				
				string substr = string(args[0].stringData->string, args[0].stringData->size).substr(args[1].numberData, args[2].numberData);
				return new Entry(new ts::String(substr));
			}
			return nullptr;
		}

		Entry* strPos(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 2) {
				string substring(args[1].stringData->string, args[1].stringData->size);
				uint64_t position = string(args[0].stringData->string, args[0].stringData->size).find(substring);
				if (position == std::string::npos) {
					return new Entry(-1);
				}
				else {
					return new Entry(position);
				}
			}
			else if(argc == 3) {
				string substring(args[1].stringData->string, args[1].stringData->size);
				uint64_t position = string(args[0].stringData->string, args[0].stringData->size).find(substring, args[2].numberData);
				if (position == std::string::npos) {
					return new Entry(-1);
				}
				else {
					return new Entry(position);
				}
			}

			return new Entry(-1);
		}

		Entry* trim(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				ts::String* words = args[0].stringData;
				bool foundFirst = false;
				uint64_t firstIndex = 0, secondIndex = 0, length = 0;
				for(uint16_t i = 0; i < words->size; i++) {
					char character = words->string[i];
					if(
						character == ' '
						|| character == '\t'
						|| character == '\n'
						|| character == '\r'
					) {
						secondIndex++;
					}
					else {
						foundFirst = true;
						secondIndex = 0;
					}

					if(!foundFirst) {
						firstIndex++;
					}

					length++;
				}

				if(!foundFirst) {
					secondIndex = 0;
				}

				return new Entry(new ts::String(&args[0].stringData->string[firstIndex], length - firstIndex - secondIndex));
			}

			return nullptr;
		}

		Entry* ltrim(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				ts::String* words = args[0].stringData;
				bool foundFirst = false;
				uint64_t firstIndex = 0;
				for(uint16_t i = 0; i < words->size; i++) {
					char character = words->string[i];
					if(
						character != ' '
						&& character != '\t'
						&& character != '\n'
						&& character != '\r'
					) {
						foundFirst = true;
					}

					if(!foundFirst) {
						firstIndex++;
					}
				}

				return new Entry(new ts::String(&args[0].stringData->string[firstIndex], args[0].stringData->size - firstIndex));
			}

			return nullptr;
		}

		Entry* rtrim(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				ts::String* words = args[0].stringData;
				bool foundFirst = false;
				uint64_t secondIndex = 0, length = 0;
				for(uint16_t i = 0; i < words->size; i++) {
					char character = words->string[i];
					if(
						character == ' '
						|| character == '\t'
						|| character == '\n'
						|| character == '\r'
					) {
						foundFirst = true;
						secondIndex++;
					}
					else {
						secondIndex = 0;
					}

					length++;
				}

				if(!foundFirst) {
					secondIndex = 0;
				}

				return new Entry(new ts::String(args[0].stringData->string, length - secondIndex));
			}
			
			return nullptr;
		}

		Entry* strCmp(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 2) {
				return new Entry(strncmp(args[0].stringData->string, args[1].stringData->string, std::min(args[0].stringData->size, args[1].stringData->size)));
			}

			return new Entry(0.0);
		}

		Entry* strICmp(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 2) {
				return new Entry(strncasecmp(args[0].stringData->string, args[1].stringData->string, std::min(args[0].stringData->size, args[1].stringData->size)));
			}

			return new Entry(0.0);
		}

		Entry* strLwr(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				ts::String* words = args[0].stringData;
				ts::String* output = new ts::String(words->size);
				for(uint16_t i = 0; i < words->size; i++) {
					char character = words->string[i];
					output->string[i] = tolower(character);
				}
				return new Entry(output);
			}

			return nullptr;
		}

		Entry* strUpr(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				ts::String* words = args[0].stringData;
				ts::String* output = new ts::String(words->size);
				for(uint16_t i = 0; i < words->size; i++) {
					char character = words->string[i];
					output->string[i] = toupper(character);
				}
				return new Entry(new ts::String(output));
			}

			return nullptr;
		}

		Entry* strChr(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 2) {
				if(args[1].stringData->size == 0) {
					return nullptr;
				}

				const char search = args[1].stringData->string[0];
				char* result = (char*)memchr(args[0].stringData->string, search, args[0].stringData->size);
				if(result) {
					return new Entry(new ts::String(result, args[0].stringData->size - (result - args[0].stringData->string)));
				}
			}

			return nullptr;
		}

		Entry* stripChars(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 2) {
				string output;
				ts::String* source = args[0].stringData;
				ts::String* replace = args[1].stringData;
				for(uint16_t i = 0; i < source->size; i++) {
					char sourceChar = source->string[i];

					bool found = false;
					for(uint16_t j = 0; j < replace->size; j++) {
						char replaceChar = replace->string[j];
						if(sourceChar == replaceChar) {
							found = true;
							break;
						}
					}

					if(found == false) {
						output += sourceChar;
					}
				}

				return new Entry(new ts::String(output));
			}
			
			return nullptr;
		}

		Entry* _collapseEscape(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				string value(args[0].stringData->string, args[0].stringData->size);
				string collapsed = collapseEscape(value);
				return new Entry(new ts::String(collapsed));
			}

			return nullptr;
		}

		Entry* expandEscape(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				const char* lookup[18] = {
					"",			// 0x00
					"\\c0", // 0x01 black
					"\\c1", // 0x02 gray
					"\\c2", // 0x03 white
					"\\c3", // 0x04 red
					"\\c4", // 0x05 green
					"\\c5", // 0x06 yellow
					"\\c6", // 0x07 blue
					"\\b",	// 0x08
					"\\t",	// 0x09
					"\\n",	// 0x0A
					"\\c7",	// 0x0B purple
					"\\c8",	// 0x0C cyan
					"\\r",	// 0x0D
					"\\c9",	// 0x0E black
					"\\cr",	// 0x0F
					"\\cp",	// 0x10
					"\\co",	// 0x11
				};
				
				string output;
				ts::String* source = args[0].stringData;
				for(uint16_t i = 0; i < source->size; i++) {
					char character = source->string[i];
					if(character <= 17) {
						output += lookup[(int)character];
					}
					else if(character < 32) {
						output += "\\x";
						output += numberToHex(character);
					}
					else {
						output += character;
					}
				}

				return new Entry(new ts::String(output));
			}
			
			return nullptr;
		}

		Entry* strReplace(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 3) {
				ts::String* words = args[0].stringData;
				ts::String* search = args[1].stringData;
				ts::String* replacement = args[2].stringData;
				uint16_t searchLength = search->size;
				string output;

				uint16_t searchIndex = 0;

				char replacementCharacter;
				uint64_t count = 0;
				for(uint16_t i = 0; i < words->size; i++) {
					char character = words->string[i];

					if((replacementCharacter = search->string[searchIndex]) == '\0') {
						output = output.erase(count - searchLength, searchLength);
						output += string(replacement->string, replacement->size);
						searchIndex = 0;
						count = output.length();
					}
					else if(replacementCharacter == character) {
						searchIndex++;
					}
					else {
						searchIndex = 0;
					}

					output += character;
					count++;
				}

				return new Entry(new ts::String(output));
			}

			return nullptr;
		}
	}
}
