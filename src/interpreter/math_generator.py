import sys
sys.path.insert(0, "../../tools")
from gen import get_generated_code

number_operations = {
	"MATH_ADDITION": "this->push({0} + {1}, instruction.pushType);",
	"MATH_SUBTRACT": "this->push({0} - {1}, instruction.pushType);",
	"MATH_MULTIPLY": "this->push({0} * {1}, instruction.pushType);",
	"MATH_DIVISION": "this->push({0} / {1}, instruction.pushType);",
	"MATH_MODULUS": """if(((int){1}) == 0) {{
			this->push((double)0, instruction.pushType);
		}}
		else {{
			this->push((int){0} % (int){1}, instruction.pushType);
		}}
	""",
	"MATH_SHIFT_LEFT": "this->push((int){0} << (int){1}, instruction.pushType);",
	"MATH_SHIFT_RIGHT": "this->push((int){0} >> (int){1}, instruction.pushType);",
	"MATH_EQUAL": "this->push({0} == {1}, instruction.pushType);",
	"MATH_NOT_EQUAL": "this->push({0} != {1}, instruction.pushType);",
	"MATH_LESS_THAN_EQUAL": "this->push({0} <= {1}, instruction.pushType);",
	"MATH_GREATER_THAN_EQUAL": "this->push({0} >= {1}, instruction.pushType);",
	"MATH_LESS_THAN": "this->push({0} < {1}, instruction.pushType);",
	"MATH_GREATER_THAN": "this->push({0} > {1}, instruction.pushType);",
	"MATH_BITWISE_AND": "this->push((int){0} & (int){1}, instruction.pushType);",
	"MATH_BITWISE_OR": "this->push((int){0} | (int){1}, instruction.pushType);",
	"MATH_BITWISE_XOR": "this->push((int){0} ^ (int){1}, instruction.pushType);",
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

number_body = """if(type1 != type2) {{
	this->pushEmpty(instruction.pushType);
}}
else if(type1 == entry::NUMBER) {{
	{0}
}}
else {{
	this->pushEmpty(instruction.pushType);
}}"""

NUMBER_MATH_MACRO = get_generated_code("math", "numbers", 3)

# handle number instructions
for instruction, operation in number_operations.items():
	formatted = operation.format("lvalueNumber", "rvalueNumber")
	formatted = number_body.format(formatted)

	print(f"""		case instruction::{instruction}: {{
{NUMBER_MATH_MACRO}

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