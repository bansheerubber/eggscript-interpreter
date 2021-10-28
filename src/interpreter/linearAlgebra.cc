#include "linearAlgebra.h"

#include "entry.h"

ts::Matrix::Matrix() {
	this->data = nullptr;
	this->rows = 0;
	this->columns = 0;
}

ts::Matrix::~Matrix() {
	if(this->data != nullptr) {
		for(unsigned int r = 0; r < this->rows; r++) {
			delete[] this->data[r];
		}
		delete[] this->data;
	}
}

ts::Matrix* ts::initializeMatrix(Matrix* matrix, unsigned int rows, unsigned int columns, bool fillZeros) {
	if(matrix->data != nullptr) {
		for(unsigned int r = 0; r < matrix->rows; r++) {
			delete[] matrix->data[r];
		}
		delete[] matrix->data;
	}
	
	matrix->rows = rows;
	matrix->columns = columns;

	matrix->data = new Entry*[rows];
	for(unsigned int r = 0; r < matrix->rows; r++) {
		matrix->data[r] = new Entry[columns];
		if(fillZeros) {
			for(unsigned int c = 0; c < matrix->columns; c++) {
				matrix->data[r][c].setNumber(0);
			}
		}
	}
	return matrix;
}

ts::Matrix* ts::addMatrix(Matrix* matrix1, Matrix* matrix2) {
	if(matrix1->rows != matrix2->rows || matrix1->columns != matrix2->columns) {
		return nullptr;
	}
	
	Matrix* output = new Matrix();
	initializeMatrix(output, matrix1->rows, matrix1->columns);
	for(unsigned int r = 0; r < matrix1->rows; r++) {
		for(unsigned int c = 0; c < matrix1->columns; c++) {
			output->data[r][c] = matrix1->data[r][c].numberData + matrix2->data[r][c].numberData;
		}
	}
	return output;
}

ts::Matrix* ts::subtractMatrix(Matrix* matrix1, Matrix* matrix2) {
	if(matrix1->rows != matrix2->rows || matrix1->columns != matrix2->columns) {
		return nullptr;
	}
	
	Matrix* output = new Matrix();
	initializeMatrix(output, matrix1->rows, matrix1->columns);
	for(unsigned int r = 0; r < matrix1->rows; r++) {
		for(unsigned int c = 0; c < matrix1->columns; c++) {
			output->data[r][c] = matrix1->data[r][c].numberData - matrix2->data[r][c].numberData;
		}
	}
	return nullptr;
}

ts::Matrix* ts::cloneMatrix(Matrix* matrix) {
	Matrix* output = new Matrix();
	initializeMatrix(output, matrix->rows, matrix->columns);
	for(unsigned int r = 0; r < matrix->rows; r++) {
		for(unsigned int c = 0; c < matrix->columns; c++) {
			copyEntry(matrix->data[r][c], output->data[r][c]);
		}
	}
	return output;
}

ts::Matrix* ts::cloneRowToVector(Matrix* matrix, unsigned int index) { // extract a row out of a matrix and return it as a vector
	if(index >= matrix->rows) {
		return nullptr;
	}

	Matrix* output = new Matrix();
	initializeMatrix(output, 1, matrix->columns);
	for(unsigned int c = 0; c < matrix->columns; c++) {
		copyEntry(matrix->data[index][c], output->data[0][c]);
	}
	return output;
}
