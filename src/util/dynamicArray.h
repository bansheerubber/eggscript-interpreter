#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <utility>

#define DYNAMIC_ARRAY_MAX_SIZE 5000000

template <typename T, typename S = void, bool disableReallocateOnPush = false>
class DynamicArray {
	public:
		uint64_t head;
		uint64_t size;

		DynamicArray() {
			this->array = nullptr;
			this->dontDelete = true;
		}

		DynamicArray(uint64_t size) {
			this->parent = nullptr;
			this->init = nullptr;
			this->onRealloc = nullptr;

			this->head = 0;
			this->size = size;

			this->constructArray();
		}

		DynamicArray(
			S* parent,
			uint64_t size,
			void (*init) (S* parent, T* location),
			void (*onRealloc) (S* parent)
		) {
			this->parent = parent;
			this->init = init;
			this->onRealloc = onRealloc;

			this->head = 0;
			this->size = size;

			this->constructArray();
		}

		~DynamicArray() {
			if(this->dontDelete) {
				this->dontDelete = false;
				return;
			}
			
			if(this->array != nullptr) {
				free(this->array);
				this->array = nullptr;
			}
		}

		void pushed() {
			this->head++;

			if constexpr(!disableReallocateOnPush) {
				if(this->head == this->size) {
					this->allocate(this->size * 2);
				}
			}
		}

		void popped() {
			this->head--;
		}

		void allocate(uint64_t amount) {
			if(amount * 2 > DYNAMIC_ARRAY_MAX_SIZE) {
				printf("stack overflow\n");
				exit(1);
			}
			
			T* array = (T*)realloc(this->array, sizeof(T) * amount);
			if(array == NULL) {
				printf("invalid dynamic array realloc\n");
				exit(1);
			}
			this->array = array;

			if(this->init != nullptr) {
				for(uint64_t i = this->size; i < amount; i++) {
					(*this->init)(this->parent, &this->array[i]);
				}
			}
			this->size = amount;
			
			if(this->onRealloc != nullptr) {
				(*this->onRealloc)(this->parent);
			}
		}

		void remove(T entry) {
			int64_t index = this->index(entry);
			if(index != -1) {
				this->shift(index + 1, -1);
			}
		}

		int64_t index(T entry) {
			for(uint64_t i = 0; i < this->head; i++) {
				if(entry == this->array[i]) {
					return i;
				}
			}
			return -1;
		}

		void shift(int64_t index, int64_t amount) {
			int64_t end = (int64_t)this->head;

			for(int i = 0; i < amount; i++) {
				this->pushed(); // allocate space
			}

			// start from the end for shifting right
			if(amount >= 0) {
				for(int i = end - 1; i >= index; i--) {
					this->array[i + amount] = std::move(this->array[i]); // move
				}
			}
			else {
				for(int i = index; i < end; i++) {
					this->array[i + amount] = std::move(this->array[i]); // move
				}
			}

			for(int i = index; i < index + amount; i++) {
				// re-initialize entries
				if(this->init != nullptr) {
					(*this->init)(this->parent, &this->array[i]);
				}
			}

			if(amount < 0) { // pop for shift lefts
				for(int i = end - 1; i >= end + amount; i--) {
					if(this->init != nullptr) {
						(*this->init)(this->parent, &this->array[i]);
					}
					this->popped();
				}
			}
		}

		T& operator[](size_t index) {
			return this->array[index];
		}
	
	private:
		void constructArray() {
			T* array = (T*)malloc(sizeof(T) * this->size);
			if(array == NULL) {
				printf("invalid dynamic array malloc\n");
				exit(1);
			}
			this->array = array;

			if(this->init != nullptr) {
				for(uint64_t i = 0; i < this->size; i++) {
					(*this->init)(this->parent, &(this->array[i]));
				}
			}
		}

		bool dontDelete = false;
		T* array;
		S* parent;
		void (*init) (S* parent, T* location);
		void (*onRealloc) (S* parent);
};
