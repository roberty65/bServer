#ifndef _BASE64_STRING_H_
#define _BASE64_STRING_H_

#include <string>

namespace Base64 {

static const char base64_encode[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char base64_decode[] = {
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  /*  0-15 */
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  /*  16-31 */
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,  /*  32-47 ('+', '/'   */
52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,  /*  48-63 ('0' - '9') */
-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,  /*  64-79 ('A' - 'Z'  */
15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,  /*  80-95 */
-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,  /*  96-111 ('a' - 'z') */
41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, -1, -1, -1, -1,  /*  112-127 */};

char GetBase64Value(char c)
{
	if ((c >= 'A') && (c <= 'Z')) {
		return c - 'A';
	} else if ((c >= 'a') && (c <= 'z')) {
		return c - 'a' + 26;
	} else if ((c >= '0') && (c <= '9')) {
		return c - '0' + 52;
	}
	switch (c) {
	case '+':
		return 62; // 0x3E
	case '/':
		return 63; // 0x3F
	case '=':
	default:
		return 0;
	}
}

int Encode(const char* src, char* dst, int len)
{
	int dst_len = 0;
	const unsigned char* bytes = reinterpret_cast<const unsigned char*>(src);
	while (len != 0) {
		*dst++ = base64_encode[(bytes[0] >> 2) & 0x3F];
		if (len > 2) {
			*dst++ = base64_encode[((bytes[0] & 0x3) << 4) | (bytes[1] >> 4)];
			*dst++ = base64_encode[((bytes[1] & 0xF) << 2) | (bytes[2] >> 6)];
			*dst++ = base64_encode[bytes[2] & 0x3F];
		} else { // len <= 2
			switch (len) {
				case 1:
					*dst++ = base64_encode[(bytes[0] & 0x3) << 4];
					*dst++ = '=';
					*dst++ = '=';
					break;
				case 2:
					*dst++ = base64_encode[((bytes[0] & 0x3) << 4) | (bytes[1] >> 4)];
					*dst++ = base64_encode[((bytes[1] & 0xF) << 2)];
					*dst++ = '=';
					break;
			}
			dst_len += 4;
			break;
		}
		bytes   += 3;
		len     -= 3;
		dst_len += 4;
	}
	*dst = 0;
	return dst_len;
}

int Decode(const char* src, char* dst, int len)
{
	int dst_len = 0;
	if ((len < 4) || (len % 4 != 0)) {
		*dst = 0;
		return dst_len;
	}
	const unsigned char* bytes = reinterpret_cast<const unsigned char*>(src);
	while (len != 4) {
		*dst++ = (base64_decode[static_cast<unsigned int>(bytes[0])] << 2) | (base64_decode[static_cast<unsigned int>(bytes[1])] >> 4);
		*dst++ = (base64_decode[static_cast<unsigned int>(bytes[1])] << 4) | (base64_decode[static_cast<unsigned int>(bytes[2])] >> 2);
		*dst++ = (base64_decode[static_cast<unsigned int>(bytes[2])] << 6) | (base64_decode[static_cast<unsigned int>(bytes[3])]);
		bytes   += 4;
		len     -= 4;
		dst_len += 3;
	}
	if (bytes[3] != '=') {
		*dst++ = (base64_decode[static_cast<unsigned int>(bytes[0])] << 2) | (base64_decode[static_cast<unsigned int>(bytes[1])] >> 4);
		*dst++ = (base64_decode[static_cast<unsigned int>(bytes[1])] << 4) | (base64_decode[static_cast<unsigned int>(bytes[2])] >> 2);
		*dst++ = (base64_decode[static_cast<unsigned int>(bytes[2])] << 6) | (base64_decode[static_cast<unsigned int>(bytes[3])]);
		dst_len += 3;
	} else { // bytes[3] == '='
		if (bytes[2] != '=') {
			*dst++ = (base64_decode[static_cast<unsigned int>(bytes[0])] << 2) | (base64_decode[static_cast<unsigned int>(bytes[1])] >> 4);
			*dst++ = (base64_decode[static_cast<unsigned int>(bytes[1])] << 4) | (base64_decode[static_cast<unsigned int>(bytes[2])] >> 2);
			dst_len += 2;
		} else { // bytes[2] == '='
			*dst++   = (base64_decode[static_cast<unsigned int>(bytes[0])] << 2) | (base64_decode[static_cast<unsigned int>(bytes[1])] >> 4);
			dst_len += 1;
		}
	}
	*dst = 0;
	return dst_len;
}

std::string Encode(const std::string& src)
{
	char buffer[(src.length() + 2) / 3 * 4 + 1];
	Encode(src.c_str(), buffer, src.length());
	return buffer;
}

std::string Decode(const std::string& src)
{
	char buffer[(src.length() + 3) / 4 * 3 + 1];
	Decode(src.c_str(), buffer, src.length());
	return buffer;
}
};

#endif
