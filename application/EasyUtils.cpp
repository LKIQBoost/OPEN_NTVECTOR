#include "EasyUtils.h"
#pragma once
using namespace std;
class Easy {
public:
	// 用于填充随机字节的函数
	static std::string fillRandomBytes(const std::string& text, size_t target_length) {
		if (target_length <= text.length()) {
			throw std::invalid_argument("Target length must be greater than original string length");
		}

		std::string result = text;
		result.reserve(target_length);

		size_t bytes_to_fill = target_length - text.length();

		// 使用随机设备作为随机数种子
		std::random_device rd;
		std::mt19937 gen(rd());

		// 修正：使用 int 而不是 uint8_t，然后转换为 char
		std::uniform_int_distribution<int> dist(0, 255);

		for (size_t i = 0; i < bytes_to_fill; ++i) {
			result.push_back(static_cast<unsigned char>(dist(gen)));
		}

		return result;
	}
	static std::string to_binary_string(const std::string&str) {
		std::string binary;
		binary.reserve(str.size() * 8); // 每个字符8位+1空格

		for (const auto& ch : str) {
			unsigned char c = static_cast<unsigned char>(ch);
			for (int i = 7; i >= 0; --i) {
				binary += (c & (1 << i)) ? '1' : '0';
			}
		}

		return binary;
	}
    static std::string encrypt(const std::string& message, int offset, int rounds) {
        if (message.empty()) return {};

        static const uint32_t shiftTable[] = { 0xD0A0601, 0xE090502, 0x30B0704, 0x50B0803, 0xE0B0701 };
        static const uint32_t constTable[] = {
            0xF61E2562, 0xC040B340, 0x265E5A51, 0xE9B6C7AA, 0xA4BEEA44, 0x4BDECFA9, 0xF6BB4B60, 0xBEBFBC70,
            0x655B59C3, 0x8F0CCC92, 0xFFEFF47D, 0x85845DD1, 0x289B7EC6, 0xEAA127FA, 0xD4EF3085, 0x04881D05,
            0x21E1CDE6, 0xC33707D6, 0xF4D50D87, 0x455A14ED, 0xF61E2562, 0xC040B340, 0x265E5A51, 0xE9B6C7AA,
            0xA4BEEA44, 0x4BDECFA9, 0xF6BB4B60, 0xBEBFBC70, 0x655B59C3, 0x8F0CCC92, 0xFFEFF47D, 0x85845DD1,
            0x289B7EC6, 0xEAA127FA, 0xD4EF3085, 0x04881D05, 0x21E1CDE6, 0xC33707D6, 0xF4D50D87, 0x455A14ED
        };

        // 填充字符串
        std::string padded = message;
        while (padded.length() % 4 != 0) padded += '0';

        // 分组处理
        std::vector<uint32_t> group;
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(padded.c_str());
        for (size_t i = 0; i < padded.length(); i += 4) {
            group.push_back((bytes[i] << 24) | (bytes[i + 1] << 16) | (bytes[i + 2] << 8) | bytes[i + 3]);
        }

        // 填充到64倍数
        while (group.size() % 64 != 0) group.push_back(0xABCDE987);

        uint32_t shiftBase = shiftTable[offset];
        uint32_t s0 = shiftBase & 0xFF, s1 = (shiftBase >> 8) & 0xFF;
        uint32_t s2 = (shiftBase >> 16) & 0xFF, s3 = (shiftBase >> 24) & 0xFF;

        int cb = offset * 4;
        uint32_t c0 = constTable[cb], c1 = constTable[cb + 1];
        uint32_t c2 = constTable[cb + 2], c3 = constTable[cb + 3];

        uint32_t hA = 0x67452301, hB = 0xEFCDAB89, hC = 0x98BADCFE, hD = 0x10325476;

        for (int r = 0; r < rounds; r++) {
            for (size_t i = 0; i < group.size(); i += 4) {
                uint32_t chunk = group[i];

                hA = hB + ((chunk + c0 + (hB & hC | hD & ~hB) + hA) << s0);
                hB = hB + ((hA + chunk + c1 + ((hC & ~hD) | (hD & hB))) << s1);
                hC = hB + ((hA + chunk + c2 + (hB ^ hD ^ hC)) << s2);
                hD = hB + ((hA + chunk + c3 + (hC ^ (hB | hD))) << s3);
            }
        }

        // 小端序输出
        char result[16];
        for (int i = 0; i < 4; i++) {
            result[i] = hA >> (i * 8);
            result[i + 4] = hB >> (i * 8);
            result[i + 8] = hC >> (i * 8);
            result[i + 12] = hD >> (i * 8);
        }
        return std::string(result, 16);
    }
	static string* GetRandomIV(int length) {
		if (length == 0)
			return NULL;
		string* reulst = new string("");
		int min = 0, max = 61;
		random_device seed;
		ranlux48 engine(seed());
		uniform_int_distribution<> distrib(min, max);
		for (size_t i = 0; i < length; i++)
		{
			int random = distrib(engine);
			*reulst += IV_ASCII_BASE[random];

		}
		return reulst;
	}
	static std::string StringToHex(const std::string& data,bool B = false)
	{
		std::string hex;
		if (B)
			hex = BASE_16B;
		else
			hex = BASE_16;
		std::stringstream ss;
		int leng = data.size();
		for (std::string::size_type i = 0; i < leng; ++i)
			ss << hex[(unsigned char)data[i] >> 4] << hex[(unsigned char)data[i] & 0xf];
		return ss.str();
	}
	static std::string HexToString(const std::string& hex) {
		if (hex.length() % 2 != 0) {
			throw std::invalid_argument("Hex string must have even length");
		}

		std::string result;
		result.reserve(hex.length() / 2);

		for (size_t i = 0; i < hex.length(); i += 2) {
			std::string byteString = hex.substr(i, 2);
			char byte = static_cast<char>(strtol(byteString.c_str(), nullptr, 16));
			if (byte == 0 && byteString != "00") {
				throw std::invalid_argument("Invalid hex character in input");
			}
			result.push_back(byte);
		}

		return result;
	}
	static std::string StringToHex(const std::string& data, int length, bool B = false)
	{
		std::string hex;
		if (B)
			hex = BASE_16B;
		else
			hex = BASE_16;
		std::stringstream ss;
		int leng = length;
		for (std::string::size_type i = 0; i < leng; ++i)
			ss << hex[(unsigned char)data[i] >> 4] << hex[(unsigned char)data[i] & 0xf];
		return ss.str();
	}
	static std::string StringToHex_s(const char* data, int length, bool B = false)
	{
		std::string hex;
		if (B)
			hex = BASE_16B;
		else
			hex = BASE_16;
		std::stringstream ss;
		int leng = length;
		for (std::string::size_type i = 0; i < leng; ++i)
			ss << hex[(unsigned char)data[i] >> 4] << hex[(unsigned char)data[i] & 0xf];
		return ss.str();
	}
    static void ComputeDynamicToken(string Token, string body, string url, string* result) {
        string params = StringToHex_s(Token.data(), 16);
        params.append(body);
        params.append("0eGsBkhl");
        params.append(url);
        unsigned char md[16];
        // 计算 MD5 值

        MD5((unsigned char*)params.data(), params.length(), md);
        string md5hex = StringToHex_s((char*)md, 16);
        string data = to_binary_string(md5hex);
        data = data.substr(0, 256);
        string out = data.substr(6, data.length() - 6) + data.substr(0, 6);
        for (int i = 0; i < 32; i++)
        {
            string text2 = out.substr(i * 8, 8);
            char b = 0;
            for (int j = 0; j < 8; j++)
            {
                if (text2[7 - j] == '1')
                {
                    b = (char)((int)b | (1 << j));
                }
            }
            md5hex[i] = (char)(b ^ md5hex[i]);
        }
        char* output = new char[md5hex.length() * 2 + 6];
        Base64Encoding((const unsigned char*)md5hex.data(), md5hex.length(), output);
        md5hex = output;
        delete[] output;
        md5hex = md5hex.substr(0, 16);
        for (int i = 0;i < md5hex.length();i++) {
            if (md5hex[i] == '+')
                md5hex[i] = 'm';
            if (md5hex[i] == '/')
                md5hex[i] = 'o';
        }
        md5hex += '1';
        result->assign(md5hex);
    }
    static std::string ComputeDynamicToken(string Token, string body, string url) {
        string params = Token.data();
        params.append(body);
        params.append("0eGsBkhl");
        params.append(url);
        unsigned char md[16];
        // 计算 MD5 值

        MD5((unsigned char*)params.data(), params.length(), md);
        string md5hex = StringToHex_s((char*)md, 16);
        string data = to_binary_string(md5hex);
        data = data.substr(0, 256);
        string out = data.substr(6, data.length() - 6) + data.substr(0, 6);
        for (int i = 0; i < 32; i++)
        {
            string text2 = out.substr(i * 8, 8);
            char b = 0;
            for (int j = 0; j < 8; j++)
            {
                if (text2[7 - j] == '1')
                {
                    b = (char)((int)b | (1 << j));
                }
            }
            md5hex[i] = (char)(b ^ md5hex[i]);
        }
        char* output = new char[md5hex.length() * 2 + 6];
        Base64Encoding((const unsigned char*)md5hex.data(), md5hex.length(), output);
        md5hex = output;
        delete[] output;
        md5hex = md5hex.substr(0, 16);
        for (int i = 0;i < md5hex.length();i++) {
            if (md5hex[i] == '+')
                md5hex[i] = 'm';
            if (md5hex[i] == '/')
                md5hex[i] = 'o';
        }
        md5hex += '1';
        return md5hex;
    }
	static void HttpEncrypt(string* out, string* in) {
		string* pad = GetRandomIV(16);
		string inputdata = *in;
		inputdata.append(*pad);
		delete pad;
		int body_length = inputdata.length();
		int length = 0;
		int m = body_length % 16;
		if (m == 0) {
			length = (body_length / 16) * 16;
		}
		else
		{
			length = (body_length / 16 + 1) * 16;
			inputdata.append(Paddingion, 16 - m - 1);
		}
		unsigned char* oute = (unsigned char*)malloc(length + 17);
		if (oute == NULL) {
			free(oute);
			return;
		}
		memset(oute, 0, length + 17);

		AES_KEY encrypt_key;
		int min = 0, max = 15;
		random_device seed;
		ranlux48 engine(seed());
		uniform_int_distribution<> distrib(min, max);
		int random = distrib(engine);
		AES_set_encrypt_key((unsigned char*)neteasehttpkey[random], 128, &encrypt_key);
		string* iv = GetRandomIV(16);
		memcpy(oute, iv->data(), 16);
		unsigned char* outdata = (unsigned char*)malloc(length);
		if (outdata != NULL) {
			memset(outdata, 0, length);
			AES_cbc_encrypt((unsigned char*)inputdata.data(), outdata, length, &encrypt_key, (unsigned char*)iv->data(), AES_ENCRYPT);
			memcpy(oute + 16, outdata, length);
			oute[length + 16] = (random << 4) + 12;
			out->assign((char*)oute, length + 17);
			free(outdata);
			free(oute);
			delete iv;
		}
		else
		{
			free(outdata);
			free(oute);
			delete iv;
		}
	}

