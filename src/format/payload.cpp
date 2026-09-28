#include "format/payload.hpp"

#include <cstdio>

namespace format {

namespace {

constexpr const char* kPlaceholder = "\xC2\xB7";  // middle dot

bool decode_utf8(std::string_view s, std::size_t& i, uint32_t& cp) {
    auto lead = static_cast<unsigned char>(s[i]);
    std::size_t length = 0;
    if (lead < 0x80) {
        cp = lead;
        length = 1;
    } else if ((lead & 0xE0) == 0xC0) {
        cp = lead & 0x1F;
        length = 2;
    } else if ((lead & 0xF0) == 0xE0) {
        cp = lead & 0x0F;
        length = 3;
    } else if ((lead & 0xF8) == 0xF0) {
        cp = lead & 0x07;
        length = 4;
    } else {
        return false;
    }
    if (i + length > s.size()) {
        return false;
    }
    for (std::size_t k = 1; k < length; ++k) {
        auto byte = static_cast<unsigned char>(s[i + k]);
        if ((byte & 0xC0) != 0x80) {
            return false;
        }
        cp = (cp << 6) | (byte & 0x3F);
    }
    bool overlong = (length == 2 && cp < 0x80) || (length == 3 && cp < 0x800) || (length == 4 && cp < 0x10000);
    if (overlong || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
        return false;
    }
    i += length;
    return true;
}

bool is_control(uint32_t cp) { return cp < 0x20 || (cp >= 0x7F && cp <= 0x9F); }

bool is_layout(uint32_t cp) { return cp == '\n' || cp == '\r' || cp == '\t'; }

bool starts_with(const std::string& text, std::string_view prefix) {
    return text.compare(0, prefix.size(), prefix) == 0;
}

}  // namespace

bool is_text(std::string_view bytes) {
    std::size_t i = 0;
    std::size_t glyphs = 0;
    std::size_t controls = 0;
    while (i < bytes.size()) {
        uint32_t cp = 0;
        if (!decode_utf8(bytes, i, cp)) {
            return false;
        }
        ++glyphs;
        if (is_control(cp) && !is_layout(cp)) {
            ++controls;
        }
    }
    return controls * 20 < glyphs || glyphs == 0;
}

bool is_text(const mqtt::Message& message) {
    if (message.props.payload_format && *message.props.payload_format == 1) {
        return true;
    }
    if (const auto& type = message.props.content_type) {
        if (starts_with(*type, "text/") || starts_with(*type, "application/json") ||
            starts_with(*type, "application/xml")) {
            return true;
        }
    }
    return is_text(message.payload);
}

std::string sanitize(std::string_view bytes) {
    std::string out;
    out.reserve(bytes.size());
    std::size_t i = 0;
    while (i < bytes.size()) {
        std::size_t start = i;
        uint32_t cp = 0;
        if (!decode_utf8(bytes, i, cp)) {
            out += kPlaceholder;
            ++i;
        } else if (cp == '\n') {
            out += '\n';
        } else if (cp == '\t') {
            out += "  ";
        } else if (is_control(cp)) {
            out += kPlaceholder;
        } else {
            out.append(bytes.substr(start, i - start));
        }
    }
    return out;
}

std::string preview(std::string_view bytes, std::size_t max_glyphs) {
    std::string out;
    std::size_t glyphs = 0;
    std::size_t i = 0;
    while (i < bytes.size()) {
        if (glyphs == max_glyphs) {
            out += "\xE2\x80\xA6";  // ellipsis
            break;
        }
        std::size_t start = i;
        uint32_t cp = 0;
        if (!decode_utf8(bytes, i, cp)) {
            out += kPlaceholder;
            ++i;
        } else if (is_layout(cp)) {
            out += ' ';
        } else if (is_control(cp)) {
            out += kPlaceholder;
        } else {
            out.append(bytes.substr(start, i - start));
        }
        ++glyphs;
    }
    return out;
}

std::vector<std::string> hex_dump(std::string_view bytes, std::size_t bytes_per_row) {
    std::vector<std::string> rows;
    for (std::size_t offset = 0; offset < bytes.size(); offset += bytes_per_row) {
        char header[16];
        std::snprintf(header, sizeof header, "%08zx  ", offset);
        std::string row = header;
        std::string ascii;
        for (std::size_t k = 0; k < bytes_per_row; ++k) {
            if (k == bytes_per_row / 2) {
                row += ' ';
            }
            if (offset + k < bytes.size()) {
                auto byte = static_cast<unsigned char>(bytes[offset + k]);
                char hex[4];
                std::snprintf(hex, sizeof hex, "%02x ", byte);
                row += hex;
                ascii += (byte >= 0x20 && byte < 0x7F) ? static_cast<char>(byte) : '.';
            } else {
                row += "   ";
            }
        }
        row += " |" + ascii + "|";
        rows.push_back(std::move(row));
    }
    return rows;
}

std::string hex_preview(std::string_view bytes, std::size_t max_bytes) {
    std::string out;
    std::size_t count = bytes.size() < max_bytes ? bytes.size() : max_bytes;
    for (std::size_t k = 0; k < count; ++k) {
        char hex[4];
        std::snprintf(hex, sizeof hex, "%02x", static_cast<unsigned char>(bytes[k]));
        if (k > 0) {
            out += ' ';
        }
        out += hex;
    }
    if (bytes.size() > max_bytes) {
        out += " \xE2\x80\xA6";
    }
    return out;
}

}  // namespace format
