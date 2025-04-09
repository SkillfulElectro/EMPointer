#ifndef EM_POINTER
#define EM_POINTER

#include <iostream>
#include <utility>
#include <cstddef>
#include <new>
#include <functional>
#include <iterator>
#include <type_traits>

namespace em {

    template<typename T> class pointer;

    template<typename T>
    class pointer {
        static_assert(!std::is_void<T>::value, "Use pointer<void> specialization for void");

    private:
        int* ptr_counter = nullptr;
        T* value = nullptr;
        T* original_value = nullptr;
        bool isArray = false;
        std::function<void(T*)> deleter = nullptr;

        void delete_ptr() {
            if (!ptr_counter) {
                value = nullptr;
                original_value = nullptr;
                isArray = false;
                deleter = nullptr;
                return;
            }

            --(*ptr_counter);

            if (*ptr_counter == 0) {
                delete ptr_counter;
                ptr_counter = nullptr;

                T* pointer_to_delete = original_value;

                if (deleter) {
                    try {
                        deleter(pointer_to_delete);
                    }
                    catch (...) { /* Cannot throw */ }
                }
                else {
                    if (pointer_to_delete) {
                        if (isArray) {
                            delete[] pointer_to_delete;
                        }
                        else {
                            delete pointer_to_delete;
                        }
                    }
                }

                value = nullptr;
                original_value = nullptr;
                isArray = false;
                deleter = nullptr;

            }
            else {
                ptr_counter = nullptr;
                value = nullptr;
                original_value = nullptr;
                isArray = false;
                deleter = nullptr;
            }
        }

        void setup_control_block(T* val, bool is_arr, std::function<void(T*)> d) {
            value = val;
            original_value = val;
            isArray = is_arr;
            deleter = std::move(d);

            if (value) {
                ptr_counter = new(std::nothrow) int(1);
                if (!ptr_counter) {
                    T* pointer_to_delete = original_value;
                    if (deleter && pointer_to_delete) {
                        try { deleter(pointer_to_delete); }
                        catch (...) {}
                    }
                    else if (!deleter && pointer_to_delete) {
                        if (isArray) delete[] pointer_to_delete; else delete pointer_to_delete;
                    }
                    value = nullptr;
                    original_value = nullptr;
                    isArray = false;
                    deleter = nullptr;
                }
            }
            else {
                ptr_counter = nullptr;
                original_value = nullptr;
                isArray = false;
                deleter = nullptr;
            }
        }

        template <typename U> friend class pointer;

    public:
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using pointer_type = T*;
        using reference = T&;
        using iterator_category = std::random_access_iterator_tag;

        explicit pointer(size_t size) {
            T* allocated_value = nullptr;
            try {
                allocated_value = new T[size];
            }
            catch (...) {
                allocated_value = nullptr;
            }
            setup_control_block(allocated_value, true, nullptr);
        }

        pointer(T* val) {
            setup_control_block(val, false, nullptr);
        }

        pointer(T* val, std::function<void(T*)> d) {
            setup_control_block(val, false, std::move(d));
        }

        template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
        pointer(const pointer<U>& other) :
            ptr_counter(nullptr),
            value(static_cast<T*>(other.value)),
            original_value(nullptr),
            isArray(false),
            deleter(nullptr)
        {}

        template <typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
        pointer(pointer<U>&& other) noexcept :
            ptr_counter(other.ptr_counter),
            value(static_cast<T*>(other.value)),
            original_value(static_cast<T*>(other.original_value)),
            isArray(other.isArray),
            deleter(nullptr)
        {
            other.ptr_counter = nullptr;
            other.value = nullptr;
            other.original_value = nullptr;
            other.isArray = false;
        }

        pointer() = default;

        pointer(const pointer& other) :
            ptr_counter(other.ptr_counter),
            value(other.value),
            original_value(other.original_value),
            isArray(other.isArray),
            deleter(other.deleter)
        {
            if (ptr_counter) {
                ++(*ptr_counter);
            }
        }

