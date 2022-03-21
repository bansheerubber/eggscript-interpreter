#include "fileObject.h"

#include "../util/cloneString.h"
#include "../engine/engine.h"
#include "../util/getEmptyString.h"
#include "../util/stringToChars.h"

namespace ts {
	namespace sl {
		void FileObject::open(ts::String* fileName, FileObjectMode mode) {
			ios_base::openmode fileMode;
			if(mode == READ) {
				fileMode = ios_base::in;
			}
			else if(mode == WRITE) {
				fileMode = ios_base::trunc;
			}
			else if(mode == APPEND) {
				fileMode = ios_base::app;
			}

			this->file.open(string(fileName->string, fileName->size), fileMode);
			this->mode = mode;
		}

		void FileObject::close() {
			this->file.close();
		}

		ts::String* FileObject::readLine() {
			if(this->mode != READ || this->isEOF()) {
				return getEmptyString();
			}
			
			string output;
			getline(this->file, output);

			if(output.length() > 0 && output[output.length() - 1] == '\r') {
				output.pop_back();
			}

			return new ts::String(output.data(), output.size());
		}

		void FileObject::writeLine(ts::String* string) {
			if(this->mode == READ || this->mode == NOT_OPEN) {
				return;
			}

			this->file.write(string->string, string->size);
		}

		bool FileObject::isEOF() {
			return this->file.eof();
		}
		
		void FileObject__constructor(ObjectWrapper* wrapper) {
			wrapper->data = new FileObject();
		}

		Entry* FileObject__openForRead(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 2 && args[0].objectData->objectWrapper->object->methodTree->name == "FileObject") {
				((FileObject*)args[0].objectData->objectWrapper->data)->open(args[1].stringData, READ);
			}
			
			return nullptr;
		}

		Entry* FileObject__openForWrite(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 2 && args[0].objectData->objectWrapper->object->methodTree->name == "FileObject") {
				((FileObject*)args[0].objectData->objectWrapper->data)->open(args[1].stringData, WRITE);
			}
			
			return nullptr;
		}

		Entry* FileObject__openForAppend(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 2 && args[0].objectData->objectWrapper->object->methodTree->name == "FileObject") {
				((FileObject*)args[0].objectData->objectWrapper->data)->open(args[1].stringData, APPEND);
			}
			
			return nullptr;
		}

		Entry* FileObject__close(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1 && args[0].objectData->objectWrapper->object->methodTree->name == "FileObject") {
				((FileObject*)args[0].objectData->objectWrapper->data)->close();
			}
			
			return nullptr;
		}

		Entry* FileObject__readLine(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1 && args[0].objectData->objectWrapper->object->methodTree->name == "FileObject") {
				return new Entry(((FileObject*)args[0].objectData->objectWrapper->data)->readLine());
			}
			
			return nullptr;
		}

		Entry* FileObject__writeLine(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 2 && args[0].objectData->objectWrapper->object->methodTree->name == "FileObject") {
				((FileObject*)args[0].objectData->objectWrapper->data)->writeLine(args[1].stringData);
			}
			
			return nullptr;
		}

		Entry* FileObject__isEOF(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1 && args[0].objectData->objectWrapper->object->methodTree->name == "FileObject") {
				return new Entry((double)((FileObject*)args[0].objectData->objectWrapper->data)->isEOF());
			}
			
			return nullptr;
		}

		Entry* fileBase(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				ts::String* path = args[0].stringData;
				const char* slashLocation = (const char*)memrchr(path->string, '/', path->size);
				if(slashLocation == nullptr) {
					slashLocation = path->string;
				}
				else {
					slashLocation++;
				}

				const char* dotLocation = (const char*)memrchr(path->string, '.', path->size);
				
				uint64_t length = dotLocation
					? dotLocation - slashLocation
					: strlen(slashLocation);
				
				return new Entry(new ts::String(slashLocation, length));
			}

			return nullptr;
		}

		Entry* fileExt(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				ts::String* path = args[0].stringData;
				const char* dotLocation = (const char*)memrchr(path->string, '.', path->size);
				if(dotLocation == nullptr) {
					return new Entry(getEmptyString());
				}

				// TODO is this even close to being portable?
				return new Entry(new ts::String(dotLocation, path->size - (dotLocation - path->string)));
			}

			return nullptr;
		}

		Entry* fileName(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				ts::String* path = args[0].stringData;
				const char* slashLocation = (const char*)memrchr(path, '/', path->size);
				if(slashLocation == nullptr) {
					slashLocation = path->string;
				}
				else {
					slashLocation++;
				}

				return new Entry(new ts::String(slashLocation, path->size - (slashLocation - path->string)));
			}

			return nullptr;
		}

		Entry* filePath(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				ts::String* path = args[0].stringData;
				const char* slashLocation = (const char*)memrchr(path, '/', path->size);
				if(slashLocation == nullptr) {
					return new Entry(new ts::String(path));
				}

				return new Entry(new ts::String(path->string, slashLocation - path->string));
			}

			return nullptr;
		}
	}
}
