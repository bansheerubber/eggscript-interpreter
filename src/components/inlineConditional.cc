#include "inlineConditional.h"

bool InlineConditional::ShouldParse(Tokenizer* tokenizer, Parser* parser) {
	return tokenizer->peekToken().type == QUESTION_MARK;
}

InlineConditional* InlineConditional::Parse(Component* leftHandSide, Component* parent, Tokenizer* tokenizer, Parser* parser) {
	InlineConditional* output = new InlineConditional(parser);
	output->parent = parent;
	output->leftHandSide = leftHandSide;
	leftHandSide->setParent(output->leftHandSide);

	parser->expectToken(QUESTION_MARK);

	// parse a component
	if(!Component::ShouldParse(output, tokenizer, parser)) {
		parser->error("expected evaluateable expression, string literal, number literal, or boolean literal for inline conditional true condition");
	}
	output->ifTrue = Component::Parse(output, tokenizer, parser);

	parser->expectToken(COLON);

	// parse a component
	if(!Component::ShouldParse(output, tokenizer, parser)) {
		parser->error("expected evaluateable expression, string literal, number literal, or boolean literal for inline conditional false condition");
	}
	output->ifFalse = Component::Parse(output, tokenizer, parser);

	return output;
}

string InlineConditional::print() {
	return this->leftHandSide->print() + this->parser->space + "?" + this->parser->space + this->ifTrue->print() + " : " + this->ifFalse->print();
}

ts::InstructionReturn InlineConditional::compile() {
	return {};
}