/*
* kinetic-c
* Copyright (C) 2014 Seagate Technology.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*/

#include "unity.h"
#include "unity_helper.h"
#include "protobuf-c/protobuf-c.h"
#include "byte_array.h"
#include "kinetic_types.h"
#include "kinetic_types_internal.h"
#include "kinetic_proto.h"
#include "kinetic_message.h"
#include "kinetic_logger.h"

uint8_t KeyData[1024];
ByteArray Key;
ByteBuffer KeyBuffer;
uint8_t NewVersionData[1024];
ByteArray NewVersion;
ByteBuffer NewVersionBuffer;
uint8_t VersionData[1024];
ByteArray Version;
ByteBuffer VersionBuffer;
uint8_t TagData[1024];
ByteArray Tag;
ByteBuffer TagBuffer;

void setUp(void)
{
    memset(KeyData, 0, sizeof(KeyData));
    Key = ByteArray_Create(KeyData, sizeof(Key));
    KeyBuffer = ByteBuffer_CreateWithArray(Key);
    ByteBuffer_AppendCString(&KeyBuffer, "my_key_3.1415927");
    memset(NewVersionData, 0, sizeof(KeyData));
    NewVersion = ByteArray_Create(NewVersionData, sizeof(NewVersion));
    NewVersionBuffer = ByteBuffer_CreateWithArray(NewVersion);
    ByteBuffer_AppendCString(&NewVersionBuffer, "v2.0");
    memset(VersionData, 0, sizeof(KeyData));
    Version = ByteArray_Create(VersionData, sizeof(Version));
    VersionBuffer = ByteBuffer_CreateWithArray(Version);
    ByteBuffer_AppendCString(&VersionBuffer, "v1.0");
    memset(TagData, 0, sizeof(KeyData));
    Tag = ByteArray_Create(TagData, sizeof(Tag));
    TagBuffer = ByteBuffer_CreateWithArray(Tag);
    ByteBuffer_AppendCString(&TagBuffer, "SomeTagValue");
}

void tearDown(void)
{
}


void test_KineticMessage_Init_should_initialize_the_message_and_required_protobuf_fields(void)
{
    KineticMessage protoMsg;

    KineticMessage_Init(&protoMsg);

    TEST_ASSERT_EQUAL_PTR(&protoMsg.header, protoMsg.command.header);
    TEST_ASSERT_TRUE(protoMsg.message.has_authType);
    TEST_ASSERT_EQUAL(KINETIC_PROTO_MESSAGE_AUTH_TYPE_HMACAUTH, protoMsg.message.authType);
    TEST_ASSERT_EQUAL_PTR(&protoMsg.hmacAuth, protoMsg.message.hmacAuth);
    TEST_ASSERT_EQUAL_PTR(protoMsg.hmacData, protoMsg.message.hmacAuth->hmac.data);
    TEST_ASSERT_EQUAL(KINETIC_HMAC_MAX_LEN, protoMsg.message.hmacAuth->hmac.len);
    TEST_ASSERT_NULL(protoMsg.command.body);
    TEST_ASSERT_NULL(protoMsg.command.status);
}

void test_KineticMessage_ConfigureKeyValue_should_configure_Body_KeyValue_and_add_to_message(void)
{
    KineticMessage message;

    KineticEntry entry = {
        .key = KeyBuffer,
        .newVersion = NewVersionBuffer,
        .dbVersion = VersionBuffer,
        .tag = TagBuffer,
        .algorithm = KINETIC_ALGORITHM_SHA1,
        .metadataOnly = true,
        .force = true,
        .synchronization = KINETIC_SYNCHRONIZATION_WRITETHROUGH,
    };
    memset(&message, 0, sizeof(KineticMessage));
    KineticMessage_Init(&message);

    KineticMessage_ConfigureKeyValue(&message, &entry);

    // Validate that message keyValue and body container are enabled in protobuf
    TEST_ASSERT_EQUAL_PTR(&message.body, message.command.body);
    TEST_ASSERT_EQUAL_PTR(&message.keyValue, message.command.body->keyValue);

    // Validate keyValue fields
    TEST_ASSERT_TRUE(message.keyValue.has_newVersion);
    TEST_ASSERT_ByteArray_EQUALS_ByteBuffer(message.keyValue.newVersion, entry.newVersion);
    TEST_ASSERT_TRUE(message.keyValue.has_key);
    TEST_ASSERT_ByteArray_EQUALS_ByteBuffer(message.keyValue.key, entry.key);
    TEST_ASSERT_TRUE(message.keyValue.has_dbVersion);
    TEST_ASSERT_ByteArray_EQUALS_ByteBuffer(message.keyValue.dbVersion, entry.dbVersion);
    TEST_ASSERT_TRUE(message.keyValue.has_tag);
    TEST_ASSERT_ByteArray_EQUALS_ByteBuffer(message.keyValue.tag, entry.tag);
    TEST_ASSERT_TRUE(message.keyValue.has_algorithm);
    TEST_ASSERT_EQUAL(KINETIC_PROTO_COMMAND_ALGORITHM_SHA1, message.keyValue.algorithm);
    TEST_ASSERT_TRUE(message.keyValue.has_metadataOnly);
    TEST_ASSERT_TRUE(message.keyValue.metadataOnly);
    TEST_ASSERT_TRUE(message.keyValue.has_force);
    TEST_ASSERT_TRUE(message.keyValue.force);
    TEST_ASSERT_TRUE(message.keyValue.has_synchronization);
    TEST_ASSERT_EQUAL(KINETIC_SYNCHRONIZATION_WRITETHROUGH, message.keyValue.synchronization);


    entry = (KineticEntry) {
        .metadataOnly = false
    };
    memset(&message, 0, sizeof(KineticMessage));
    KineticMessage_Init(&message);

    KineticMessage_ConfigureKeyValue(&message, &entry);

    // Validate that message keyValue and body container are enabled in protobuf
    TEST_ASSERT_EQUAL_PTR(&message.body, message.command.body);
    TEST_ASSERT_EQUAL_PTR(&message.keyValue, message.command.body->keyValue);

    // Validate keyValue fields
    TEST_ASSERT_FALSE(message.keyValue.has_newVersion);
    TEST_ASSERT_FALSE(message.keyValue.has_key);
    TEST_ASSERT_FALSE(message.keyValue.has_dbVersion);
    TEST_ASSERT_FALSE(message.keyValue.has_tag);
    TEST_ASSERT_FALSE(message.keyValue.has_algorithm);
    TEST_ASSERT_FALSE(message.keyValue.has_metadataOnly);
    TEST_ASSERT_FALSE(message.keyValue.has_force);
    TEST_ASSERT_FALSE(message.keyValue.has_synchronization);
}

