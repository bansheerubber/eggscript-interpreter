#pragma once

#include "class.h"
#include "loops.h"
#include "package.h"
#include "scope.h"

namespace ts {
	struct CompilationContext {
		LoopsContext* loop;
		PackageContext* package;
		Scope* scope;
		ClassContext* classContext;
	};
}
