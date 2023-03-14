#pragma once

namespace NetworkLib
{
	class INetworkClientEventListener
	{
	public:
		virtual void OnError(const int errorCode, const wchar_t* message) = 0;
		virtual void OnRecv(Message* message) = 0;
		virtual void OnConnect() = 0;
		virtual void OnDisconnect() = 0;
	};
}