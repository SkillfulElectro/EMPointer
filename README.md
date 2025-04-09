# EMPointer

## Goal

`EMPointer` is a C++ header-only smart pointer designed to bring RAII (Resource Acquisition Is Initialization) and reference-counted shared ownership to dynamically allocated memory, while aiming to be as syntactically close to raw pointers (`T*`) as possible. The goal is to provide a safer alternative to manual memory management (`new`/`delete`/`delete[]`) with minimal code changes for basic operations.

## Key Features

*   **RAII:** Automatically manages the lifetime of dynamically allocated objects or arrays. Memory is released when the last `EMPointer` referencing it goes out of scope.
*   **Shared Ownership:** Uses reference counting to allow multiple `EMPointer` instances to safely share ownership of the same resource.
*   **Raw Pointer Syntax:** Overloads common operators (`*`, `->`, `[]`, comparisons, boolean conversion) to mimic raw pointer usage.
*   **Pointer Arithmetic:** Supports pointer arithmetic operators (`++`, `--`, `+=`, `-=`, `+`, `-`), while ensuring correct deallocation by tracking the original allocation address.
*   **Custom Deleters:** Allows providing custom cleanup logic (e.g., for C API resources like `FILE*` or memory from `malloc`) using `std::function`.
*   **`void*` Specialization:** Provides basic support for managing `void*`.
*   **Implicit Conversion:** Offers an implicit conversion to the underlying raw pointer type (`T*`) for easier interoperability with functions expecting raw pointers (use with caution).
*   **Ownership Release:** Includes a `do_not_manage()` method to detach the smart pointer and release ownership, returning the raw pointer for manual management.

## Differences from Raw Pointers & Handling

While `EMPointer` mimics much raw pointer syntax, it's not a *completely* transparent drop-in replacement due to safety features and C++ limitations. Migrating code requires attention to these key differences:

1.  **Array Allocation:**
    *   **Difference:** You cannot initialize an `EMPointer` for an array using `em::pointer<T> ptr = new T[size];`. This syntax incorrectly uses the single-object constructor.
    *   **Handling:** **Must** use the explicit size constructor: `em::pointer<T> ptr = em::pointer<T>(size);`. This ensures `EMPointer` knows to use `delete[]` during cleanup.

2.  **Pointer Arithmetic:**
    *   **Difference:** Arithmetic operators (`++`, `+=`, etc.) modify the *current* pointer (`value`) used by access operators (`*`, `->`, `[]`). However, `EMPointer` stores the `original_value` separately and uses *that* for deallocation (`delete`/`delete[]`/custom deleter).
    *   **Handling:** Arithmetic is safe regarding *deallocation*. However, after performing arithmetic, ensure the *current* pointer (`value`) still points within the valid bounds of the allocated memory before using `*`, `->`, or `[]`.

3.  **Passing to Functions Expecting Raw Pointers:**
    *   **Difference:** `EMPointer<T>` provides an implicit conversion `operator T*()`. This allows passing it directly to functions like `void func(T* ptr);` via `func(my_em_pointer);`.
    *   **Handling:** While convenient, this carries a risk: if the obtained raw pointer outlives the `EMPointer` object, it becomes a dangling pointer. For clarity or safety, consider explicitly using `func(my_em_pointer.get_raw_ptr());`.

4.  **Manual Deletion:**
    *   **Difference:** `EMPointer` handles deletion automatically.
    *   **Handling:** **Must remove** all manual `delete ptr;` and `delete[] ptr;` calls for pointers managed by `EMPointer`. Failure to do so will result in double-free errors and crashes.

5.  **Casting:**
    *   **Difference:** C++ casting keywords (`static_cast`, `reinterpret_cast`, `const_cast`) cannot be overloaded for custom types like `EMPointer`.
    *   **Handling:** Perform casts on the raw pointer obtained via `get_raw_ptr()` or the implicit conversion: `static_cast<N*>(my_em_pointer)` or `static_cast<N*>(my_em_pointer.get_raw_ptr())`.

6.  **Copying Semantics:**
    *   **Difference:** Copying an `EMPointer` (`em::pointer<T> p2 = p1;` or `p2 = p1;`) implements *shared ownership* by incrementing the reference count. Copying a raw pointer (`T* p2 = p1;`) creates a shallow copy (alias), leading to potential double-delete or dangling pointer issues.
    *   **Handling:** Be aware that multiple `EMPointer` objects can validly point to and manage the same resource. The resource is only deleted when the *last* owning `EMPointer` is destroyed.

## Roadmap & Drop-in Replacement Status

**Goal:** To be a near drop-in replacement for `T*` for common dynamic allocation patterns, adding RAII and shared ownership safety.

**Added Features for Compatibility:**

*   RAII for `new`/`new[]` (via size constructor)
*   Reference counting / Shared Ownership
*   Overloaded `*`, `->`, `[]`
*   Overloaded comparison operators (`==`, `!=`, `<`, `>`, `<=`, `>=`) vs `EMPointer` and `nullptr`
*   Boolean context evaluation (`if(ptr)`)
*   Implicit conversion `operator T*()`
*   Pointer Arithmetic Operators (`++`, `--`, `+=`, `-=`, `+`, `-`) (with safe deletion)
*   Custom Deleter Support (`std::function`)
*   `void*` specialization
*   `do_not_manage()` method

**Limitations / Not a Fully Transparent Replacement:**

*   **Array Allocation Syntax:** Requires `em::pointer<T>(size)`, cannot use `new T[]` directly in assignment/initialization.
*   **Casting:** Requires getting the raw pointer first (`get_raw_ptr()` or implicit conversion) before using C++ cast keywords.
*   **Manual Deletion:** Existing `delete`/`delete[]` calls **must** be removed.
*   **Implicit Conversion Risk:** While convenient, implicit conversion to `T*` can lead to dangling pointers if not handled carefully.

`EMPointer` significantly improves safety over raw pointers but requires specific syntax adjustments (especially for arrays) and removal of manual deletion calls during migration.

## Basic Usage

```c++
#include "EMPointer.h"
#include <iostream>

struct Widget {
    int id;
    Widget(int i) : id(i) { std::cout << "Widget " << id << " created.\n"; }
    ~Widget() { std::cout << "Widget " << id << " destroyed.\n"; }
    void show() { std::cout << "Widget ID: " << id << std::endl; }
};

int main() {
    // Single object
    {
        em::pointer<Widget> pW1 = new Widget(1); // Allocation + RAII
        if (pW1) {
            pW1->show(); // Use like a raw pointer

            em::pointer<Widget> pW2 = pW1; // Shared ownership (ref count = 2)
            std::cout << "Widget use count: " << pW1.use_count() << std::endl;

        } // pW2 goes out of scope (ref count = 1)
          // pW1 goes out of scope (ref count = 0), Widget(1) is deleted automatically
    }

    std::cout << "-----\n";

    // Array object
    {
        const size_t count = 3;
        em::pointer<Widget> pWArray = em::pointer<Widget>(count); // MUST use size constructor
        if (pWArray) {
            for(size_t i = 0; i < count; ++i) {
                 pWArray[i] = Widget(10 + i); // Use operator[]
            }
            pWArray[1].show();

            // Pointer arithmetic example (use with care)
            em::pointer<Widget> pWPtr = pWArray;
            pWPtr++; // Move to second element
            pWPtr->show();

        } // pWPtr goes out of scope (detaches)
          // pWArray goes out of scope, delete[] is called automatically for the array
    }

    return 0;
}
```
