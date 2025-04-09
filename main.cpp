#define _CRT_SECURE_NO_WARNINGS


#include "EMPointer.h" 
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstdlib>
#include <cstdio>
#include <functional>
#include <memory>     
#include <utility>   

// --- Helper Struct ---
struct MyData {
    int id;
    std::string name;
    static int instance_count;

    MyData(int i = 0, std::string n = "Default") : id(i), name(std::move(n)) {
        ++instance_count;
        std::cout << "    MyData[" << instance_count << "] Constructed (ID: " << id << ") at " << std::addressof(*this) << std::endl;
    }

    MyData(const MyData& other) : id(other.id), name(other.name) {
        ++instance_count;
        std::cout << "    MyData[" << instance_count << "] Copied (ID: " << id << ") to " << std::addressof(*this) << " from " << std::addressof(other) << std::endl;
    }

    MyData(MyData&& other) noexcept : id(other.id), name(std::move(other.name)) {
        std::cout << "    MyData Moved (ID: " << id << ") to " << std::addressof(*this) << " from " << std::addressof(other) << std::endl;
        other.id = -1;
        other.name = "MOVED";
    }

    MyData& operator=(const MyData& other) {
        std::cout << "    MyData Copy Assigned (ID: " << other.id << ") to " << this->id << " at " << std::addressof(*this) << std::endl;
        if (this != &other) {
            id = other.id;
            name = other.name;
        }
        return *this;
    }

    MyData& operator=(MyData&& other) noexcept {
        std::cout << "    MyData Move Assigned (ID: " << other.id << ") to " << this->id << " at " << std::addressof(*this) << std::endl;
        if (this != &other) {
            id = other.id;
            name = std::move(other.name);
            other.id = -1;
            other.name = "MOVED";
        }
        return *this;
    }

    ~MyData() {
        int current_count = (id == -1 && name == "MOVED") ? instance_count : instance_count--;
        std::cout << "    MyData[" << current_count << "] Destructed (ID: " << id << ") at " << std::addressof(*this) << std::endl;
    }

    void print(const char* prefix = "") const {
        std::cout << "    " << prefix << "Data - ID: " << id << ", Name: " << name << " at " << std::addressof(*this) << std::endl;
    }
};
int MyData::instance_count = 0;

// --- Helper Functions ---
void process_raw_ptr(MyData* ptr, const char* msg) {
    std::cout << "  -> process_raw_ptr (" << msg << "): ";
    if (ptr) {
        ptr->print("Raw ");
    }
    else {
        std::cout << "Received nullptr." << std::endl;
    }
}

void risky_operation(bool do_throw) {
    if (do_throw) {
        std::cout << "    risky_operation: Throwing exception!" << std::endl;
        throw std::runtime_error("Risky op failed");
    }
    std::cout << "    risky_operation: Succeeded." << std::endl;
}

// --- Custom Deleters ---
void free_deleter(int* ptr) {
    std::cout << "    Custom deleter (free) called for " << ptr << std::endl;
    free(ptr);
}
void free_deleter_void(void* ptr) {
    std::cout << "    Custom deleter (free_void) called for " << ptr << std::endl;
    free(ptr);
}
void file_closer(FILE* ptr) {
    std::cout << "    Custom deleter (fclose) called." << std::endl;
    if (ptr) fclose(ptr);
}
void file_closer_void(void* ptr) {
    std::cout << "    Custom deleter (fclose_void) called." << std::endl;
    if (ptr) fclose(static_cast<FILE*>(ptr));
}

