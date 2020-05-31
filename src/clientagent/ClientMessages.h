#pragma once

#define CLIENT_HELLO 100
#define CLIENT_HELLO_RESP 102
#define CLIENT_DISCONNECT 110
#define CLIENT_EJECT 120
#define CLIENT_HEARTBEAT 130
#define CLIENT_OBJECT_DISABLE 140
#define CLIENT_OBJECT_DISABLE_OWNER 142
#define CLIENT_OBJECT_DELETE 144
#define CLIENT_OBJECT_SET_FIELD 150
#define CLIENT_OBJECT_SET_FIELDS 152
#define CLIENT_OBJECT_LOCATION 160
#define CLIENT_ENTER_OBJECT_REQUIRED 170
#define CLIENT_ENTER_OBJECT_REQUIRED_OTHER 172
#define CLIENT_ENTER_OBJECT_REQUIRED_OTHER_OWNER 174
#define CLIENT_ADD_INTEREST 180
#define CLIENT_ADD_INTEREST_MULTIPLE 182
#define CLIENT_ADD_INTEREST_OBJECTS 184
#define CLIENT_REMOVE_INTEREST 186
#define CLIENT_DONE_INTEREST_RESP 188

#define CLIENT_DISCONNECT_GENERIC 100
#define CLIENT_DISCONNECT_OVERSIZED_DATAGRAM 110
#define CLIENT_DISCONNECT_NO_HELLO 120
#define CLIENT_DISCONNECT_NO_HEARTBEAT 130
#define CLIENT_DISCONNECT_INVALID_MSGTYPE 140
#define CLIENT_DISCONNECT_TRUNCATED_DATAGRAM 150
#define CLIENT_DISCONNECT_ANONYMOUS_VIOLATION 160
#define CLIENT_DISCONNECT_FORBIDDEN_INTEREST 170
#define CLIENT_DISCONNECT_MISSING_OBJECT 180
#define CLIENT_DISCONNECT_FORBIDDEN_FIELD 190
#define CLIENT_DISCONNECT_FORBIDDEN_RELOCATE 200
#define CLIENT_DISCONNECT_BAD_VERSION 210
#define CLIENT_DISCONNECT_BAD_DCHASH 220
#define CLIENT_DISCONNECT_AI_DISCONNECT 230
#define CLIENT_DISCONNECT_FIELD_CONSTRAINT 240
#define CLIENT_DISCONNECT_SESSION_OBJECT_DELETED 250