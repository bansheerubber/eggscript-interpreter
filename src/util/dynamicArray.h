#pragma once

#include <cstddef>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>

#define DYNAMIC_ARRAY_MAX_SIZE 5000000

template <typename T, typename S = void>
class DynamicArray {
	public:
		size_t head;
		size_t size;

		DynamicArray() {
			this->array = nullptr;
			this->dontDelete = true;
		}

		DynamicArray(size_t size) {
			this->parent = nullptr;
			this->init = nullptr;
			this->onRealloc = nullptr;

			this->head = 0;
			this->size = size;

			this->constructArray();
		}

		DynamicArray(
			S* parent,
			size_t size,
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

			if(this->head == this->size) {
				this->allocate(this->size * 2);
			}
		}

		void popped() {
			this->head--;
		}

		void allocate(size_t amount) {
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
				for(size_t i = this->size; i < amount; i++) {
					(*this->init)(this->parent, &this->array[i]);
				}
			}
			this->size = amount;
			
			if(this->onRealloc != nullptr) {
				(*this->onRealloc)(this->parent);
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
				for(size_t i = 0; i < this->size; i++) {
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
