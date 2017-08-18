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
class SharedLibrary
{
public:
    SharedLibrary() {}
    SharedLibrary(const std::string& path)
    { load(path); }
    SharedLibrary(const char* path)
    { load(path); }

    ~SharedLibrary() {}

    // Non-copyable
    SharedLibrary(const SharedLibrary& other) = delete;
    const SharedLibrary& operator=(const SharedLibrary& other) = delete;

    bool load(const char* path)
    {
        // Try to unload previous library
        if(isLoaded() && !unload())
            return false;
        return loadImpl(path);
    }

    bool load(const std::string& path)
    { return load(path.c_str()); }

    bool isLoaded() const
    { return _handle != nullptr; }

    bool unload()
    { return isLoaded() && unloadImpl(); }


    bool hasSymbol(const char* symbolName)
    {
        std::string error = _lastError;
        getImpl(symbolName);
        bool has = _lastError.empty();
        _lastError = error;
        return has;
    }

    bool hasSymbol(const std::string& symbolName)
    { return hasSymbol(symbolName.c_str()); }

    template<typename Type>
    Type& get(const char* symbolName)
    { return *(reinterpret_cast<Type*>(reinterpret_cast<uintptr_t>(getImpl(symbolName)))); }

    template<typename Type>
    Type& get(const std::string& symbolName)
    { return get<Type>(symbolName.c_str()); }

    void* getRawAdress(const char* symbolName)
    { return getImpl(symbolName); }

    void* getRawAdress(const std::string& symbolName)
    { return getRawAdress(symbolName.c_str()); }

    std::string errorString() const
    { return _lastError; }

#if defined(CONFINFO_PLATFORM_LINUX) || defined(CONFINFO_PLATFORM_CYGWIN)
    typedef void* NativeLibHandle;
#elif defined(CONFINFO_PLATFORM_WIN32)
    typedef HINSTANCE NativeLibHandle;
#endif

    NativeLibHandle handle() const
    { return _handle; }

private:

    // Private members
    NativeLibHandle _handle;
    std::string _lastError;

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
