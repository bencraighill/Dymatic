#pragma once

#include <stdint.h>
#include <string_view>

// Dymatic server-client packet standard for information interchange
// Note that this is not a standard for the underlying transport protocol (UDP/TCP), just a standard for the application-level protocol
// All messages, usernames and string data are serialized as UTF-8 strings as per the Dymatic standard

enum class PacketType : uint16_t
{
	None = 0, // Invalid packet
	InternalMessage, // Internal message for some Dymatic API
	ScriptMessage, // Message to be passed to the script engine
	ServerShutdown,
	ClientKick
};

