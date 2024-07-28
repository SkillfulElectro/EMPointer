#ifndef EM_POINTER
#define EM_POINTER

/*
MIT License

Copyright (c) 2024 ElectroMutex

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <iostream>

namespace em {
  template<typename T>
    struct custom_ptr {
      // Type aliases for the allocator and deallocator function pointers
      using Allocator = int (*)(T*&);   // Function pointer type for allocator
      using Deallocator = int (*)(T*&); // Function pointer type for deallocator

      // Members to store the function pointers
      Allocator allocator;
      Deallocator deallocator;
    };

  template<typename T>
    class pointer{
      private:
        T* value;
        int* ptr_counter;
        bool custom_style;
        bool allocated;
        em::custom_ptr<T> custom;

      public:

        pointer(size_t size){
          ptr_counter = new int;
          *ptr_counter = 1;

          value = nullptr;
          value = new T[size];
          custom_style = false;

          if (value){
            allocated = true;
          }else{
            delete ptr_counter;
            ptr_counter = nullptr;
            allocated = false;
          }
        }

        pointer(custom_ptr<T> custom){
          ptr_counter = new int;
          *ptr_counter = 1;

          this->custom = custom;
          custom_style = true;
          int result = custom.allocator(value);

          if (result != 0){
            delete ptr_counter;
            ptr_counter = nullptr;
            value = nullptr;
            allocated = false;
            std::cerr << "Error: custom_ptr allocator -> " << result << '\n';
          }else {
            allocated = true;
          }
        }

        pointer(const pointer& other){
          this->ptr_counter = other.ptr_counter;
          *ptr_counter += 1;
          value = other.value;
          custom_style = other.custom_style;
          allocated = other.allocated;
          custom = other.custom;
        }

        int delete_ptr(){
          if (ptr_counter){
            if (*ptr_counter != 1){
              *ptr_counter -= 1;
              ptr_counter = nullptr;
              value = nullptr;
            }else{
              if (!custom_style){
                if (value){
                  delete ptr_counter;
                  delete[] value;
                  ptr_counter = nullptr;
                  value = nullptr;
                }else{
                  delete ptr_counter;
                  ptr_counter = nullptr;
                  value = nullptr;
                }
              }else{
                if (value){
                  int result = custom.deallocator(value);
                  if (result == 0){
                    delete ptr_counter;
                    ptr_counter = nullptr;
                    value = nullptr;

                    return result;
                  }

                  return result;
                }else{
                  delete ptr_counter;
                  ptr_counter = nullptr;
                  value = nullptr;
                }
              }
            }


          }

          return 0;
        }

        ~pointer(){
          int result = delete_ptr();
          if (result != 0){
            std::cerr << "Error: custom_ptr deallocator -> " << result << '\n';
            return;
          }
        }

        pointer& operator=(pointer& other){
          this->delete_ptr();

          this->value = other.value;
          this->custom = other.custom;
          this->allocated = other.allocated;
          this->custom_style = other.custom_style;
          this->ptr_counter = other.ptr_counter;
          *ptr_counter += 1;

          return *this;
        }

        T& operator*() {
          return *value;
        }

        T& operator[](size_t index) {
          return value[index];
        }

        T* operator->() {
          return value;
        }

        operator bool() const {
          return value;
        }

        pointer& operator++() {
          ++value;
          return *this;
        }

        T* operator++(int) {
          T* addr = value;
          ++(*this);
          return addr;
        }

        pointer& operator--() {
          --value;
          return *this;
        }

        T* operator--(int) {
          T* addr = value;
          --(*this);
          return addr;
        }

        pointer& operator+=(const double& other) {
          value += int(other);
          return *this;
        }


        pointer& operator-=(const double& other) {
          value -= int(other);
          return *this;
        }
        
        pointer& operator-(const double& other){
          value -= other;
          return *this;
        }

        pointer& operator+(const double& other){
          value += other;
          return *this;
        }
    };
}
#endif // !EM_POINTER
