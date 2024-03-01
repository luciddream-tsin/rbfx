//
// Copyright (c) 2022-2022 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include <Urho3D/Urho3D.h>

#include <EASTL/string.h>
#include <EASTL/string_view.h>

namespace Urho3D
{

/// File identifier, similar to Uniform Resource Identifier (URI).
/// Known differences:
/// - If URI starts with `/` or `x:/` it is treated as `file` scheme automatically.
/// - Host names are not supported for `file:` scheme.
///   All of `file:/path/to/file`, `file://path/to/file`, and `file:///path/to/file` are supported
///   and denote absolute file path.
/// - If URI does not contain `:`, it is treated as special "empty" scheme,
///   and the entire URI is treated as relative path.
/// - Conversion to URI string uses `scheme://` format.

/* 对这些规则的进一步中文注释:
 *
 * - 以 / 或 x:/ 开头被视为 file 协议:
 * 当文件标识符的路径以 / (在类 Unix 系统中表示根目录)开始, 或者以 x:/ (在 Windows 系统中, x 通常表示驱动器字母)开始时,
 * 它将自动被视为使用 file 协议. 这意味着不需要在路径前面显式添加 file: 来表示路径是一个文件路径.
 *
 * - file: 协议不支持主机名:
 * 对于 file: 协议, 不支持使用主机名. 这意味着, 即使在某些 URI 中可以使用 file://hostname/path 来指定一个网络上的文件路径,
 * 这里的文件标识符不支持这种用法. 无论是 file:/path/to/file or file://path/to/file 还是 file:///path/to/file,
 * 这些形式都是被支持的, 并且都表示一个绝对文件路径.

 * - 不包含冒号 : 视为特殊的“空”协议:
 * 如果文件标识符中没有包含协议分隔符 : , 那么它将被视为使用了一个特殊的"空"协议, 并且整个文件标识符被当作相对路径处理.
 * 这表示文件标识符仅仅是一个路径, 没有指定一个明确的协议来描述如何访问这个路径.

 * - 转换到 URI 字符串使用 scheme:// 格式:
 * 当将这种文件标识符转换为 URI 字符串时, 使用 scheme:// 的格式, 即使对于 file 协议也是如此.
 * 这个规则意味着, 转换后的 URI 将保持使用 :// 之后跟随路径的标准格式, 即使 file 协议并不需要使用两个斜线或者支持主机名.
 * TODO: 通过举例进一步说明
 * */

struct URHO3D_API FileIdentifier
{
    /// File identifier that references nothing.
    static const FileIdentifier Empty;

    /// Construct default.
    FileIdentifier() = default;
    /// Construct from scheme and path (as is).
    FileIdentifier(ea::string_view scheme, ea::string_view fileName);
    /// Deprecated. Use FromUri() instead.
    FileIdentifier(const ea::string& uri) : FileIdentifier(FromUri(uri)) {}

    /// Construct from uri-like path.
    static FileIdentifier FromUri(ea::string_view uri);
    /// Return URI-like path. Does not always return original path.
    ea::string ToUri() const;

    /// Append path to the current path, adding slash in between if it's missing.
    /// Ignores current scheme restrictions.
    void AppendPath(ea::string_view path);

    /// URI-like scheme. May be empty if not specified.
    ea::string scheme_;
    /// URI-like path to the file.
    ea::string fileName_;

    /// Return whether the identifier is empty.
    bool IsEmpty() const { return scheme_.empty() && fileName_.empty(); }

    /// Operators.
    /// @{
    explicit operator bool() const { return !IsEmpty(); }
    bool operator!() const { return IsEmpty(); }

    bool operator<(const FileIdentifier& rhs) const noexcept
    {
        return (scheme_ < rhs.scheme_) || (scheme_ == rhs.scheme_ && fileName_ < rhs.fileName_);
    }

    bool operator==(const FileIdentifier& rhs) const noexcept
    {
        return scheme_ == rhs.scheme_ && fileName_ == rhs.fileName_;
    }

    bool operator!=(const FileIdentifier& rhs) const noexcept { return !(rhs == *this); }

    FileIdentifier& operator+=(ea::string_view rhs)
    {
        AppendPath(rhs);
        return *this;
    }

    FileIdentifier operator+(ea::string_view rhs) const
    {
        FileIdentifier tmp = *this;
        tmp += rhs;
        return tmp;
    }
    /// @}

    static ea::string SanitizeFileName(ea::string_view fileName);
};

} // namespace Urho3D
