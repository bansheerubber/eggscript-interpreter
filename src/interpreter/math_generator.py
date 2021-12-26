import sys
sys.path.insert(0, "../../tools")
from gen import get_generated_code

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

math_body_without_matrix = """if(type1 != type2) {{
	{0}
}}
else if(type1 == entry::NUMBER) {{
	{1}
}}
else {{
	{0}
}}"""

multiplication_body = """if((type1 == entry::NUMBER && type2 == entry::MATRIX) || (type1 == entry::MATRIX && type2 == entry::NUMBER)) {{
	{2}
}}
else if(type1 != type2) {{
	{0}
}}
else if(type1 == entry::NUMBER) {{
	{1}
}}
else {{
	{0}
}}"""

division_body = """if(type1 == entry::MATRIX && type2 == entry::NUMBER) {{
	{2}
}}
else if(type1 != type2) {{
	{0}
}}
else if(type1 == entry::NUMBER) {{
	{1}
}}
else {{
	{0}
}}"""

math_operations = {
	"MATH_ADDITION": (
		math_body,
		"this->pushEmpty(instruction.pushType);",
		"this->push({0} + {1}, instruction.pushType);",
		"this->push({0}->add({1}), instruction.pushType);",
	),
	"MATH_SUBTRACT": (
		math_body,
		"this->pushEmpty(instruction.pushType);",
		"this->push({0} - {1}, instruction.pushType);",
		"this->push({0}->subtract({1}), instruction.pushType);",
	),
	"MATH_MULTIPLY": (
		multiplication_body,
		"this->pushEmpty(instruction.pushType);",
		"this->push({0} * {1}, instruction.pushType);",
		"type1 == entry::NUMBER ? this->push({1}->multiply(lvalueNumber), instruction.pushType) : this->push({0}->multiply(rvalueNumber), instruction.pushType);",
	),
	"MATH_DIVISION": (
		division_body,
		"this->pushEmpty(instruction.pushType);",
		"this->push({0} / {1}, instruction.pushType);",
		"this->push({0}->multiply(1.0 / rvalueNumber), instruction.pushType);",
	),
	"MATH_MODULUS": (
		math_body_without_matrix,
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
		math_body_without_matrix,
		"this->pushEmpty(instruction.pushType);",
		"this->push((int){0} << (int){1}, instruction.pushType);",
		None,
	),
	"MATH_SHIFT_RIGHT": (
		math_body_without_matrix,
		"this->pushEmpty(instruction.pushType);",
		"this->push((int){0} >> (int){1}, instruction.pushType);",
		None,
	),
	"MATH_LESS_THAN_EQUAL": (
		math_body_without_matrix,
		"this->push(0.0, instruction.pushType);",
		"this->push({0} <= {1}, instruction.pushType);",
		None,
	),
	"MATH_GREATER_THAN_EQUAL": (
		math_body_without_matrix,
		"this->push(0.0, instruction.pushType);",
		"this->push({0} >= {1}, instruction.pushType);",
		None,
	),
	"MATH_LESS_THAN": (
		math_body_without_matrix,
		"this->push(0.0, instruction.pushType);",
		"this->push({0} < {1}, instruction.pushType);",
		None,
	),
	"MATH_GREATER_THAN": (
		math_body_without_matrix,
		"this->push(0.0, instruction.pushType);",
		"this->push({0} > {1}, instruction.pushType);",
		None,
	),
	"MATH_BITWISE_AND": (
		math_body_without_matrix,
		"this->pushEmpty(instruction.pushType);",
		"this->push((int){0} & (int){1}, instruction.pushType);",
		None,
	),
	"MATH_BITWISE_OR": (
		math_body_without_matrix,
		"this->pushEmpty(instruction.pushType);",
		"this->push((int){0} | (int){1}, instruction.pushType);",
		None,
	),
	"MATH_BITWISE_XOR": (
		math_body_without_matrix,
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

NUMBER_MATH_MACRO = get_generated_code("math", "numbers", 3)
NUMBER_WITHOUT_MATRIX_MATH_MACRO = get_generated_code("math", "numbersWithoutMatrix", 3)
NUMBER_DIVISION_MATH_MACRO = get_generated_code("math", "numbersDivision", 3)

# handle number instructions
for instruction, (body, failure, number_operation, matrix_operation) in math_operations.items():
	if matrix_operation == None:
		matrix_operation = failure
	
	formatted = number_operation.format("lvalueNumber", "rvalueNumber")
	formatted = body.format(failure, formatted, matrix_operation.format("lvalueMatrix", "rvalueMatrix"))

	macro = NUMBER_MATH_MACRO
	if body == math_body_without_matrix:
		macro = NUMBER_WITHOUT_MATRIX_MATH_MACRO
	elif body == division_body:
		macro = NUMBER_DIVISION_MATH_MACRO


	print(f"""		case instruction::{instruction}: {{
{macro}

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