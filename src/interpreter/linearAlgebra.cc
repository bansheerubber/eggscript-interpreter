#include "linearAlgebra.h"

#include "../util/cloneString.h"
#include "entry.h"
#include "../util/numberToString.h"

ts::Matrix::Matrix() {
	this->data = nullptr;
	this->rows = 0;
	this->columns = 0;
}

ts::Matrix::Matrix(unsigned int rows, unsigned int columns, bool fillZeros) {
	this->data = nullptr;
	this->rows = 0;
	this->columns = 0;
	this->initialize(rows, columns, fillZeros);
}

ts::Matrix::~Matrix() {
	if(this->data != nullptr) {
		for(unsigned int r = 0; r < this->rows; r++) {
			delete[] this->data[r];
		}
		delete[] this->data;
	}
}

ts::Matrix* ts::Matrix::initialize(unsigned int rows, unsigned int columns, bool fillZeros) {
	if(this->data != nullptr) {
		for(unsigned int r = 0; r < this->rows; r++) {
			delete[] this->data[r];
		}
		delete[] this->data;
	}
	
	this->rows = rows;
	this->columns = columns;

	this->data = new Entry*[rows];
	for(unsigned int r = 0; r < this->rows; r++) {
		this->data[r] = new Entry[columns];
		if(fillZeros) {
			for(unsigned int c = 0; c < this->columns; c++) {
				this->data[r][c].setNumber(0);
			}
		}
	}
	return this;
}

ts::Matrix* ts::Matrix::add(const Matrix* other) {
	if(this->rows != other->rows || this->columns != other->columns) {
		return nullptr;
	}

	Matrix* output = new Matrix(this->rows, this->columns);
	for(unsigned int r = 0; r < this->rows; r++) {
		for(unsigned int c = 0; c < this->columns; c++) {
			output->data[r][c] = this->data[r][c].numberData + other->data[r][c].numberData;
		}
	}
	return output;
}

ts::Matrix* ts::Matrix::subtract(const Matrix* other) {
	if(this->rows != other->rows || this->columns != other->columns) {
		return nullptr;
	}
	
	Matrix* output = new Matrix(this->rows, this->columns);
	for(unsigned int r = 0; r < this->rows; r++) {
		for(unsigned int c = 0; c < this->columns; c++) {
			output->data[r][c] = this->data[r][c].numberData - other->data[r][c].numberData;
		}
	}
	return nullptr;
}

ts::Matrix* ts::Matrix::clone() {
	Matrix* output = new Matrix(this->rows, this->columns);
	for(unsigned int r = 0; r < this->rows; r++) {
		for(unsigned int c = 0; c < this->columns; c++) {
			copyEntry(this->data[r][c], output->data[r][c]);
		}
	}
	return output;
}

ts::Matrix* ts::Matrix::cloneRowToVector(unsigned int index) { // extract a row out of a matrix and return it as a vector
	if(index >= this->rows) {
		return nullptr;
	}

	Matrix* output = new Matrix(1, this->columns);
	for(unsigned int c = 0; c < this->columns; c++) {
		copyEntry(this->data[index][c], output->data[0][c]);
	}
	return output;
}

string ts::Matrix::print() {
	string output = "{ ";
	for(unsigned int r = 0; r < this->rows; r++) {
		for(unsigned int c = 0; c < this->columns; c++) {
			const char* number;
			bool deleteString = false;
			## type_conversion.py "this->data[r][c]" number ALL STRING deleteString
			output += number;
			output += ", ";
			if(deleteString) {
				delete[] number;
			}
		}

		if(r != this->rows - 1) {
			output += "\n  ";
		}
	}
	output += "}";
	return output;
}
