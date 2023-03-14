#include "pch.h"

namespace NetworkLib
{
	void Crypto::Encode(WANPacketHeader* header, char* payload, const unsigned char fixedKey)
	{
		unsigned char encodeKey = 0;
		unsigned char encoded = 0;

		unsigned char randKey = header->RandKey;

		encodeKey = header->CheckSum ^ (randKey + 1);
		encoded = encodeKey ^ (fixedKey + 1);

		header->CheckSum = encoded;

		for (INT16 i = 0; i < header->Length; ++i)
		{
			encodeKey = payload[i] ^ (randKey + encodeKey + i + 2);
			encoded = encodeKey ^ (fixedKey + encoded + i + 2);

			payload[i] = encoded;
		}
	}

	void  Crypto::Decode(WANPacketHeader* header, char* payload, const unsigned char fixedKey)
	{
		unsigned char preDecodeKey;
		unsigned char preEncoded;

		unsigned char curDecodeKey;
		unsigned char curEncoded;

		unsigned char randKey = header->RandKey;

		preEncoded = header->CheckSum;

		preDecodeKey = preEncoded ^ (fixedKey + 1);
		header->CheckSum = preDecodeKey ^ (randKey + 1);

		for (INT16 i = 0; i < header->Length; ++i)
		{
			curEncoded = payload[i];
			curDecodeKey = curEncoded ^ (preEncoded + fixedKey + i + 2);
			payload[i] = curDecodeKey ^ (preDecodeKey + randKey + i + 2);

			preEncoded = curEncoded;
			preDecodeKey = curDecodeKey;
		}
	}

	BYTE  Crypto::CheckSum(const char* data, const unsigned int len)
	{
		BYTE checkSum = 0;

		for (unsigned int i = 0; i < len; ++i)
		{
			checkSum += data[i];
		}
		return checkSum;
	}
}
