#include <cassert>
#include <string>

#include "format/payload.hpp"
#include "format/units.hpp"

int main() {
    using namespace format;

    assert(is_text("hello"));
    assert(is_text("{\"t\":21.4}\n"));
    assert(is_text("caf\xC3\xA9"));
    assert(!is_text(std::string("\xFF\xD8\xFF\xE0", 4)));
    assert(!is_text(std::string("\x00\x01\x02\x03" "abc", 7)));

    mqtt::Message binary_by_bytes;
    binary_by_bytes.payload = std::string("\x01\x02\x03", 3);
    assert(!is_text(binary_by_bytes));
    binary_by_bytes.props.payload_format = 1;
    assert(is_text(binary_by_bytes));

    mqtt::Message json;
    json.payload = std::string("\x01\x02\x03", 3);
    json.props.content_type = "application/json";
    assert(is_text(json));

    assert(sanitize("a\tb\x01" "c") == "a  b\xC2\xB7" "c");
    assert(sanitize("line\nnext") == "line\nnext");
    assert(preview("line\nnext", 20) == "line next");
    assert(preview("abcdefgh", 3) == "abc\xE2\x80\xA6");
    assert(preview("\xC3\xA9\xC3\xA9\xC3\xA9", 2) == "\xC3\xA9\xC3\xA9\xE2\x80\xA6");

    auto rows = hex_dump(std::string("\xFF\xD8\xFF\xE0JFIF\x00\x01", 10), 8);
    assert(rows.size() == 2);
    assert(rows[0] == "00000000  ff d8 ff e0  4a 46 49 46  |....JFIF|");
    assert(rows[1] == "00000008  00 01                     |..|");
    assert(hex_preview(std::string("\xFF\xD8\xFF", 3), 2) == "ff d8 \xE2\x80\xA6");

    assert(human_bytes(512) == "512 B");
    assert(human_bytes(1536) == "1.5 KiB");
    assert(duration_short(std::chrono::seconds(45)) == "45s");
    assert(duration_short(std::chrono::seconds(3725)) == "1h02m");
    assert(pad_right("ab", 4) == "ab  ");
    return 0;
}
