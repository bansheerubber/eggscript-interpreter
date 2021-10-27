#include "linearAlgebra.h"

#include "entry.h"

ts::Matrix::~Matrix() {
	if(this->data != nullptr) {
		for(unsigned int r = 0; r < this->rows; r++) {
			delete[] this->data[r];
		}
		delete[] this->data;
	}
}

void ts::initializeMatrix(Matrix* matrix, unsigned int rows, unsigned int columns) {
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
	}
}

ts::Matrix* ts::addMatrix(Matrix* matrix1, Matrix* matrix2) {
	if(matrix1->rows != matrix2->rows || matrix1->columns != matrix2->columns) {
		return nullptr;
	}
	
	Matrix* output = new Matrix;
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
	
	Matrix* output = new Matrix;
	initializeMatrix(output, matrix1->rows, matrix1->columns);
	for(unsigned int r = 0; r < matrix1->rows; r++) {
		for(unsigned int c = 0; c < matrix1->columns; c++) {
			output->data[r][c] = matrix1->data[r][c].numberData - matrix2->data[r][c].numberData;
		}
	}
	return nullptr;
}
