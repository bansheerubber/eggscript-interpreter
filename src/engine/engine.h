#pragma once

#include <string>
#include <vector>

#ifdef __linux__
#include <termios.h>
#endif

#include "../args.h"
#include "../util/dynamicArray.h"
#include "../interpreter/function.h"
#include "../interpreter/interpreter.h"
#include "../io.h"
#include "../interpreter/methodTree.h"
#include "../compiler/package.h"
#include "../interpreter/package.h"
#include "../interpreter/packagedFunctionList.h"
#include "../parser/parser.h"
#include "../include/robin-map/include/tsl/robin_map.h"
#include "../include/robin-map/include/tsl/robin_set.h"
#include "../tokenizer/tokenizer.h"

using namespace std;

class FunctionDeclaration;
class NewStatement;

extern struct termios originalTerminalAttributes;

void disableRawMode();

namespace ts {
	struct InstructionSource {
		string fileName;
	};

	struct InstructionDebug {
		InstructionSource* commonSource = nullptr;
		unsigned short character;
		unsigned int line;
	};
	
	void initPackagedFunctionList(class Engine* engine, PackagedFunctionList** list);
	void initMethodTree(class Engine* engine, MethodTree** tree);

	namespace sl {
		Entry* SimObject__isMethod(Engine* engine, unsigned int argc, Entry* args);
	};
	
	class Engine {
		friend Interpreter;
		friend Package;
		friend FunctionDeclaration;
		friend NewStatement;
		friend Instruction;
		friend void sl::define(Engine* engine);
		friend void copyInstruction(Engine* engine, Instruction &source, Instruction &destination);
		friend Entry* sl::SimObject__isMethod(Engine* engine, unsigned int argc, Entry* args);

		public:
			Engine(ParsedArguments args, bool isParallel = false);
			~Engine();

			Tokenizer* tokenizer;
			Parser* parser;
			Interpreter* interpreter;

			void execFile(string fileName, bool forceExecution = false);
			void execVirtualFile(string fileName, string shell);
			void execPiped(string piped);
			void execShell(string shell, bool forceExecution = false);

			void link();

			esPrintFunction(printFunction) = &printf;
			esPrintFunction(warningFunction) = &printWarning;
			esPrintFunction(errorFunction) = &printError;

			esVPrintFunction(vPrintFunction) = &vprintf;
			esVPrintFunction(vWarningFunction) = &printWarning;
			esVPrintFunction(vErrorFunction) = &printError;

			void enterShell();

			double getRandom();
			void setRandomSeed(int seed);
			int getRandomSeed();

			void defineFunction(string &name, InstructionReturn output, uint64_t argumentCount, uint64_t variableCount);
			void defineMethod(string &nameSpace, string &name, InstructionReturn output, uint64_t argumentCount, uint64_t variableCount);
			void defineTSSLFunction(sl::Function* function);
			void defineTSSLMethodTree(MethodTree* tree);

			MethodTree* createMethodTreeFromNamespace(string nameSpace);
			MethodTree* getNamespace(string nameSpace);

			const InstructionDebug& getInstructionDebug(Instruction* instruction);
			void setInstructionDebugEnabled(bool instructionDebugEnabled);

			void addUnlinkedInstruction(Instruction* instruction);

			void printUnlinkedInstructions();

			void printSubTypeCounts();
		
		private:
			ParsedArguments args;

			string shellBuffer = "";

			void shellPrintBuffer();

			queue<string> fileQueue; // queue for parallel interpreter file execution
			queue<string> shellQueue; // queue for parallel interpreter shell execution

			int randomSeed;

			Package* createPackage(PackageContext* package);

			void addPackageFunction(PackageContext* package, string &name, InstructionReturn output, uint64_t argumentCount, uint64_t variableCount);
			void addPackageMethod(PackageContext* package, string &nameSpace, string &name, InstructionReturn output, uint64_t argumentCount, uint64_t variableCount);

			// function data structures
			robin_map<string, uint64_t> nameToFunctionIndex;
			DynamicArray<PackagedFunctionList*, Engine> functions
				= DynamicArray<PackagedFunctionList*, Engine>(this, 1024, initPackagedFunctionList, nullptr);

			robin_map<string, uint64_t> namespaceToMethodTreeIndex;
			DynamicArray<MethodTree*, Engine> methodTrees = DynamicArray<MethodTree*, Engine>(this, 1024, initMethodTree, nullptr);

			robin_map<string, Package*> nameToPackage;

			// used to index into a method tree
			robin_map<string, uint64_t> methodNameToIndex;
			uint64_t currentMethodNameIndex = 0;

			// debug data structures
			robin_map<string, InstructionSource*> fileNameToSource;
			robin_map<Instruction*, InstructionDebug> instructionDebug;
			bool instructionDebugEnabled = false;

			// data structures for keeping track of instructions that need extra linking
			robin_set<Instruction*> unlinkedFunctions;

			robin_map<string, string*> visitedFiles; // little bit of a hack to keep all filenames persistent

			// tally up the different subtypes for debugging
			uint64_t subtypes[instruction::SUBTYPE_END + 1];

			void swapInstructionDebug(Instruction* source, Instruction* destination);
			void addInstructionDebug(Instruction* source, string symbolicFileName, unsigned short character, unsigned int line);

			void swapInstructionLinking(Instruction* source, Instruction* destination);
	};
}
