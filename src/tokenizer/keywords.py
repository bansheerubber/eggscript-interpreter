keywords = {
	"!=": "NOT_EQUAL",
	"==": "EQUAL",
	"<=": "LESS_THAN_EQUAL",
	">=": "GREATER_THAN_EQUAL",
	"<": "LESS_THAN",
	">": "GREATER_THAN",
	"&&": "LOGICAL_AND",
	"||": "LOGICAL_OR",
	"++": "INCREMENT",
	"--": "DECREMENT",
	"<<": "SHIFT_LEFT",
	">>": "SHIFT_RIGHT",
	"+=": "PLUS_ASSIGN",
	"-=": "MINUS_ASSIGN",
	"/=": "SLASH_ASSIGN",
	"*=": "ASTERISK_ASSIGN",
	"|=": "OR_ASSIGN",
	"&=": "AND_ASSIGN",
	"^=": "XOR_ASSIGN",
	"<<=": "SHIFT_LEFT_ASSIGN",
	">>=": "SHIFT_RIGHT_ASSIGN",
	"=": "ASSIGN",
	"!": "LOGICAL_NOT",
	"%": "MODULUS",
	"&": "BITWISE_AND",
	"|": "BITWISE_OR",
	"^": "BITWISE_XOR",
	"?": "QUESTION_MARK",
	":": "COLON",
	"@": "APPEND",
	"~": "BITWISE_NOT",
	"+": "PLUS",
	"-": "MINUS",
	"*": "ASTERISK",
	"/": "SLASH",
	".": "DOT_PRODUCT",
	"::": "NAMESPACE",
	"(": "LEFT_PARENTHESIS",
	")": "RIGHT_PARENTHESIS",
	"[": "LEFT_BRACE",
	"]": "RIGHT_BRACE",
	"{": "LEFT_BRACKET",
	"}": "RIGHT_BRACKET",
	";": "SEMICOLON",
	",": "COMMA",
	"return": "RETURN",
	"package": "PACKAGE",
	"new": "NEW",
	"function": "FUNCTION",
	"if": "IF",
	"else": "ELSE",
	"while": "WHILE",
	"for": "FOR",
	"switch": "SWITCH",
	"case": "CASE",
	"or": "OR",
	"default": "DEFAULT",
	"parent": "PARENT",
	"continue": "CONTINUE",
	"break": "BREAK",
	"datablock": "DATABLOCK",
	"true": "TRUE",
	"false": "FALSE",
	"spc": "SPC",
	"tab": "TAB",
	"nl": "NL",
	"null": "EMPTY",
	"class": "CLASS",
	"inherits": "INHERITS",
}

for syntax, token_type in keywords.items():
	print(f'\tthis->validKeywords.insert(pair<string, TokenType>("{syntax}", {token_type}));')