/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Fabien Caylus
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef SHAREDLIBRARY_H
#define SHAREDLIBRARY_H

#include <string>

#include "confinfo.h"

#if defined(CONFINFO_PLATFORM_LINUX) || defined(CONFINFO_PLATFORM_CYGWIN)
#include <dlfcn.h>
#elif defined(CONFINFO_PLATFORM_WIN32)
// TODO
#endif

namespace jp
{

//TODO: implement windows support

/**
 * @brief Provides cross-platform low-level access to a shared library.
 * This class allows to load/unload a library and access its symbols.
 *
 * Each SharedLibrary object represents one library and is constructed using the
 * path to library file.
 *
 * @note SharedLibrary objects are not copyable.
 */
class SharedLibrary
{
public:

    /**
     * @brief Default constructor
     * Construct an object with no library associated with.
     */
    SharedLibrary() {}
    /**
     * @brief Creates a SharedLibrary object and immediatly try to load it.
     * @param path The path to the shared library.
     */
    SharedLibrary(const char* path)
    { load(path); }
    /**
     * @overload SharedLibrary(const char* path)
     */
    SharedLibrary(const std::string& path)
    { load(path); }


    /**
     * @brief Default destructor
     */
    ~SharedLibrary() {}

    // Non-copyable
    SharedLibrary(const SharedLibrary& other) = delete;
    const SharedLibrary& operator=(const SharedLibrary& other) = delete;

    /**
     * @brief Load a library
     * If the object already represents a loaded library, this function will
     * first unload the previous library, and only after, load the new one.
     * @note If the object was constructed using a path, the library is already loaded.
     * @param path The path to the library to load
     * @return true on success
     */
    bool load(const char* path)
    {
        // Try to unload previous library
        if(isLoaded() && !unload())
            return false;
        return loadImpl(path);
    }

    /**
     * @overload bool load(const char* path)
     */
    bool load(const std::string& path)
    { return load(path.c_str()); }

    /**
     * @brief Checks if the library is loaded.
     * @return true if the library is loaded, otherwise returns false
     */
    bool isLoaded() const
    { return _handle != nullptr; }

    /**
     * @brief Unload the library
     * @return true on success (if the library is already unloaded, returns false)
     */
    bool unload()
    { return isLoaded() && unloadImpl(); }


    /**
     * @brief Checks for symbol
     * Checks if the library has the symbol specified by @a symbolName.
     * @param symbolName name of the symbole
     * @return true if the library has the symbol
     */
    bool hasSymbol(const char* symbolName)
    {
        std::string error = _lastError;
        getImpl(symbolName);
        bool has = _lastError.empty();
        _lastError = error;
        return has;
    }

    /**
     * @overload bool hasSymbol(const char* symbolName)
     */
    bool hasSymbol(const std::string& symbolName)
    { return hasSymbol(symbolName.c_str()); }

    /**
     * @brief Get a symbol
     * Returns the symbol specified by @a symbolName, and cast it to the type @Type.
     * It's the user responsability to ensure that Type match the library's symbol type.
     * @note Returns nullptr if the library doesn't have the symbol.
     * @param symbolName the symbol Name
     * @return the symbol object
     */
    template<typename Type>
    Type& get(const char* symbolName)
    { return *(reinterpret_cast<Type*>(reinterpret_cast<uintptr_t>(getImpl(symbolName)))); }

    /**
     * @overload get(const char* symbolName)
     */
    template<typename Type>
    Type& get(const std::string& symbolName)
    { return get<Type>(symbolName.c_str()); }

    /**
     * @brief Get the address of a symbol
     * @note Returns nullptr if the library doesn't have the symbol.
     * @param symbolName
     * @return The address
     */
    void* getRawAddress(const char* symbolName)
    { return getImpl(symbolName); }

    /**
     * @overload getRawAddress(const char* symbolName)
     */
    void* getRawAddress(const std::string& symbolName)
    { return getRawAdress(symbolName.c_str()); }

    /**
     * @brief Checks if the last call raise an error, or not.
     * Please not that each call of load(), unload(), get...() clears previous errors.
     * So this function can be used to check the success of the last called function.
     * @return true if an error occured
     * @see errorString()
     */
    bool hasError() const
    { return !_lastError.empty(); }

    /**
     * @brief Get the last error string
     * @return The error string
     * @see hasError()
     */
    std::string errorString() const
    { return _lastError; }

#if defined(CONFINFO_PLATFORM_LINUX) || defined(CONFINFO_PLATFORM_CYGWIN)
    typedef void* NativeLibHandle;
#elif defined(CONFINFO_PLATFORM_WIN32)
    typedef HINSTANCE NativeLibHandle;
#endif

    /**
     * @brief Get the native handle used on the system.
     * NativeLibHandle is a typedef for the native handle type used on the system.
     * @return The handle.
     */
    NativeLibHandle handle() const
    { return _handle; }

private:

    // Private members
    NativeLibHandle _handle = nullptr;
    std::string _lastError;

// Linux implementation
#if defined(CONFINFO_PLATFORM_LINUX) || defined(CONFINFO_PLATFORM_CYGWIN)
    bool loadImpl(const char* path)
    {
        _lastError.clear();
        _handle = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
        if(!_handle)
        {
            _lastError = dlerror();
            return false;
        }
        return true;
    }

    bool unloadImpl()
    {
        _lastError.clear();
        // dlclose returns 0 on success
        if(dlclose(_handle) != 0)
        {
            _lastError = dlerror();
            return false;
        }
        _handle = nullptr;
        return true;
    }

    void* getImpl(const char* symbolName)
    {
        _lastError.clear();
        dlerror();
        void* symbol = dlsym(_handle, symbolName);
        const char* error = dlerror();
        if(error)
        {
            // An error occured
            _lastError = error;
            return nullptr;
        }
        return symbol;
    }
#endif
};

} // namespace jp

#endif // SHAREDLIBRARY_H
