#pragma once

namespace NetworkLib
{
#pragma pack(push, 1)
	struct WANPacketHeader
	{
		BYTE		Code;
		INT16		Length;
		BYTE		RandKey;
		BYTE		CheckSum;
	};
	static_assert(sizeof(WANPacketHeader) == 5, "Invalid WAN Server PacketHeader size");
	static_assert(offsetof(WANPacketHeader, Code) == 0, "Offset of Code is not Valid");
	static_assert(offsetof(WANPacketHeader, Length) == 1, "Offset of Length is not Valid");
	static_assert(offsetof(WANPacketHeader, RandKey) == 3, "Offset of RandKey is not Valid");
	static_assert(offsetof(WANPacketHeader, CheckSum) == 4, "Offset of CheckSum is not Valid");

	struct LANPacketHeader
	{
		INT16		Length;
	};
	static_assert(sizeof(LANPacketHeader) == 2, "Invalid LAN Server PacketHeader size");
	static_assert(offsetof(LANPacketHeader, Length) == 0, "Offset of Length is not Valid");

#pragma pack(pop)
}