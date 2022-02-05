import os
import pathlib
import re
from shutil import copyfile
import subprocess
import sys

total_lines = 0

def write_file(filename, contents):
	file = open(filename, "w", encoding='utf-8')
	for line in contents:
		file.write(line)
	file.close()

def handle_interpreter():
	file = open("tmp/interpreter/interpreter.cc")

	contents = []
	in_interpreter_function = False
	in_switch_statement = False
	brackets = 0

	function_contents = {}
	instruction = None

	for line in file:
		if "@perf" in line:
			line = line.replace("//", "").replace("@perf ", "")
			contents.append(line)
			continue
		
		if "Interpreter::interpret" in line:
			in_interpreter_function = True
		
		if in_interpreter_function and "switch(instruction.type)" in line:
			in_switch_statement = True
		elif in_switch_statement and brackets == 1:
			in_switch_statement = False
		
		if in_interpreter_function:
			if "{" in line:
				brackets += 1

			if "}" in line:
				brackets -= 1
		
		if brackets == 0 and in_interpreter_function:
			in_interpreter_function = False
			contents.append(line)

			# add instruction functions
			replacement = re.compile(r"(?<=[^a-zA-Z])this(?=[^a-zA-Z])")
			for instruction, function in function_contents.items():
				contents.append(f"void instruction_{instruction}(ts::Interpreter* interpreter, ts::Instruction &instruction) {{\n")

				for function_line in function:
					if instruction == "INVALID_INSTRUCTION":
						function_line = replacement.sub("interpreter", function_line).replace("return;", "exit(0);")
					else:
						function_line = replacement.sub("interpreter", function_line).replace("break;", "return;")
					contents.append(function_line)

				contents.append("}\n")

			continue
		
		if in_switch_statement:
			if "case" in line and brackets == 3:
				instruction = line.strip().split(" ")[1].replace("instruction", "").replace(":", "")
				if instruction not in function_contents:
					function_contents[instruction] = []
			elif brackets >= 3 and instruction:
				function_contents[instruction].append(line)
			continue

		contents.append(line)

	file.close()

	write_file("tmp/interpreter/interpreter.cc", contents)

def preprocess(filename, contents, directory = None):
	global total_lines
	
	if "include" not in filename:
		total_lines = total_lines + len(contents)
	
	pattern = r'^[\s]*?##(.+)$'
	new_contents = []
	for line in contents:
		if match := re.match(pattern, line):
			if directory == None:
				directory = str(pathlib.Path(filename).parent.as_posix())

			command = match.group(1).strip()

			if ".py" in command.split(" ")[0]:
				command = f"cd {directory} && python3 {command}"
			else:
				command = f"cd {directory} && {command}"

			process = os.popen(command)
			output = process.read()
			if process.close() != None:
				print("Encountered preprocessor error:", file=sys.stderr)
				print(output, file=sys.stderr)
				exit(1)
			else:
				new_contents.append(output)
		else:
			new_contents.append(line)
	return new_contents

if __name__ == "__main__":
	TS_INSTRUCTIONS_AS_FUNCTIONS = False
	if "-DTS_INSTRUCTIONS_AS_FUNCTIONS" in sys.argv:
		TS_INSTRUCTIONS_AS_FUNCTIONS = True

	include_file = open("./src/lib/libSymbols.h")
	functions = []
	for line in include_file:
		if match := re.search(r'[\w]+(?=\()', line):
			functions.append(match.group(0))

	include_file.close()

	# generate the map file
	global_symbols = ";".join(functions)
	map_contents = """eggscript {
		global: %%;
		local: *;
	};""".replace("%%", global_symbols)

	map_file = open("libeggscript.map", "w")
	map_file.write(map_contents)
	map_file.close()

	# generate the include files
	os.makedirs("./dist/include.cpp", exist_ok=True)
	copyfile("./src/lib/libSymbols.h", "./dist/include.cpp/egg.h")

	os.makedirs("./dist/include.c", exist_ok=True)
	include_c_source = open("./src/lib/libSymbols.h")
	include_c_destination = open("./dist/include.c/egg.h", "w")
	include_c_lines = []
	for line in include_c_source:
		if 'extern "C"' not in line and line.strip() != "}":
			include_c_lines.append(line.strip() + "\n")

	include_c_destination.writelines(include_c_lines)
	include_c_destination.close()
	include_c_source.close()

	for root, subdirs, files in os.walk("./src"):
		files = [f"{root}/{file}" for file in files]
		for file in files:
			file = file.replace("\\", "/")
			file_object = pathlib.Path(file)
			tmp_file = file.replace("./src/", "./tmp/")
			tmp_file_object = pathlib.Path(tmp_file)
			
			extension = file_object.suffix
			parent = file_object.parent
			tmp_parent = tmp_file_object.parent

			if extension == ".cc" or extension == ".h":
				opened_file = open(file_object, "r", encoding='utf-8')
				file_contents = opened_file.readlines()
				opened_file.close()
				
				if os.path.exists(tmp_file):
					src_time = file_object.stat().st_mtime
					tmp_time = tmp_file_object.stat().st_mtime

					if src_time > tmp_time: # recopy file if source file is newer than tmp file
						write_file(tmp_file, preprocess(file, file_contents))
						if "interpreter.cc" in tmp_file and TS_INSTRUCTIONS_AS_FUNCTIONS:
							handle_interpreter()
				else:
					os.makedirs(tmp_parent, exist_ok=True)
					write_file(tmp_file, preprocess(file, file_contents))
					if "interpreter.cc" in tmp_file and TS_INSTRUCTIONS_AS_FUNCTIONS:
						handle_interpreter()
