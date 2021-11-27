#pragma once

#include <new>
#include <type_traits>

#include "dynamicArray.h"

template <typename T, typename S = void>
class GarbageCollectionHeap {
	public:
		GarbageCollectionHeap() {
			new((void*)&this->array) DynamicArray<T, S>(4);
		}
		
		GarbageCollectionHeap(
			S* parent,
			void (*init) (S* parent, T* location),
			void (*onRealloc) (S* parent)
		) {
			new((void*)&this->array) DynamicArray<T, S>(parent, 4, init, onRealloc);
		}

		void insert(T value) {
			// insert at the end
			size_t iterator = this->array.head;
			this->array[iterator] = value;
			value->heapIndex = iterator; // update value heap index
			this->array.pushed();

			while(iterator != 0 && this->compare(iterator, this->parentIndex(iterator))) {
				this->swap(iterator, this->parentIndex(iterator));
				iterator = this->parentIndex(iterator);
			}
		}

		// call when the value has been incremented
		void updateUp(size_t index) {
			this->organize(index);
		}

		// call when the value has been decremented
		void updateDown(size_t index) {
			while(index != 0 && this->compare(index, this->parentIndex(index))) {
				this->swap(index, this->parentIndex(index));
				index = this->parentIndex(index);
			}
		}

		T& top() {
			return this->array[0];
		}

		void pop() {
			if(this->array.head == 1) {
				this->array.popped();
			}
			else {
				this->array[0] = this->array[this->array.head - 1];
				this->array[0]->heapIndex = 0;
				this->array.popped();

				this->organize(0);
			}
		}
	
		DynamicArray<T, S> array;
	
	private:
		void (*init) (S* parent, T* location);
		void (*onRealloc) (S* parent);

		size_t parentIndex(size_t index) {
			return (index - 1) / 2;
		}

		size_t leftChildIndex(size_t index) {
			return (2 * index + 1);
		}

		size_t rightChildIndex(size_t index) {
			return (2 * index + 2);
		}

		void swap(size_t index1, size_t index2) {
			// update value heap index
			this->array[index1]->heapIndex = index2;
			this->array[index2]->heapIndex = index1;
			
			T temp = this->array[index1];
			this->array[index1] = this->array[index2];
			this->array[index2] = temp;
		}

		void organize(size_t index) {
			size_t leftIndex = this->leftChildIndex(index);
			size_t rightIndex = this->rightChildIndex(index);
			size_t smallestIndex = index;

			if(leftIndex < this->array.head && this->compare(leftIndex, smallestIndex)) {
				smallestIndex = leftIndex;
			}

			if(rightIndex < this->array.head && this->compare(rightIndex, smallestIndex)) {
				smallestIndex = rightIndex;
			}

			if(smallestIndex != index) {
				this->swap(index, smallestIndex);
				this->organize(smallestIndex);
			}
		}

		bool compare(size_t leftIndex, size_t rightIndex) {
			return *(this->array[leftIndex]) < *(this->array[rightIndex]);
		}
};