        pointer(pointer&& other) noexcept :
            ptr_counter(other.ptr_counter),
            value(other.value),
            original_value(other.original_value),
            isArray(other.isArray),
            deleter(std::move(other.deleter))
        {
            other.ptr_counter = nullptr;
            other.value = nullptr;
            other.original_value = nullptr;
            other.isArray = false;
        }


        ~pointer() {
            delete_ptr();
        }

        pointer& operator=(const pointer& other) {
            if (this != &other) {
                pointer temp(other);
                swap(temp);
            }
            return *this;
        }

        pointer& operator=(pointer&& other) noexcept {
            if (this != &other) {
                pointer temp(std::move(other));
                swap(temp);
            }
            return *this;
        }


        pointer& operator=(T* val) {
            pointer temp(val);
            swap(temp);
            return *this;
        }

        pointer& operator=(std::nullptr_t) {
            pointer temp;
            swap(temp);
            return *this;
        }


        T& operator*() const {
            return *value;
        }

        T* operator->() const {
            return value;
        }

        T& operator[](difference_type index) const {
            return value[index];
        }

        operator T* () const {
            return value;
        }

        explicit operator bool() const {
            return value != nullptr;
        }

        T* get_raw_ptr() const {
            return value;
        }

        T* get_original_ptr() const {
            return original_value;
        }

        T* do_not_manage() {
            T* released_ptr = original_value;
            if (ptr_counter) {
                --(*ptr_counter);
                ptr_counter = nullptr;
                value = nullptr;
                original_value = nullptr;
                isArray = false;
                deleter = nullptr;
            }
            else {
                released_ptr = nullptr;
            }
            return released_ptr;
        }


        bool is_null() const {
            return value == nullptr;
        }

        int use_count() const {
            return ptr_counter ? *ptr_counter : 0;
        }

        bool is_array() const {
            return (original_value != nullptr && ptr_counter != nullptr) ? isArray : false;
        }

        std::function<void(T*)> get_deleter() const {
            return deleter;
        }

        void swap(pointer& other) noexcept {
            using std::swap;
            swap(ptr_counter, other.ptr_counter);
            swap(value, other.value);
            swap(original_value, other.original_value);
            swap(isArray, other.isArray);
            swap(deleter, other.deleter);
        }

        pointer& operator++() {
            ++value;
            return *this;
        }

        pointer operator++(int) {
            pointer temp = *this;
            ++value;
            return temp;
        }

        pointer& operator--() {
            --value;
            return *this;
        }

        pointer operator--(int) {
            pointer temp = *this;
            --value;
            return temp;
        }

        pointer& operator+=(difference_type n) {
            value += n;
            return *this;
        }

        pointer& operator-=(difference_type n) {
            value -= n;
            return *this;
        }

        pointer operator+(difference_type n) const {
            pointer result;
            result.value = value + n;
            return result;
        }

        pointer operator-(difference_type n) const {
            pointer result;
            result.value = value - n;
            return result;
        }

        difference_type operator-(const pointer& other) const {
            return value - other.value;
        }


        template<typename N>
        explicit operator pointer<N>() const {
            pointer<N> casted_ptr;
            casted_ptr.value = static_cast<N*>(value);
            return casted_ptr;
        }
    };

    // --- Specialization for void ---
    template<>
    class pointer<void> {
    private:
        int* ptr_counter = nullptr;
        void* value = nullptr;
        void* original_value = nullptr;
        std::function<void(void*)> deleter = nullptr;

