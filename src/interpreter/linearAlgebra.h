#pragma once

namespace ts {
	struct Entry;
	struct Matrix {
		Entry** data;
		unsigned int rows;
		unsigned int columns;

		Matrix();
		~Matrix();
	};

	Matrix* initializeMatrix(Matrix* matrix, unsigned int rows, unsigned int columns, bool fillZeros = false);
	void deleteMatrix(Matrix* matrix);
	Matrix* addMatrix(Matrix* matrix1, Matrix* matrix2);
	Matrix* subtractMatrix(Matrix* matrix1, Matrix* matrix2);
	Matrix* cloneMatrix(Matrix* matrix);
	Matrix* cloneRowToVector(Matrix* matrix, unsigned int index);
}