	static void HttpDecrypt(string* out, string* in) {
		string data = *in;
		int inlen = data.length();
		int aeslen = inlen - 17;
		if (inlen < 33) {
			return;
		}
		if (inlen % 16 != 1) {
			return;
		}
		char aeskey[16];
		//索引到末尾字节
		unsigned char keya = data.data()[inlen - 1];
		int keyp = keya >> 4;
		if (keyp > 16)
			return;
		if (keyp < 0)
			return;
		memcpy(aeskey, (char*)neteasehttpkey[keyp],16);
		char aesiv[16];
		memcpy(aesiv, data.data(),16);
		AES_KEY aes_key;
		if (AES_set_decrypt_key((const unsigned char*)aeskey, 128, &aes_key) != 0) {
			return;
		}
		char* indata = new char[aeslen];
		char* outdata = new char[aeslen];
		memcpy(indata,data.data()+16, aeslen);
		AES_cbc_encrypt((const unsigned char*)indata,
			(unsigned char*)outdata,
			aeslen,
			&aes_key,
			(unsigned char*)aesiv,
			AES_DECRYPT);
		int index = aeslen - 1;
		int c = 0;
		int dst_index = 0;
		while (true) {
			if (outdata[index] == 0) {
				dst_index++;
				index--;
			}
			else
				break;
			if (c > 16)
				break;
		}
		out->assign(outdata, aeslen - dst_index - 128);
		delete[] indata;
		delete[] outdata;
		// 解除PKCS7Padding填充
		return;
	}
	static std::string encrypt_with_tail(const std::string& input) {
		if (input.empty()) {
			return "";
		}

		try {
			// 获取随机填充
			auto pad = GetRandomIV(16);
			if (!pad || pad->length() != 16) {
				return "";
			}

			std::string inputdata = input;
			inputdata.append(*pad);

			// 计算填充长度 - 零填充到16字节倍数
			size_t body_length = inputdata.length();
			size_t m = body_length % 16;
			size_t length = (m == 0) ? body_length : ((body_length / 16 + 1) * 16);

			// 零填充
			if (m != 0) {
				inputdata.append(16 - m, '\0');
			}

			// 分配输出缓冲区
			std::vector<unsigned char> oute(length + 17, 0);

			// 设置加密密钥
			AES_KEY encrypt_key;
			std::random_device seed;
			std::ranlux48 engine(seed());
			std::uniform_int_distribution<> distrib(0, 15);
			int random = distrib(engine);

			if (AES_set_encrypt_key(neteasehttpkey[random], 128, &encrypt_key) != 0) {
				return "";
			}

			// 获取随机IV
			auto iv = GetRandomIV(16);
			if (!iv || iv->length() != 16) {
				return "";
			}

			// 复制IV到输出
			memcpy(oute.data(), iv->c_str(), 16);

			// 加密数据
			std::vector<unsigned char> outdata(length, 0);
			AES_cbc_encrypt(
				reinterpret_cast<const unsigned char*>(inputdata.data()),
				outdata.data(),
				length,
				&encrypt_key,
				(unsigned char*)(iv->data()),
				AES_ENCRYPT
			);

			// 组合输出
			memcpy(oute.data() + 16, outdata.data(), length);
			oute[length + 16] = static_cast<unsigned char>((random << 4) + 4);

			return std::string(reinterpret_cast<const char*>(oute.data()), length + 17);

		}
		catch (const std::exception& e) {
			return "";
		}
	}

