#pragma once

#include <string>

using namespace std;

namespace ts {
	struct Entry;
	class Matrix {
		public:
			Matrix();
			Matrix(unsigned int rows, unsigned int columns, bool fillZeros = false);
			~Matrix();

			Entry** data;
			unsigned int rows;
			unsigned int columns;

			bool equal(const Matrix* other);
			Matrix* initialize(unsigned int rows, unsigned int columns, bool fillZeros = false);
			Matrix* add(const Matrix* other);
			Matrix* subtract(const Matrix* other);
			Matrix* multiply(double scalar);
			double dot(const Matrix* other);
			Matrix* clone();
			Matrix* cloneRowToVector(unsigned int index);
			string print();
	};
}
