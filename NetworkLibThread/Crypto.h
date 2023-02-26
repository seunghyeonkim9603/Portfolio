#pragma once

namespace NetworkLib
{
	class Crypto final
	{
	public:
		Crypto() = delete;
		~Crypto() = delete;

		static void	Encode(WANPacketHeader* header, char* payload, const unsigned char fixedKey);
		static void	Decode(WANPacketHeader* header, char* payload, const unsigned char fixedKey);
		static BYTE CheckSum(const char* data, const unsigned int len);
	};
}