# EMPointer
- inspired from Rustlang Rc
- smart pointer which allows you to use it exactly how you used your pointers

## Why EMPointer
- it tracks number of pointers to one value so it keeps the value till the last object goes out of the scope like GC ; so no dangling pointers from now 
- it supports old C APIs via em::custom_ptr
- it does deallocation for you automatically , so you dont need to call delete manually

## Getting started
- include EMPointer.h
- C++ pointer api :
```c++
em::pointer<int> p1(5); // Create a pointer managing an array of 5 integers

// Initialize the array
p1[0] = 1;
p1[1] = 2;
// Copy p1 to p2 using the copy constructor
em::pointer<int> p2(p1);
/*the copy is same as pointing to the same object , the ptr_counter will track both of the p1 and p2*/
```
- C pointer api :
```c
#include "EMPointer.h"

// Custom allocation function
template<typename T>
int custom_allocator(T*& ptr) {
    ptr = new T[10]; // Allocate space for 10 elements
    return ptr ? 0 : -1; // Return 0 if allocation was successful, otherwise -1
}

// Custom deallocation function
template<typename T>
int custom_deallocator(T*& ptr) {
    delete[] ptr; // Deallocate the array
    ptr = nullptr; // Set pointer to nullptr
    return 0; // Return 0 to indicate success
}

int main() {
    // Define custom allocator and deallocator
    em::custom_ptr<int> custom{
        custom_allocator<int>,
        custom_deallocator<int>
    };

    // Create a pointer with custom allocation and deallocation
    em::pointer<int> p2(custom);
    
    // Use the pointer (e.g., initialize the array)
    p2[0] = 7;
    p2[1] = 14;

    std::cout << "p2[0]: " << p2[0] << std::endl;
    std::cout << "p2[1]: " << p2[1] << std::endl;

    return 0;
}
```

make sure to read comments on the source code for better understanding
