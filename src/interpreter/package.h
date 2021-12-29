#pragma once

#include <string>
#include <vector>

#include "function.h"

using namespace std;

namespace std {
	template<>
	struct hash<pair<string, string>> {
		uint64_t operator()(pair<string, string> const& source) const noexcept {
			uint64_t result = hash<string>{}(source.first);
			return result ^ (hash<string>{}(source.second) + 0x9e3779b9 + (result << 6) + (result >> 2));
    }
	};

	template<>
	struct equal_to<pair<string, string>> {
		bool operator()(const pair<string, string>& x, const pair<string, string>& y) const {
			return x.first == y.first && x.second == y.second;
		}
	};
};

namespace ts {
	class Package {
		public:
			Package(class Engine* engine);
			
			void addPackageFunction(string name, Function* function);
			void removeFunction(string function);
			void addPackageMethod(string nameSpace, string name, Function* function);
			void removeMethod(string nameSpace, string function);
		
		private:
			class Engine* engine;
			
			string name;
			robin_map<string, Function*> nameToFunction;
			robin_map<pair<string, string>, Function*> nameToMethod;
	};
};