	static std::string decrypt_with_tail(const std::string& input) {
		if (input.empty()) {
			return "";
		}

		try {
			// 输入验证
			size_t inlen = input.length();
			if (inlen < 33) {
				return "";
			}

			if (inlen % 16 != 1) {
				return "";
			}

			size_t aeslen = inlen - 17;
			if (aeslen <= 0 || aeslen % 16 != 0) {
				return "";
			}

			// 提取密钥索引
			unsigned char keya = static_cast<unsigned char>(input[inlen - 1]);
			int keyp = keya >> 4;

			if (keyp >= 16 || keyp < 0) {
				return "";
			}

			// 设置解密密钥
			unsigned char aeskey[16];
			memcpy(aeskey, neteasehttpkey[keyp], 16);

			AES_KEY aes_key;
			if (AES_set_decrypt_key(aeskey, 128, &aes_key) != 0) {
				return "";
			}

			// 提取IV
			unsigned char aesiv[16];
			memcpy(aesiv, input.data(), 16);

			// 分配缓冲区
			std::vector<unsigned char> indata(aeslen);
			std::vector<unsigned char> outdata(aeslen);

			// 复制加密数据
			memcpy(indata.data(), input.data() + 16, aeslen);

			// 解密数据
			AES_cbc_encrypt(
				indata.data(),
				outdata.data(),
				aeslen,
				&aes_key,
				aesiv,
				AES_DECRYPT
			);

			// 移除零填充 - 从末尾开始移除连续的零字节
			size_t data_length = aeslen;
			for (int i = aeslen - 1; i >= 0; --i) {
				if (outdata[i] == 0) {
					data_length--;
				}
				else {
					break;
				}

				// 安全限制：最多移除16个字节的填充
				if (aeslen - data_length >= 16) {
					break;
				}
			}

			// 验证解密后的数据长度
			if (data_length <= 16) { // 至少应该包含原始数据+16字节随机填充
				return "";
			}

			// 移除尾部的16字节随机填充
			if (data_length < 16) {
				return "";
			}
			data_length -= 16;

			return std::string(reinterpret_cast<const char*>(outdata.data()), data_length);

		}
		catch (const std::exception& e) {
			return "";
		}
	}
	static std::string base64url_decode_s(const std::string input) {
		std::string base64 = input;

		// 替换URL安全的字符
		for (auto& c : base64) {
			if (c == '-') c = '+';
			if (c == '_') c = '/';
		}

		// 添加填充字符
		switch (base64.size() % 4) {
		case 2: base64 += "=="; break;
		case 3: base64 += "="; break;
		}
		return base64;
	}
	static std::string base64url_encode_s(const std::string input) {
		std::string base64 = input;

		// 替换URL安全的字符
		for (auto& c : base64) {
			if (c == '+') c = '-';
			if (c == '/') c = '_';
		}

		// 删除填充字符
		int len = base64.length();
		while (true) {
			if (base64[len - 1] == '=')
				len--;
			else
				break;
		}
		base64 = base64.substr(0, len);
		return base64;
	}
	static bool split_jwt(const std::string& jwt, std::string& header, std::string& payload, std::string& signature) {
		size_t dot1 = jwt.find('.');
		size_t dot2 = jwt.rfind('.');

		if (dot1 == std::string::npos || dot2 == std::string::npos || dot1 == dot2) {
			return false;
		}

		header = jwt.substr(0, dot1);
		payload = jwt.substr(dot1 + 1, dot2 - dot1 - 1);
		signature = jwt.substr(dot2 + 1);

		return true;
	}

