import sys
sys.path.insert(0, "../../tools")
from gen import get_generated_code

math_operations = {
	"MATH_ADDITION": (
		"this->pushEmpty(instruction.pushType);",
		"this->push({0} + {1}, instruction.pushType);",
		"this->push({0}->add({1}), instruction.pushType);",
	),
	"MATH_SUBTRACT": (
		"this->pushEmpty(instruction.pushType);",
		"this->push({0} - {1}, instruction.pushType);",
		"this->push({0}->subtract({1}), instruction.pushType);",
	),
	"MATH_MULTIPLY": (
		"this->pushEmpty(instruction.pushType);",
		"this->push({0} * {1}, instruction.pushType);",
		None,
	),
	"MATH_DIVISION": (
		"this->pushEmpty(instruction.pushType);",
		"this->push({0} / {1}, instruction.pushType);",
		None,
	),
	"MATH_MODULUS": (
		"this->pushEmpty(instruction.pushType);",
		"""if(((int){1}) == 0) {{
				this->push((double)0, instruction.pushType);
			}}
			else {{
				this->push((int){0} % (int){1}, instruction.pushType);
			}}
		""",
		None,
	),
	"MATH_SHIFT_LEFT": (
		"this->pushEmpty(instruction.pushType);",
		"this->push((int){0} << (int){1}, instruction.pushType);",
		None,
	),
	"MATH_SHIFT_RIGHT": (
		"this->pushEmpty(instruction.pushType);",
		"this->push((int){0} >> (int){1}, instruction.pushType);",
		None,
	),
	"MATH_LESS_THAN_EQUAL": (
		"this->push(0.0, instruction.pushType);",
		"this->push({0} <= {1}, instruction.pushType);",
		None,
	),
	"MATH_GREATER_THAN_EQUAL": (
		"this->push(0.0, instruction.pushType);",
		"this->push({0} >= {1}, instruction.pushType);",
		None,
	),
	"MATH_LESS_THAN": (
		"this->push(0.0, instruction.pushType);",
		"this->push({0} < {1}, instruction.pushType);",
		None,
	),
	"MATH_GREATER_THAN": (
		"this->push(0.0, instruction.pushType);",
		"this->push({0} > {1}, instruction.pushType);",
		None,
	),
	"MATH_BITWISE_AND": (
		"this->pushEmpty(instruction.pushType);",
		"this->push((int){0} & (int){1}, instruction.pushType);",
		None,
	),
	"MATH_BITWISE_OR": (
		"this->pushEmpty(instruction.pushType);",
		"this->push((int){0} | (int){1}, instruction.pushType);",
		None,
	),
	"MATH_BITWISE_XOR": (
		"this->pushEmpty(instruction.pushType);",
		"this->push((int){0} ^ (int){1}, instruction.pushType);",
		None,
	),
}

# operations shared between all types
common_operations = {
	"MATH_EQUAL": "this->push(isEntryEqual({0}, {1}), instruction.pushType);",
	"MATH_NOT_EQUAL": "this->push(!isEntryEqual({0}, {1}), instruction.pushType);",
}

string_operations = {
	"MATH_STRING_EQUAL": """bool result = stringCompareInsensitive({0}, {1}) == true;
			%%popStrings%%
			this->push(result, instruction.pushType);""",
	"MATH_STRING_NOT_EQUAL": """bool result = stringCompareInsensitive({0}, {1}) == false;
			%%popStrings%%
			this->push(result, instruction.pushType);""",
	"MATH_APPEND": """size_t firstSize = strlen({0}), secondSize = strlen({1});
			char* stringResult = new char[firstSize + secondSize + 1];
			memcpy(stringResult, {0}, firstSize);
			memcpy(&stringResult[firstSize], {1}, secondSize);
			stringResult[firstSize + secondSize] = '\\0';

			%%popStrings%%

			this->push(stringResult, instruction.pushType);""",
	"MATH_SPC": """size_t firstSize = strlen({0}), secondSize = strlen({1});
			char* stringResult = new char[firstSize + secondSize + 2];
			memcpy(stringResult, {0}, firstSize);
			stringResult[firstSize] = ' ';
			memcpy(&stringResult[firstSize + 1], {1}, secondSize);
			stringResult[firstSize + secondSize + 1] = '\\0';

			%%popStrings%%
			
			this->push(stringResult, instruction.pushType);""",
	"MATH_TAB": """size_t firstSize = strlen({0}), secondSize = strlen({1});
			char* stringResult = new char[firstSize + secondSize + 2];
			memcpy(stringResult, {0}, firstSize);
			stringResult[firstSize] = '\\t';
			memcpy(&stringResult[firstSize + 1], {1}, secondSize);
			stringResult[firstSize + secondSize + 1] = '\\0';

			%%popStrings%%

			this->push(stringResult, instruction.pushType);""",
	"MATH_NL": """size_t firstSize = strlen({0}), secondSize = strlen({1});
			char* stringResult = new char[firstSize + secondSize + 2];
			memcpy(stringResult, {0}, firstSize);
			stringResult[firstSize] = '\\n';
			memcpy(&stringResult[firstSize + 1], {1}, secondSize);
			stringResult[firstSize + secondSize + 1] = '\\0';

			%%popStrings%%

			this->push(stringResult, instruction.pushType);""",
}

string_pop = """if(popLValue) {{
				this->pop();
			}}

			if(popRValue) {{
				this->pop();
			}}"""

math_body = """if(type1 != type2) {{
	{0}
}}
else if(type1 == entry::NUMBER) {{
	{1}
}}
else if(type1 == entry::MATRIX) {{
	{2}
}}
else {{
	{0}
}}"""

NUMBER_MATH_MACRO = get_generated_code("math", "numbers", 3)

# handle number instructions
for instruction, (failure, number_operation, matrix_operation) in math_operations.items():
	if matrix_operation == None:
		matrix_operation = failure
	
	formatted = number_operation.format("lvalueNumber", "rvalueNumber")
	formatted = math_body.format(failure, formatted, matrix_operation.format("lvalueMatrix", "rvalueMatrix"))

	print(f"""		case instruction::{instruction}: {{
{NUMBER_MATH_MACRO}

			{formatted}
			break;
		}}\n""")

COMMON_MATH_MACRO = get_generated_code("math", "common", 3)

# handle common instructions
for instruction, operation in common_operations.items():
	formatted = operation.format("*lvalue", "*rvalue")

	print(f"""		case instruction::{instruction}: {{
{COMMON_MATH_MACRO}

			{formatted}
			break;
		}}\n""")

STRING_MATH_MACRO = get_generated_code("math", "strings", 3)

# handle string instructions
for instruction, operation in string_operations.items():
	formatted = operation.format("lvalueString", "rvalueString")
	DELETE_STRINGS_MACRO = get_generated_code("math", "deleteStrings", 3)

	print(f"""		case instruction::{instruction}: {{
{STRING_MATH_MACRO}

			{formatted.replace("%%popStrings%%", string_pop)}
{DELETE_STRINGS_MACRO}
			break;
		}}\n""")