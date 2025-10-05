#ifndef DISCORD_ASM_OPCODES_H
#define DISCORD_ASM_OPCODES_H

// Discord Gateway Opcodes
// Reference: https://discord.com/developers/docs/topics/opcodes-and-status-codes

#define DISCORD_OP_DISPATCH         0   // Receive: Event dispatch
#define DISCORD_OP_HEARTBEAT        1   // Send/Receive: Heartbeat
#define DISCORD_OP_IDENTIFY         2   // Send: Identify
#define DISCORD_OP_PRESENCE_UPDATE  3   // Send: Presence update
#define DISCORD_OP_VOICE_STATE      4   // Send: Voice state update
#define DISCORD_OP_RESUME           6   // Send: Resume
#define DISCORD_OP_RECONNECT        7   // Receive: Reconnect
#define DISCORD_OP_REQUEST_MEMBERS  8   // Send: Request guild members
#define DISCORD_OP_INVALID_SESSION  9   // Receive: Invalid session
#define DISCORD_OP_HELLO           10   // Receive: Hello
#define DISCORD_OP_HEARTBEAT_ACK   11   // Receive: Heartbeat ACK

// Close Codes
#define DISCORD_CLOSE_NORMAL        1000  // Normal closure
#define DISCORD_CLOSE_UNKNOWN_ERROR 4000  // Unknown error
#define DISCORD_CLOSE_UNKNOWN_OPCODE 4001 // Unknown opcode
#define DISCORD_CLOSE_DECODE_ERROR  4002  // Decode error
#define DISCORD_CLOSE_NOT_AUTH      4003  // Not authenticated
#define DISCORD_CLOSE_AUTH_FAILED   4004  // Authentication failed
#define DISCORD_CLOSE_ALREADY_AUTH  4005  // Already authenticated
#define DISCORD_CLOSE_INVALID_SEQ   4007  // Invalid sequence
#define DISCORD_CLOSE_RATE_LIMITED  4008  // Rate limited
#define DISCORD_CLOSE_SESSION_TIMEOUT 4009 // Session timed out
#define DISCORD_CLOSE_INVALID_SHARD 4010  // Invalid shard
#define DISCORD_CLOSE_SHARDING_REQ  4011  // Sharding required
#define DISCORD_CLOSE_INVALID_VERSION 4012 // Invalid API version
#define DISCORD_CLOSE_INVALID_INTENTS 4013 // Invalid intents
#define DISCORD_CLOSE_DISALLOWED_INTENTS 4014 // Disallowed intents

// Default Gateway version
#define DISCORD_GATEWAY_VERSION     10

// Default intents for basic bot functionality
#define DISCORD_INTENT_GUILDS                    (1 << 0)
#define DISCORD_INTENT_GUILD_MEMBERS             (1 << 1)
#define DISCORD_INTENT_GUILD_MODERATION          (1 << 2)
#define DISCORD_INTENT_GUILD_EMOJIS              (1 << 3)
#define DISCORD_INTENT_GUILD_INTEGRATIONS        (1 << 4)
#define DISCORD_INTENT_GUILD_WEBHOOKS            (1 << 5)
#define DISCORD_INTENT_GUILD_INVITES             (1 << 6)
#define DISCORD_INTENT_GUILD_VOICE_STATES        (1 << 7)
#define DISCORD_INTENT_GUILD_PRESENCES           (1 << 8)  // Privileged
#define DISCORD_INTENT_GUILD_MESSAGES            (1 << 9)
#define DISCORD_INTENT_GUILD_MESSAGE_REACTIONS   (1 << 10)
#define DISCORD_INTENT_GUILD_MESSAGE_TYPING      (1 << 11)
#define DISCORD_INTENT_DIRECT_MESSAGES           (1 << 12)
#define DISCORD_INTENT_DIRECT_MESSAGE_REACTIONS  (1 << 13)
#define DISCORD_INTENT_DIRECT_MESSAGE_TYPING     (1 << 14)
#define DISCORD_INTENT_MESSAGE_CONTENT           (1 << 15) // Privileged
#define DISCORD_INTENT_GUILD_SCHEDULED_EVENTS    (1 << 16)

// Default safe intents (non-privileged)
#define DISCORD_DEFAULT_INTENTS (DISCORD_INTENT_GUILDS | \
                                DISCORD_INTENT_GUILD_MESSAGES | \
                                DISCORD_INTENT_DIRECT_MESSAGES)

#endif // DISCORD_ASM_OPCODES_H