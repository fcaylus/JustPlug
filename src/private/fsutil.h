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

#ifndef FSUTIL_H
#define FSUTIL_H

/*
 * This file is an internal header. It's not part of the public API,
 * and may change at any moment.
 */

#include <string> // for std::string
#include <vector> // for std::vector

/*
 * Collection of some useful filesystem functions.
 */

namespace jp_private
{
namespace fsutil
{

// Returns the extension (without dot) of libraries
std::string libraryExtension();

// Return the suffix for libraries (.extension)
std::string librarySuffix();

typedef std::vector<std::string> PathList;

// List files in the specified directory, and append them to filesList
// The search can be recursive across directories
// extFilter can be used to search only for specified files
// NOTE: Return false on error (even if errors occurs, filesList
// could have been modified)
// NOTE: This function sets the errno variable in case of errors
bool listFilesInDir(const std::string& rootDir,
                    PathList* filesList,
                    const std::string& extFilter = std::string(),
                    bool recursive = false);

bool listLibrariesInDir(const std::string& rootDir,
                        PathList* filesList,
                        bool recursive = false);

// Returns the app directory
// Use whereami library
std::string appDir();

} // namespace fsutil
} // namespace jp_private

#endif // FSUTIL_H