        void delete_ptr() {
            if (!ptr_counter) {
                value = nullptr;
                original_value = nullptr;
                deleter = nullptr;
                return;
            }

            --(*ptr_counter);

            if (*ptr_counter == 0) {
                delete ptr_counter;
                ptr_counter = nullptr;

                void* pointer_to_delete = original_value;

                if (deleter) {
                    try {
                        deleter(pointer_to_delete);
                    }
                    catch (...) { /* Cannot throw */ }
                }

                value = nullptr;
                original_value = nullptr;
                deleter = nullptr;
            }
            else {
                ptr_counter = nullptr;
                value = nullptr;
                original_value = nullptr;
                deleter = nullptr;
            }
        }

        void setup_control_block(void* val, std::function<void(void*)> d) {
            value = val;
            original_value = val;
            deleter = std::move(d);
            if (value) {
                ptr_counter = new(std::nothrow) int(1);
                if (!ptr_counter) {
                    void* pointer_to_delete = original_value;
                    if (deleter && pointer_to_delete) {
                        try { deleter(pointer_to_delete); }
                        catch (...) {}
                    }
                    value = nullptr;
                    original_value = nullptr;
                    deleter = nullptr;
                }
            }
            else {
                ptr_counter = nullptr;
                original_value = nullptr;
                deleter = nullptr;
            }
        }

        template <typename U> friend class pointer;

    public:
        using difference_type = std::ptrdiff_t;
        using value_type = void;
        using pointer_type = void*;

        pointer() = default;
        pointer(std::nullptr_t) : pointer() {}

        pointer(void* val, std::function<void(void*)> d) {
            setup_control_block(val, std::move(d));
        }

        template <typename T>
        pointer(T* val, std::function<void(T*)> d) : pointer(static_cast<void*>(val), [d = std::move(d)](void* vp) { if (d) d(static_cast<T*>(vp)); }) {}

        template <typename U>
        pointer(const pointer<U>& other) :
            ptr_counter(nullptr),
            value(other.value),
            original_value(nullptr),
            deleter(nullptr)
        {}

        template <typename U>
        pointer(pointer<U>&& other) noexcept :
            ptr_counter(other.ptr_counter),
            value(other.value),
            original_value(other.original_value),
            deleter(nullptr)
        {
            other.ptr_counter = nullptr;
            other.value = nullptr;
            other.original_value = nullptr;
            // other.isArray = false; // No isArray in void spec
        }


        pointer(const pointer& other) :
            ptr_counter(other.ptr_counter),
            value(other.value),
            original_value(other.original_value),
            deleter(other.deleter)
        {
            if (ptr_counter) {
                ++(*ptr_counter);
            }
        }

        pointer(pointer&& other) noexcept :
            ptr_counter(other.ptr_counter),
            value(other.value),
            original_value(other.original_value),
            deleter(std::move(other.deleter))
        {
            other.ptr_counter = nullptr;
            other.value = nullptr;
            other.original_value = nullptr;
        }

        ~pointer() {
            delete_ptr();
        }

        pointer& operator=(const pointer& other) {
            if (this != &other) {
                pointer temp(other);
                swap(temp);
            }
            return *this;
        }

        pointer& operator=(pointer&& other) noexcept {
            if (this != &other) {
                pointer temp(std::move(other));
                swap(temp);
            }
            return *this;
        }

        pointer& operator=(std::nullptr_t) {
            pointer temp;
            swap(temp);
            return *this;
        }

        template<typename U>
        pointer& operator=(const pointer<U>& other) {
            pointer temp(other);
            swap(temp);
            return *this;
        }
        template<typename U>
        pointer& operator=(pointer<U>&& other) {
            pointer temp(std::move(other));
            swap(temp);
            return *this;
        }

        operator void* () const {
            return value;
        }

        explicit operator bool() const {
            return value != nullptr;
        }

        void* get_raw_ptr() const {
            return value;
        }

        void* get_original_ptr() const {
            return original_value;
        }

        void* do_not_manage() {
            void* released_ptr = original_value;
            if (ptr_counter) {
                --(*ptr_counter);
                ptr_counter = nullptr;
                value = nullptr;
                original_value = nullptr;
                deleter = nullptr;
            }
            else {
                released_ptr = nullptr;
            }
            return released_ptr;
        }