void test_KineticMessage_ConfigureKeyRange_should_add_and_configure_a_KineticProto_KeyRange_to_the_message(void)
{
    KineticMessage message;

    const int numKeysInRange = 4;
    uint8_t startKeyData[32];
    uint8_t endKeyData[32];
    ByteBuffer startKey, endKey;

    startKey = ByteBuffer_Create(startKeyData, sizeof(startKeyData), 0);
    ByteBuffer_AppendCString(&startKey, "key_range_00_00");
    endKey = ByteBuffer_Create(endKeyData, sizeof(endKeyData), 0);
    ByteBuffer_AppendCString(&endKey, "key_range_00_03");

    KineticKeyRange range = {
        .startKey = startKey,
        .endKey = endKey,
        .startKeyInclusive = true,
        .endKeyInclusive = true,
        .maxReturned = numKeysInRange,
        .reverse = true,
    };

    memset(&message, 0, sizeof(KineticMessage));
    KineticMessage_Init(&message);

    KineticMessage_ConfigureKeyRange(&message, &range);

    // Validate that message keyValue and body container are enabled in protobuf
    TEST_ASSERT_EQUAL_PTR(&message.body, message.command.body);
    TEST_ASSERT_EQUAL_PTR(&message.keyRange, message.command.body->range);

    // Validate range fields
    TEST_ASSERT_TRUE(message.command.body->range->has_startKey);
    TEST_ASSERT_EQUAL_PTR(startKey.array.data, message.command.body->range->startKey.data);
    TEST_ASSERT_EQUAL(startKey.bytesUsed, message.command.body->range->startKey.len);
    TEST_ASSERT_TRUE(message.command.body->range->has_endKey);
    TEST_ASSERT_EQUAL_PTR(endKey.array.data, message.command.body->range->endKey.data);
    TEST_ASSERT_EQUAL(endKey.bytesUsed, message.command.body->range->endKey.len);
    TEST_ASSERT_TRUE(message.command.body->range->has_startKeyInclusive);
    TEST_ASSERT_TRUE(message.command.body->range->startKeyInclusive);
    TEST_ASSERT_TRUE(message.command.body->range->has_endKeyInclusive);
    TEST_ASSERT_TRUE(message.command.body->range->endKeyInclusive);
    TEST_ASSERT_TRUE(message.command.body->range->has_maxReturned);
    TEST_ASSERT_EQUAL(numKeysInRange, message.command.body->range->maxReturned);
    TEST_ASSERT_TRUE(message.command.body->range->has_reverse);
    TEST_ASSERT_TRUE(message.command.body->range->reverse);
    TEST_ASSERT_EQUAL(0, message.command.body->range->n_keys);
    TEST_ASSERT_NULL(message.command.body->range->keys);




    range = (KineticKeyRange) {
        .startKey = startKey,
        .endKey = endKey,
        .startKeyInclusive = false,
        .endKeyInclusive = false,
        .maxReturned = 1,
        .reverse = false,
    };

    memset(&message, 0, sizeof(KineticMessage));
    KineticMessage_Init(&message);

    KineticMessage_ConfigureKeyRange(&message, &range);

    // Validate that message keyValue and body container are enabled in protobuf
    TEST_ASSERT_EQUAL_PTR(&message.body, message.command.body);
    TEST_ASSERT_EQUAL_PTR(&message.body, message.command.body);
    TEST_ASSERT_EQUAL_PTR(&message.keyRange, message.command.body->range);

    // Validate range fields
    TEST_ASSERT_TRUE(message.command.body->range->has_startKey);
    TEST_ASSERT_EQUAL_PTR(startKey.array.data, message.command.body->range->startKey.data);
    TEST_ASSERT_EQUAL(startKey.bytesUsed, message.command.body->range->startKey.len);
    TEST_ASSERT_TRUE(message.command.body->range->has_endKey);
    TEST_ASSERT_EQUAL_PTR(endKey.array.data, message.command.body->range->endKey.data);
    TEST_ASSERT_EQUAL(endKey.bytesUsed, message.command.body->range->endKey.len);
    TEST_ASSERT_FALSE(message.command.body->range->has_startKeyInclusive);
    TEST_ASSERT_FALSE(message.command.body->range->has_endKeyInclusive);
    TEST_ASSERT_TRUE(message.command.body->range->has_maxReturned);
    TEST_ASSERT_EQUAL(1, message.command.body->range->maxReturned);
    TEST_ASSERT_FALSE(message.command.body->range->has_reverse);
    TEST_ASSERT_EQUAL(0, message.command.body->range->n_keys);
    TEST_ASSERT_NULL(message.command.body->range->keys);
}
