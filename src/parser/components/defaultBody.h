#pragma once

#include <vector>

#include "component.h"
#include "body.h"
#include "../parser.h"
#include "../../tokenizer/token.h"
#include "../../tokenizer/tokenizer.h"

using namespace std;

class DefaultBody : public Body {
	public:
		using Body::Body;
		
		ComponentType getType() {
			return DEFAULT_STATEMENT;
		}

		bool requiresSemicolon(Component* child) {
			return true;
		}

		string print();
		static bool ShouldParse(Tokenizer* tokenizer, class Parser* parser);
		static DefaultBody* Parse(Body* body, Tokenizer* tokenizer, class Parser* parser);
};