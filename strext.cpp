//  Strings library extentions.
//
//  Copyright (C) 2024, Martin Young <martin_young@live.cn>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see <https://www.gnu.org/licenses/>.
//------------------------------------------------------------------------

#include <random>
#include <cstddef>
#include <iterator>
#include "strext.hpp"

using namespace std::string_literals;
using namespace std::string_view_literals;

const std::string EMPTY_STR{};
const std::string SPACE_CHARS{" \t\n\r\f\v"s};

//----------------------------------------------------------------------------------------------------
// My version
// std::string replall(const std::string& src, const std::string& sfind, const std::string& swith)
// {
//     if( src.empty() || sfind.empty() ) return src;
//
//     size_t pos = src.find(sfind);
//     if (nposs(pos)) return src;
//
//     size_t nfrom{};
//     std::string res;
//     do {
//         res += src.substr(nfrom, pos-nfrom)+swith;
//         nfrom = pos+sfind.length();
//         pos = src.find(sfind, nfrom);
//     } while( !nposs(pos) );
//     if( nfrom < src.size() ) res += src.substr(nfrom);
//     return res;
// }
//----------------------------------------------------------------------------------------------------
// Deepseek version, reviewed by Gemini
std::string replall(std::string_view src, std::string_view sfind, std::string_view swith)
{
    // 边界条件处理: 如果查找的字符串为空，合理的行为是直接返回原字符串以避免无限循环。
    if( src.empty() || sfind.empty() || sfind.length() > src.length() )
        return std::string{src};

    // 第一阶段：使用 Boyer-Moore 搜索器收集所有不重叠的匹配位置
    const auto searcher = std::boyer_moore_searcher(sfind.begin(), sfind.end());
    std::vector<size_t> matches{};
    auto it = src.begin();
    const auto end = src.end();

    while (it != end) {
        const auto match = std::search(it, end, searcher);
        if (match == end)  break;
        matches.emplace_back(static_cast<size_t>(std::distance(src.begin(), match)));
        it = match + sfind.length();
    }

    if (matches.empty()) return std::string{src};

    // 第二阶段：构建结果字符串
    std::string result{};

    // 预分配内存（BUG 修正 by Gemini）
    // 使用有符号的 ptrdiff_t 计算长度差，以防止整数下溢。
    const ptrdiff_t diff = static_cast<ptrdiff_t>(swith.length()) - static_cast<ptrdiff_t>(sfind.length());
    if (diff > 0) {   // 仅当字符串增长时预分配，这是最主要优化点
        const size_t new_capacity = src.length() + matches.size() * diff;
        result.reserve(new_capacity);
    }

    size_t last_pos{};
    const size_t find_len = sfind.length();
    for (const auto& pos : matches) {
        result.append(src.data() + last_pos, pos - last_pos); // 使用 data() + 指针偏移可能更高效
        result.append(swith);
        last_pos = pos + find_len;
    }
    result.append(src.data() + last_pos, src.length() - last_pos);

    return result;
}
//----------------------------------------------------------------------------------------
std::string genPassword(PasswordSecurityLevel level, size_t length)
{
    const std::string lowercase    {"abcdefghijklmnopqrstuvwxyz"};
    const std::string uppercase    {"ABCDEFGHIJKLMNOPQRSTUVWXYZ"};
    const std::string digits       {"0123456789"};
    const std::string specialChars {"!@#$%^&*()_+-=[]{}|;':\",./<>?"};

    std::vector<std::string> required_charsets;
    std::string full_charset;

    switch (level) {
        case PasswordSecurityLevel::LOW:
            required_charsets = {lowercase, digits};
            full_charset = lowercase + digits;
            break;
        case PasswordSecurityLevel::MEDIUM:
            required_charsets = {lowercase, uppercase, digits};
            full_charset = lowercase + uppercase + digits;
            break;
        case PasswordSecurityLevel::HIGH:
        default:
            required_charsets = {lowercase, uppercase, digits, specialChars};
            full_charset = lowercase + uppercase + digits + specialChars;
    }

    const size_t min_length {required_charsets.size()};
    length = std::max(min_length, length);

    static thread_local std::mt19937 gen(std::random_device{}());

    std::string password;
    for (const auto& charset : required_charsets) {
        std::uniform_int_distribution<size_t> dis(0, charset.size() - 1);
        password += charset[dis(gen)];
    }

    if (length > min_length) {
        std::uniform_int_distribution<size_t> dis(0, full_charset.size() - 1);
        for (size_t i = min_length; i < length; ++i)
            password += full_charset[dis(gen)];
    }

    std::shuffle(password.begin(), password.end(), gen);

    return password;
}
//----------------------------------------------------------------------------------------
bool chkPassword(std::string_view password, PasswordSecurityLevel level, size_t min_length)
{
    constexpr std::string_view specialChars {"!@#$%^&*()_+-=[]{}|;':\",./<>?"};

    enum CharFlags : uint8_t {
        LOWER = 1 << 0,
        UPPER = 1 << 1,
        DIGIT = 1 << 2,
        SPECIAL = 1 << 3
    };

    if( password.length() < min_length ) return false;

    uint8_t flags{};

    const auto check_complete = [level, flags] {
        switch (level) {
            case PasswordSecurityLevel::LOW:
                return (flags & LOWER) && (flags & DIGIT);
            case PasswordSecurityLevel::MEDIUM:
                return (flags & LOWER) && (flags & UPPER) && (flags & DIGIT);
            case PasswordSecurityLevel::HIGH:
                return (flags & LOWER) && (flags & UPPER) && (flags & DIGIT) && (flags & SPECIAL);
        }
        return false;
    };

    for (const char c : password) {
        if      (std::islower(c)) flags |= LOWER;
        else if (std::isupper(c)) flags |= UPPER;
        else if (std::isdigit(c)) flags |= DIGIT;
        else if (!npossv(specialChars.find(c))) flags |= SPECIAL;

        if (check_complete()) return true;
    }
    return check_complete();
}
//----------------------------------------------------------------------------------------
