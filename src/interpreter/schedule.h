#pragma once

#include <string>

#include "entry.h"

using namespace std;

namespace ts {
	struct Schedule {
		uint64_t time; // how long this schedule should be
		uint64_t start; // when this schedule started
		uint64_t end; // when the schedule should end
		string functionName;
		Entry* arguments;
		uint64_t argumentCount;
		ObjectReference* object;

		Schedule(
			uint64_t time,
			uint64_t start,
			string functionName,
			Entry* arguments,
			uint64_t argumentCount,
			class ObjectReference* object = nullptr
		) {
			this->time = time;
			this->start = start;
			this->functionName = functionName;
			this->arguments = arguments;
			this->argumentCount = argumentCount;
			this->object = object;

			this->end = time + start;
		}

		bool operator<(const Schedule &other) {
			return this->end < other.end;
		}

		bool operator>(const Schedule &other) {
			return this->end > other.end;
		}
	};
}