	static std::string convertToPEM(const std::string& key,
		const std::string& header = "-----BEGIN PUBLIC KEY-----",
		const std::string& footer = "-----END PUBLIC KEY-----",
		int lineWidth = 64) {
		std::string pem = header + "\n";

		// 按固定宽度分割密钥
		for (size_t i = 0; i < key.size(); i += lineWidth) {
			pem += key.substr(i, lineWidth) + "\n";
		}

		pem += footer;
		return pem;
	}

	// 将PEM格式转换为单行密钥
	static std::string convertFromPEM(const std::string& pem) {
		std::string key;
		size_t beginPos = pem.find("-----BEGIN");
		size_t endPos = pem.find("-----END");

		if (beginPos == std::string::npos || endPos == std::string::npos) {
			return pem; // 如果不是PEM格式，原样返回
		}

		// 找到第一个换行符后的内容
		size_t keyStart = pem.find('\n', beginPos) + 1;
		size_t keyEnd = pem.rfind('\n', endPos);

		if (keyStart >= keyEnd) {
			return ""; // 无效格式
		}

		std::string keyContent = pem.substr(keyStart, keyEnd - keyStart);

		// 移除所有换行符和空格
		keyContent.erase(std::remove(keyContent.begin(), keyContent.end(), '\n'), keyContent.end());
		keyContent.erase(std::remove(keyContent.begin(), keyContent.end(), '\r'), keyContent.end());
		keyContent.erase(std::remove(keyContent.begin(), keyContent.end(), ' '), keyContent.end());

		return keyContent;
	}
};