// --- Main Comparison ---
int main() {
    std::cout << "===== EMPointer vs Raw Pointer Comparison =====\n" << std::endl;

    // --- 1. Single Object Allocation ---
    std::cout << "[1. Single Object Allocation]\n";
    int* raw_int1 = new int(10);
    MyData* raw_data1_orig = new MyData(1, "Raw One Original"); // *** Renamed to track leak ***
    std::cout << "  Raw Ptr: Allocated int: " << *raw_int1 << std::endl;

    em::pointer<int> em_int1 = new int(11); // Note: Implicit conversion from T* works.
    em::pointer<MyData> em_data1 = new MyData(11, "EM One");
    std::cout << "  EMPointer: Allocated int: " << *em_int1 << " count=" << em_int1.use_count() << std::endl;
    std::cout << "  Note: EMPointer handles deallocation automatically via RAII.\n";
    std::cout << "---------------------------------------------\n";

    // --- 2. Array Allocation ---
    std::cout << "[2. Array Allocation]\n";
    const size_t SIZE = 3;
    MyData* raw_array = new MyData[SIZE];

    // em::pointer<MyData> em_array = new MyData[SIZE]; // ERROR: Cannot implicitly convert T* from new[]
    em::pointer<MyData> em_array = em::pointer<MyData>(SIZE); // Note: EMPointer requires explicit size constructor for arrays.
    std::cout << "  EMPointer: Array allocated, is_array=" << em_array.is_array() << std::endl;
    std::cout << "  Note: Using size constructor ensures delete[] is used by EMPointer.\n";
    std::cout << "---------------------------------------------\n";

    // --- 3. Initialization and Assignment ---
    std::cout << "[3. Initialization and Assignment]\n";
    int* raw_int_orig_sec3 = new int(101);
    int* raw_int2_sec3 = raw_int_orig_sec3;
    delete raw_int_orig_sec3; // Deleting original invalidates aliases (dangling pointer).
    raw_int_orig_sec3 = nullptr;
    std::cout << "  Raw Ptr: raw_int2_sec3 is now dangling after deleting original." << std::endl;

    raw_int_orig_sec3 = new int(303);
    int* raw_int3_sec3 = std::move(raw_int_orig_sec3); // Move for raw pointers is just a copy.

    em::pointer<int> em_int_a = new int(111);
    em::pointer<int> em_int_b = em_int_a; // Note: EMPointer copy constructor shares ownership.
    std::cout << "  EMPointer: Copied pointer shares ownership: em_int_a count=" << em_int_a.use_count() << ", em_int_b count=" << em_int_b.use_count() << std::endl;

    em_int_a = new int(333); // Note: EMPointer assignment uses RAII (releases old, takes new).
    std::cout << "  EMPointer: After em_int_a = new int(333): em_int_a=" << *em_int_a << " count=" << em_int_a.use_count() << std::endl;
    std::cout << "  EMPointer: em_int_b still manages original object (111): em_int_b=" << *em_int_b << " count=" << em_int_b.use_count() << std::endl;

    em::pointer<int> em_int_c = std::move(em_int_a); // Note: EMPointer move transfers ownership.
    std::cout << "  EMPointer: Moved pointer em_int_c=" << *em_int_c << " count=" << em_int_c.use_count() << std::endl;
    std::cout << "  EMPointer: em_int_a after move is null=" << em_int_a.is_null() << std::endl;

    em_int_b = nullptr; // Note: Assigning nullptr releases ownership (and deletes object if count reaches 0).
    std::cout << "  EMPointer: Assigned nullptr to em_int_b. is_null=" << em_int_b.is_null() << std::endl;
    std::cout << "---------------------------------------------\n";

    // --- 4. Dereferencing, Member/Array Access ---
    std::cout << "[4. Dereferencing, Member/Array Access]\n";
    MyData* raw_data1_sec4 = new MyData(4, "Raw Four"); // Use separate var name
    if (SIZE > 1) raw_array[1] = MyData(44, "Raw Array One");
    std::cout << "  Raw Ptr: *raw_data1_sec4 value=" << raw_data1_sec4->id << std::endl;
    raw_data1_sec4->print("Raw -> ");
    if (SIZE > 1) std::cout << "  Raw Ptr: raw_array[1].id = " << raw_array[1].id << std::endl;

    em_data1 = new MyData(40, "EM Forty"); // Reassigns em_data1, RAII cleans up "EM One"
    if (SIZE > 1 && em_array) em_array[1] = MyData(440, "EM Array One");
    if (em_data1) {
        std::cout << "  EMPointer: *em_data1 value=" << (*em_data1).id << std::endl;
        em_data1->print("EM -> ");
    }
    if (SIZE > 1 && em_array) std::cout << "  EMPointer: em_array[1].id = " << em_array[1].id << std::endl;
    std::cout << "  Note: EMPointer syntax is identical for *, ->, [].\n";
    std::cout << "---------------------------------------------\n";

    // --- 5. Pointer Arithmetic ---
    std::cout << "[5. Pointer Arithmetic]\n";
    int* raw_pa_array = new int[5] {10, 20, 30, 40, 50};
    int* raw_pa_ptr = raw_pa_array;
    std::cout << "  Raw Ptr: Start value=" << *raw_pa_ptr << std::endl;
    raw_pa_ptr++;
    std::cout << "  Raw Ptr: After ++ value=" << *raw_pa_ptr << std::endl;
    raw_pa_ptr += 2;
    std::cout << "  Raw Ptr: After += 2 value=" << *raw_pa_ptr << std::endl;
    int* raw_pa_ptr2 = raw_pa_array + 4;
    std::ptrdiff_t diff = raw_pa_ptr2 - raw_pa_ptr;
    std::cout << "  Raw Ptr: Ptr2 value=" << *raw_pa_ptr2 << ", Diff=" << diff << std::endl;
    delete[] raw_pa_array;

    em::pointer<int> em_pa_array = em::pointer<int>(5);
    if (em_pa_array) {
        for (int i = 0; i < 5; ++i) em_pa_array[i] = (i + 1) * 10;
        em::pointer<int> em_pa_ptr = em_pa_array;
        std::cout << "  EMPointer: Start value=" << *em_pa_ptr << std::endl;
        em_pa_ptr++; // Note: Modifies current 'value' used by *, ->, []
        std::cout << "  EMPointer: After ++ value=" << *em_pa_ptr << " (Current ptr: " << em_pa_ptr.get_raw_ptr() << ")" << std::endl;
        em_pa_ptr += 2;
        std::cout << "  EMPointer: After += 2 value=" << *em_pa_ptr << " (Current ptr: " << em_pa_ptr.get_raw_ptr() << ")" << std::endl;
        em::pointer<int> em_pa_ptr2 = em_pa_array + 4; // Note: operator+ returns a non-owning pointer.
        std::ptrdiff_t em_diff = em_pa_ptr2 - em_pa_ptr; // Note: operator- calculates diff between current 'value' pointers.
        std::cout << "  EMPointer: Ptr2 value=" << *em_pa_ptr2 << ", Diff=" << em_diff << std::endl;
        std::cout << "  Note: Arithmetic modifies current pointer ('value'). Deletion uses stored original pointer ('original_value') -> RAII safe for deletion. Access operators use current 'value'.\n";
    }
    else {
        std::cout << "  EMPointer: Array allocation failed for arithmetic test.\n";
    }
    // Note: Deletion of em_pa_array happens automatically via RAII using original_value.
    std::cout << "---------------------------------------------\n";

    // --- 6. Boolean Context and Comparisons ---
    std::cout << "[6. Boolean Context and Comparisons]\n";
    int* raw_bool1 = new int(6);
    int* raw_bool2 = nullptr;
    if (raw_bool1) std::cout << "  Raw Ptr: raw_bool1 is true" << std::endl;
    if (!raw_bool2) std::cout << "  Raw Ptr: !raw_bool2 is true" << std::endl;

    em::pointer<int> em_bool1 = new int(6);
    em::pointer<int> em_bool2;
    if (em_bool1) std::cout << "  EMPointer: em_bool1 is true" << std::endl; // Note: Uses operator T*() or explicit operator bool()
    if (!em_bool2) std::cout << "  EMPointer: !em_bool2 is true" << std::endl;
    std::cout << "  Note: Comparison and boolean context work similarly via operator overloads.\n";
    delete raw_bool1;
    std::cout << "---------------------------------------------\n";

    // --- 7. Passing to Functions ---
    std::cout << "[7. Passing to Functions]\n";
    MyData* raw_func = new MyData(7, "Raw Func");
    process_raw_ptr(raw_func, "Direct Raw");

    em::pointer<MyData> em_func = new MyData(77, "EM Func");
    if (em_func) {
        process_raw_ptr(em_func, "Implicit Conversion"); // Note: Works due to operator T*()
    }
    std::cout << "  Note: Implicit conversion operator T*() allows passing directly where T* is expected. RISK: Dangling pointer if raw ptr outlives em::pointer.\n";
    delete raw_func;
    std::cout << "---------------------------------------------\n";

    // --- 8. Casting (including void*) ---
    std::cout << "[8. Casting (including void*)]\n";
    void* raw_vptr = new int(8);
    int* raw_iptr_cast = static_cast<int*>(raw_vptr);
    std::cout << "  Raw Ptr: Casted void* to int*: value=" << *raw_iptr_cast << std::endl;

    // Note: Manage int via pointer<void> with appropriate deleter
    em::pointer<void> em_vptr(new int(88), [](void* vp) { std::cout << "    void* deleter for int* called." << std::endl; delete static_cast<int*>(vp); });
    if (em_vptr) {
        // Note: Must cast using raw pointer obtained via get_raw_ptr or implicit conversion.
        int* em_iptr_cast = static_cast<int*>(em_vptr.get_raw_ptr()); // Cast the raw void*
        std::cout << "  EMPointer: Casted void* to int*: value=" << *em_iptr_cast << std::endl;
    }
    else {
        std::cout << "  EMPointer: Failed to create pointer<void>." << std::endl;
    }
    std::cout << "  Note: pointer<void> requires custom deleter for meaningful RAII. Casting requires using get_raw_ptr() or implicit conversion first.\n";
    delete static_cast<int*>(raw_vptr); // Manual cleanup for raw void*
    std::cout << "---------------------------------------------\n";

    // --- 9. Exception Safety ---
    std::cout << "[9. Exception Safety]\n";
    MyData* raw_except = nullptr;
    try {
        raw_except = new MyData(9, "Raw Except");
        risky_operation(true); // Throws
        delete raw_except; // <-- Skipped
        raw_except = nullptr; // <-- Skipped
    }
    catch (const std::runtime_error& e) {
        std::cout << "  Raw Ptr: Caught exception: " << e.what() << std::endl;
        if (raw_except) std::cout << "  Raw Ptr: Resource was LEAKED (needs delete in catch)!" << std::endl;
        // *** Fix: Delete if not null in catch block (simplest fix for demo) ***
        // delete raw_except;
        // raw_except = nullptr;
    }
    // Cleanup outside is dangerous due to double-delete possibility if not leaked.
    // For this test run where it leaks in catch, we NEED to delete it somewhere.
    // Let's assume the catch block is the place for demo purposes if leaked.
    // delete raw_except; // Remove this potentially problematic line


    try {
        em::pointer<MyData> em_except = new MyData(99, "EM Except");
        if (em_except) risky_operation(true); // Throws
        // No delete needed here
    }
    catch (const std::runtime_error& e) {
        std::cout << "  EMPointer: Caught exception: " << e.what() << std::endl;
        // Note: Resource automatically cleaned up by RAII during stack unwinding.
        std::cout << "  EMPointer: Resource automatically cleaned up by RAII!" << std::endl;
    }
    std::cout << "---------------------------------------------\n";

    // --- 10. Custom Deleters ---
    std::cout << "[10. Custom Deleters]\n";
    int* raw_malloc = static_cast<int*>(malloc(sizeof(int)));
    FILE* raw_file = fopen("raw_comp.txt", "w");
    if (raw_malloc) *raw_malloc = 100;
    if (raw_file) fprintf(raw_file, "Raw\n");
    if (raw_file) fclose(raw_file); // Manual cleanup
    free(raw_malloc); // Manual cleanup

    { // Scope for RAII with custom deleters
        em::pointer<int> em_malloc(static_cast<int*>(malloc(sizeof(int))), free_deleter);
        em::pointer<FILE> em_file(fopen("em_comp.txt", "w"), file_closer);
        if (em_malloc) *em_malloc = 101; else std::cout << "    EM Malloc failed\n";
        if (em_file) fprintf(em_file.get_raw_ptr(), "EM\n"); else std::cout << "    EM Fopen failed\n";

        em::pointer<void> em_malloc_v(malloc(sizeof(double)), free_deleter_void);
        em::pointer<void> em_file_v(fopen("em_comp_v.txt", "w"), file_closer_void);
        if (em_malloc_v) *(static_cast<double*>(em_malloc_v.get_raw_ptr())) = 1.23; else std::cout << "    EM Malloc Void failed\n";
        if (em_file_v) fprintf(static_cast<FILE*>(em_file_v.get_raw_ptr()), "EM Void\n"); else std::cout << "    EM Fopen Void failed\n";

        std::cout << "  EMPointer: Managing C resources." << std::endl;
        std::cout << "  Note: EMPointer cleanup via custom deleters happens automatically at scope end..." << std::endl;
    } // Deleters called automatically here
    std::cout << "---------------------------------------------\n";

    // --- 11. Releasing Management (`do_not_manage`) ---
    std::cout << "[11. Releasing Management]\n";
    int* raw_release = new int(110);

    em::pointer<int> em_release = new int(111);
    if (em_release) {
        std::cout << "  EMPointer: Before release count=" << em_release.use_count() << std::endl;
        int* released_raw_ptr = em_release.do_not_manage(); // Note: Releases ownership from EMPointer.
        std::cout << "  EMPointer: After release ptr is null=" << em_release.is_null() << " count=" << em_release.use_count() << std::endl;
        std::cout << "  EMPointer: Released raw ptr = " << released_raw_ptr << std::endl;
        std::cout << "  Note: Caller now responsible for manual deletion of pointer returned by do_not_manage().\n";
        delete released_raw_ptr; // Manual cleanup REQUIRED
    }
    else {
        std::cout << "  EMPointer: Allocation failed for release test.\n";
    }
    delete raw_release;
    std::cout << "---------------------------------------------\n";

    // --- Final Cleanup ---
    std::cout << "[Final Cleanup Phase]\n";
    std::cout << "  Cleaning up remaining raw pointers..." << std::endl;
    delete raw_data1_orig; // *** Added delete for the leaked pointer from section 1 ***
    delete raw_data1_sec4;
    delete[] raw_array;
    delete raw_int3_sec3; // Cleanup raw int from section 3
    // Note: raw_except was potentially leaked in this test code's logic if exception occurred.
    // The leak is part of the demonstration of raw pointer issues. We won't try to delete it here.

    std::cout << "  EMPointer objects go out of scope now, triggering RAII cleanup...\n";

    std::cout << "===== Comparison Finished =====\n" << std::endl;
    // Note: The final instance_count reported here might be inaccurate depending on execution environment
    // and timing relative to static object destruction, but ideally should be 0 if all leaks (including raw ones) are fixed.
    // The previous run showed 8, likely due to raw pointer leaks in the test code itself.
    std::cout << "Final MyData instance count check: " << MyData::instance_count << std::endl;
    return 0;
} // All remaining em::pointer objects are destroyed here via RAII