        bool is_null() const {
            return value == nullptr;
        }

        int use_count() const {
            return ptr_counter ? *ptr_counter : 0;
        }

        std::function<void(void*)> get_deleter() const {
            return deleter;
        }

        void swap(pointer& other) noexcept {
            using std::swap;
            swap(ptr_counter, other.ptr_counter);
            swap(value, other.value);
            swap(original_value, other.original_value);
            swap(deleter, other.deleter);
        }

    };


    template<typename T>
    void swap(pointer<T>& lhs, pointer<T>& rhs) noexcept {
        lhs.swap(rhs);
    }

    inline void swap(pointer<void>& lhs, pointer<void>& rhs) noexcept {
        lhs.swap(rhs);
    }

    template<typename T, typename U>
    bool operator==(const pointer<T>& lhs, const pointer<U>& rhs) {
        return lhs.get_raw_ptr() == rhs.get_raw_ptr();
    }

    template<typename T, typename U>
    bool operator!=(const pointer<T>& lhs, const pointer<U>& rhs) {
        return !(lhs == rhs);
    }

    template<typename T>
    bool operator==(const pointer<T>& lhs, std::nullptr_t) {
        return lhs.get_raw_ptr() == nullptr;
    }

    template<typename T>
    bool operator==(std::nullptr_t, const pointer<T>& rhs) {
        return nullptr == rhs.get_raw_ptr();
    }

    template<typename T>
    bool operator!=(const pointer<T>& lhs, std::nullptr_t) {
        return !(lhs == nullptr);
    }

    template<typename T>
    bool operator!=(std::nullptr_t, const pointer<T>& rhs) {
        return !(nullptr == rhs);
    }

    template<typename T, typename U>
    bool operator<(const pointer<T>& lhs, const pointer<U>& rhs) {
        using Common = std::common_type_t<typename pointer<T>::pointer_type, typename pointer<U>::pointer_type>;
        return std::less<Common>()(lhs.get_raw_ptr(), rhs.get_raw_ptr());
    }

    template<typename T, typename U>
    bool operator>(const pointer<T>& lhs, const pointer<U>& rhs) {
        return rhs < lhs;
    }

    template<typename T, typename U>
    bool operator<=(const pointer<T>& lhs, const pointer<U>& rhs) {
        return !(rhs < lhs);
    }

    template<typename T, typename U>
    bool operator>=(const pointer<T>& lhs, const pointer<U>& rhs) {
        return !(lhs < rhs);
    }

    template<typename T>
    bool operator<(const pointer<T>& lhs, std::nullptr_t) {
        return std::less<typename pointer<T>::pointer_type>()(lhs.get_raw_ptr(), nullptr);
    }

    template<typename T>
    bool operator<(std::nullptr_t, const pointer<T>& rhs) {
        return std::less<typename pointer<T>::pointer_type>()(nullptr, rhs.get_raw_ptr());
    }

    template<typename T>
    bool operator>(const pointer<T>& lhs, std::nullptr_t) {
        return nullptr < lhs;
    }

    template<typename T>
    bool operator>(std::nullptr_t, const pointer<T>& rhs) {
        return rhs < nullptr;
    }

    template<typename T>
    bool operator<=(const pointer<T>& lhs, std::nullptr_t) {
        return !(nullptr < lhs);
    }

    template<typename T>
    bool operator<=(std::nullptr_t, const pointer<T>& rhs) {
        return !(rhs < nullptr);
    }

    template<typename T>
    bool operator>=(const pointer<T>& lhs, std::nullptr_t) {
        return !(lhs < nullptr);
    }

    template<typename T>
    bool operator>=(std::nullptr_t, const pointer<T>& rhs) {
        return !(nullptr < rhs);
    }

}
#endif // !EM_POINTER
