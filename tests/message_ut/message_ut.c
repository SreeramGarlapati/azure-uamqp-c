// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
#endif
#include "testrunnerswitcher.h"
#include "umock_c.h"

static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void* my_gballoc_realloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

static void my_gballoc_free(void* ptr)
{
    free(ptr);
}

#define ENABLE_MOCKS

#include "azure_c_shared_utility/gballoc.h"
#include "azure_uamqp_c/amqpvalue.h"
#include "azure_uamqp_c/amqp_definitions.h"

#undef ENABLE_MOCKS

#include "azure_uamqp_c/message.h"

static const HEADER_HANDLE test_header = (HEADER_HANDLE)0x4242;
static const HEADER_HANDLE cloned_header = (HEADER_HANDLE)0x4243;
static const delivery_annotations test_delivery_annotations = (delivery_annotations)0x4244;
static const delivery_annotations cloned_delivery_annotations = (delivery_annotations)0x4245;
static const message_annotations test_message_annotations = (message_annotations)0x4244;
static const message_annotations cloned_message_annotations = (message_annotations)0x4245;
static const PROPERTIES_HANDLE test_message_properties = (PROPERTIES_HANDLE)0x4246;
static const PROPERTIES_HANDLE cloned_message_properties = (PROPERTIES_HANDLE)0x4247;
static const AMQP_VALUE test_application_properties = (AMQP_VALUE)0x4248;
static const AMQP_VALUE cloned_application_properties = (AMQP_VALUE)0x4249;
static const annotations test_footer = (annotations)0x4250;
static const annotations cloned_footer = (annotations)0x4251;
static const AMQP_VALUE test_amqp_value = (AMQP_VALUE)0x4252;
static const AMQP_VALUE cloned_amqp_value = (AMQP_VALUE)0x4253;
static const AMQP_VALUE test_sequence_1 = (AMQP_VALUE)0x4254;
static const AMQP_VALUE cloned_sequence_1 = (AMQP_VALUE)0x4255;
static const AMQP_VALUE test_sequence_2 = (AMQP_VALUE)0x4256;
static const AMQP_VALUE cloned_sequence_2 = (AMQP_VALUE)0x4257;

static const HEADER_HANDLE another_test_header = (HEADER_HANDLE)0x4258;

static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

BEGIN_TEST_SUITE(message_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_realloc, my_gballoc_realloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);
	REGISTER_GLOBAL_MOCK_RETURN(header_clone, cloned_header);
	REGISTER_GLOBAL_MOCK_RETURN(annotations_clone, cloned_delivery_annotations);
	REGISTER_GLOBAL_MOCK_RETURN(properties_clone, cloned_message_properties);
    REGISTER_UMOCK_ALIAS_TYPE(HEADER_HANDLE, void*);
	REGISTER_UMOCK_ALIAS_TYPE(AMQP_VALUE, void*);
	REGISTER_UMOCK_ALIAS_TYPE(PROPERTIES_HANDLE, void*);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
    TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

TEST_FUNCTION_INITIALIZE(test_init)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(test_cleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

/* message_create */

/* Tests_SRS_MESSAGE_01_001: [`message_create` shall create a new AMQP message instance and on success it shall return a non-NULL handle for the newly created message instance.] */
TEST_FUNCTION(message_create_succeeds)
{
	// arrange
	MESSAGE_HANDLE message;

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

	// act
	message = message_create();

	// assert
	ASSERT_IS_NOT_NULL(message);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// cleanup
	message_destroy(message);
}

/* Tests_SRS_MESSAGE_01_001: [`message_create` shall create a new AMQP message instance and on success it shall return a non-NULL handle for the newly created message instance.] */
TEST_FUNCTION(message_create_2_times_yields_2_different_message_instances)
{
	// arrange
	MESSAGE_HANDLE message1;
	MESSAGE_HANDLE message2;

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));

	// act
	message1 = message_create();
	message2 = message_create();

	// assert
	ASSERT_IS_NOT_NULL_WITH_MSG(message1, "Creating the first message failed");
	ASSERT_IS_NOT_NULL_WITH_MSG(message2, "Creating the second message failed");
	ASSERT_ARE_NOT_EQUAL(void_ptr, message1, message2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// cleanup
	message_destroy(message1);
	message_destroy(message2);
}

/* Tests_SRS_MESSAGE_01_002: [If allocating memory for the message fails, `message_create` shall fail and return NULL.] */
TEST_FUNCTION(when_allocating_memory_for_the_message_fails_then_message_create_fails)
{
	// arrange
	MESSAGE_HANDLE message;
	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.SetReturn(NULL);

	// act
	message = message_create();

	// assert
	ASSERT_IS_NULL(message);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* message_clone */

/* Tests_SRS_MESSAGE_01_003: [`message_clone` shall clone a message entirely and on success return a non-NULL handle to the cloned message.] */
/* Tests_SRS_MESSAGE_01_005: [If a header exists on the source message it shall be cloned by using `header_clone`.] */
/* Tests_SRS_MESSAGE_01_006: [If delivery annotations exist on the source message they shall be cloned by using `annotations_clone`.] */
/* Tests_SRS_MESSAGE_01_007: [If message annotations exist on the source message they shall be cloned by using `annotations_clone`.] */
/* Tests_SRS_MESSAGE_01_008: [If message properties exist on the source message they shall be cloned by using `properties_clone`.] */
/* Tests_SRS_MESSAGE_01_009: [If application properties exist on the source message they shall be cloned by using `amqpvalue_clone`.] */
/* Tests_SRS_MESSAGE_01_010: [If a footer exists on the source message it shall be cloned by using `annotations_clone`.] */
/* Tests_SRS_MESSAGE_01_011: [If an AMQP data has been set as message body on the source message it shall be cloned by allocating memory for the binary payload.] */
TEST_FUNCTION(message_clone_with_a_valid_argument_succeeds)
{
	/*
	// arrange
	MESSAGE_HANDLE message;
	MESSAGE_HANDLE source_message = message_create();
	unsigned char data_section[2] = { 0x42, 0x43 };
	BINARY_DATA binary_data = { data_section, sizeof(data_section) };

	(void)message_set_header(source_message, test_header);
	STRICT_EXPECTED_CALL(annotations_clone(custom_delivery_annotations))
		.SetReturn(cloned_delivery_annotations);
	(void)message_set_delivery_annotations(source_message, custom_delivery_annotations);
	STRICT_EXPECTED_CALL(annotations_clone(custom_message_annotations))
		.SetReturn(cloned_message_annotations);
	(void)message_set_message_annotations(source_message, custom_message_annotations);
	STRICT_EXPECTED_CALL(amqpvalue_clone(custom_properties))
		.SetReturn(cloned_properties);
	(void)message_set_properties(source_message, custom_properties);
	STRICT_EXPECTED_CALL(amqpvalue_clone(custom_application_properties))
		.SetReturn(cloned_application_properties);
	(void)message_set_application_properties(source_message, custom_application_properties);
	STRICT_EXPECTED_CALL(annotations_clone(custom_footer))
		.SetReturn(cloned_footer);
	(void)message_set_footer(source_message, custom_footer);
	(void)message_add_body_amqp_data(source_message, binary_data);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
	STRICT_EXPECTED_CALL(header_clone(test_header_handle));
	STRICT_EXPECTED_CALL(annotations_clone(cloned_delivery_annotations));
	STRICT_EXPECTED_CALL(annotations_clone(cloned_message_annotations));
	STRICT_EXPECTED_CALL(properties_clone(test_properties_handle));
	STRICT_EXPECTED_CALL(amqpvalue_clone(cloned_application_properties));
	STRICT_EXPECTED_CALL(annotations_clone(cloned_footer));
	EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG));
	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(data_section)));

	// act
	message = message_clone(source_message);

	// assert
	ASSERT_IS_NOT_NULL(message);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// cleanup
	message_destroy(source_message);
	message_destroy(message);*/
}

/* Tests_SRS_MESSAGE_01_062: [If `source_message` is NULL, `message_clone` shall fail and return NULL.] */
TEST_FUNCTION(message_clone_with_NULL_message_source_fails)
{
	// arrange

	// act
	MESSAGE_HANDLE message = message_clone(NULL);

	// assert
	ASSERT_IS_NULL(message);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_MESSAGE_01_004: [If allocating memory for the new cloned message fails, `message_clone` shall fail and return NULL.] */
TEST_FUNCTION(when_allocating_memory_fails_then_message_clone_fails)
{
	// arrange
	MESSAGE_HANDLE message;
	MESSAGE_HANDLE source_message = message_create();
    umock_c_reset_all_calls();

	EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.SetReturn(NULL);

	// act
	message = message_clone(source_message);

	// assert
	ASSERT_IS_NULL(message);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// cleanup
	message_destroy(source_message);
}

/* message_destroy */

/* Tests_SRS_MESSAGE_01_013: [ `message_destroy` shall free all resources allocated by the message instance identified by the `message` argument. ]*/
TEST_FUNCTION(message_destroy_frees_the_allocated_memory)
{
	// arrange
	MESSAGE_HANDLE message = message_create();
	umock_c_reset_all_calls();
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

	// act
	message_destroy(message);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_MESSAGE_01_014: [ If `message` is NULL, `message_destroy` shall do nothing. ]*/
TEST_FUNCTION(message_destroy_with_NULL_does_nothing)
{
	// arrange

	// act
	message_destroy(NULL);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_MESSAGE_01_015: [ The message header shall be freed by calling `header_destroy`. ]*/
TEST_FUNCTION(when_a_header_was_set_it_is_destroyed)
{
	// arrange
	MESSAGE_HANDLE message = message_create();
	umock_c_reset_all_calls();
	STRICT_EXPECTED_CALL(header_clone(test_header));
	(void)message_set_header(message, test_header);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(header_destroy(cloned_header));
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

	// act
	message_destroy(message);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_MESSAGE_01_016: [ The delivery annotations shall be freed by calling `annotations_destroy`. ]*/
TEST_FUNCTION(when_delivery_annotations_were_set_they_are_destroyed)
{
	// arrange
	MESSAGE_HANDLE message = message_create();
	umock_c_reset_all_calls();
	STRICT_EXPECTED_CALL(amqpvalue_clone(test_delivery_annotations))
		.SetReturn(cloned_delivery_annotations);
	(void)message_set_delivery_annotations(message, test_delivery_annotations);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(amqpvalue_destroy(cloned_delivery_annotations));
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

	// act
	message_destroy(message);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_MESSAGE_01_017: [ The message annotations shall be freed by calling `annotations_destroy`. ]*/
TEST_FUNCTION(when_message_annotations_were_set_they_are_destroyed)
{
	// arrange
	MESSAGE_HANDLE message = message_create();
	umock_c_reset_all_calls();
	STRICT_EXPECTED_CALL(amqpvalue_clone(test_message_annotations))
		.SetReturn(cloned_message_annotations);
	(void)message_set_message_annotations(message, test_message_annotations);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(amqpvalue_destroy(cloned_message_annotations));
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

	// act
	message_destroy(message);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_MESSAGE_01_018: [ The message properties shall be freed by calling `properties_destroy`. ]*/
TEST_FUNCTION(when_message_properties_were_set_they_are_destroyed)
{
	// arrange
	MESSAGE_HANDLE message = message_create();
	umock_c_reset_all_calls();
	STRICT_EXPECTED_CALL(properties_clone(test_message_properties))
		.SetReturn(cloned_message_properties);
	(void)message_set_properties(message, test_message_properties);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(properties_destroy(cloned_message_properties));
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

	// act
	message_destroy(message);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_MESSAGE_01_019: [ The application properties shall be freed by calling `amqpvalue_destroy`. ]*/
TEST_FUNCTION(when_application_properties_were_set_they_are_destroyed)
{
	// arrange
	MESSAGE_HANDLE message = message_create();
	umock_c_reset_all_calls();
	STRICT_EXPECTED_CALL(amqpvalue_clone(test_application_properties))
		.SetReturn(cloned_application_properties);
	(void)message_set_application_properties(message, test_application_properties);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(amqpvalue_destroy(cloned_application_properties));
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

	// act
	message_destroy(message);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_MESSAGE_01_020: [ The message footer shall be freed by calling `annotations_destroy`. ]*/
TEST_FUNCTION(when_message_footer_was_set_it_is_destroyed)
{
	// arrange
	MESSAGE_HANDLE message = message_create();
	umock_c_reset_all_calls();
	STRICT_EXPECTED_CALL(annotations_clone(test_footer))
		.SetReturn(cloned_footer);
	(void)message_set_footer(message, test_footer);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(amqpvalue_destroy(cloned_footer));
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

	// act
	message_destroy(message);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_MESSAGE_01_021: [ If the message body is made of an AMQP value, the value shall be freed by calling `amqpvalue_destroy`. ]*/
TEST_FUNCTION(when_an_AMQP_value_is_set_as_body_message_destroy_frees_it)
{
	// arrange
	MESSAGE_HANDLE message = message_create();
	umock_c_reset_all_calls();
	STRICT_EXPECTED_CALL(amqpvalue_clone(test_amqp_value))
		.SetReturn(cloned_amqp_value);
	(void)message_set_body_amqp_value(message, test_amqp_value);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(amqpvalue_destroy(cloned_amqp_value));
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

	// act
	message_destroy(message);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_MESSAGE_01_136: [ If the message body is made of several AMQP data items, they shall all be freed. ]*/
TEST_FUNCTION(when_an_AMQP_data_is_set_as_body_message_destroy_frees_it)
{
	// arrange
	MESSAGE_HANDLE message = message_create();
	unsigned char data_bytes_1[] = { 0x42 };
	BINARY_DATA binary_data_1;
	binary_data_1.bytes = data_bytes_1;
	binary_data_1.length = sizeof(data_bytes_1);
	umock_c_reset_all_calls();
	(void)message_add_body_amqp_data(message, binary_data_1);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

	// act
	message_destroy(message);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_MESSAGE_01_136: [ If the message body is made of several AMQP data items, they shall all be freed. ]*/
TEST_FUNCTION(when_two_AMQP_data_items_are_set_as_body_message_destroy_frees_them)
{
	// arrange
	MESSAGE_HANDLE message = message_create();
	unsigned char data_bytes_1[] = { 0x42 };
	BINARY_DATA binary_data_1;
	unsigned char data_bytes_2[] = { 0x43 };
	BINARY_DATA binary_data_2;
	binary_data_1.bytes = data_bytes_1;
	binary_data_1.length = sizeof(data_bytes_1);
	binary_data_2.bytes = data_bytes_2;
	binary_data_2.length = sizeof(data_bytes_2);
	umock_c_reset_all_calls();
	(void)message_add_body_amqp_data(message, binary_data_1);
	(void)message_add_body_amqp_data(message, binary_data_2);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

	// act
	message_destroy(message);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_MESSAGE_01_136: [ If the message body is made of several AMQP sequences, they shall all be freed. ]*/
/* Tests_SRS_MESSAGE_01_137: [ Each sequence shall be freed by calling `amqpvalue_destroy`. ]*/
TEST_FUNCTION(when_one_AMQP_sequence_is_set_as_body_message_destroy_frees_it)
{
	// arrange
	MESSAGE_HANDLE message = message_create();
	umock_c_reset_all_calls();
	STRICT_EXPECTED_CALL(amqpvalue_clone(test_sequence_1))
		.SetReturn(cloned_sequence_1);
	(void)message_add_body_amqp_sequence(message, test_sequence_1);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(amqpvalue_destroy(cloned_sequence_1));
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

	// act
	message_destroy(message);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_MESSAGE_01_136: [ If the message body is made of several AMQP sequences, they shall all be freed. ]*/
/* Tests_SRS_MESSAGE_01_137: [ Each sequence shall be freed by calling `amqpvalue_destroy`. ]*/
TEST_FUNCTION(when_two_AMQP_sequences_are_set_as_body_message_destroy_frees_them)
{
	// arrange
	MESSAGE_HANDLE message = message_create();
	umock_c_reset_all_calls();
	STRICT_EXPECTED_CALL(gballoc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
	STRICT_EXPECTED_CALL(amqpvalue_clone(test_sequence_1))
		.SetReturn(cloned_sequence_1);
	STRICT_EXPECTED_CALL(gballoc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
	STRICT_EXPECTED_CALL(amqpvalue_clone(test_sequence_2))
		.SetReturn(cloned_sequence_2);
	(void)message_add_body_amqp_sequence(message, test_sequence_1);
	(void)message_add_body_amqp_sequence(message, test_sequence_2);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(amqpvalue_destroy(cloned_sequence_1));
	STRICT_EXPECTED_CALL(amqpvalue_destroy(cloned_sequence_2));
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

	// act
	message_destroy(message);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_MESSAGE_01_015: [ The message header shall be freed by calling `header_destroy`. ]*/
/* Tests_SRS_MESSAGE_01_016: [ The delivery annotations shall be freed by calling `annotations_destroy`. ]*/
/* Tests_SRS_MESSAGE_01_017: [ The message annotations shall be freed by calling `annotations_destroy`. ]*/
/* Tests_SRS_MESSAGE_01_018: [ The message properties shall be freed by calling `properties_destroy`. ]*/
/* Tests_SRS_MESSAGE_01_019: [ The application properties shall be freed by calling `amqpvalue_destroy`. ]*/
/* Tests_SRS_MESSAGE_01_020: [ The message footer shall be freed by calling `annotations_destroy`. ]*/
/* Tests_SRS_MESSAGE_01_136: [ If the message body is made of several AMQP sequences, they shall all be freed. ]*/
/* Tests_SRS_MESSAGE_01_137: [ Each sequence shall be freed by calling `amqpvalue_destroy`. ]*/
TEST_FUNCTION(when_all_message_sections_are_set_and_seuqnces_are_used_then_they_are_all_destroyed)
{
	// arrange
	MESSAGE_HANDLE message = message_create();
	umock_c_reset_all_calls();
	STRICT_EXPECTED_CALL(annotations_clone(test_footer))
		.SetReturn(cloned_footer);
	(void)message_set_footer(message, test_footer);
	STRICT_EXPECTED_CALL(amqpvalue_clone(test_application_properties))
		.SetReturn(cloned_application_properties);
	(void)message_set_application_properties(message, test_application_properties);
	STRICT_EXPECTED_CALL(properties_clone(test_message_properties))
		.SetReturn(cloned_message_properties);
	(void)message_set_properties(message, test_message_properties);
	STRICT_EXPECTED_CALL(amqpvalue_clone(test_message_annotations))
		.SetReturn(cloned_message_annotations);
	(void)message_set_message_annotations(message, test_message_annotations);
	STRICT_EXPECTED_CALL(amqpvalue_clone(test_delivery_annotations))
		.SetReturn(cloned_delivery_annotations);
	(void)message_set_delivery_annotations(message, test_delivery_annotations);
	STRICT_EXPECTED_CALL(header_clone(test_header));
	(void)message_set_header(message, test_header);
	STRICT_EXPECTED_CALL(gballoc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
	STRICT_EXPECTED_CALL(amqpvalue_clone(test_sequence_1))
		.SetReturn(cloned_sequence_1);
	STRICT_EXPECTED_CALL(gballoc_realloc(IGNORED_PTR_ARG, IGNORED_NUM_ARG));
	STRICT_EXPECTED_CALL(amqpvalue_clone(test_sequence_2))
		.SetReturn(cloned_sequence_2);
	(void)message_add_body_amqp_sequence(message, test_sequence_1);
	(void)message_add_body_amqp_sequence(message, test_sequence_2);
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(header_destroy(cloned_header));
	STRICT_EXPECTED_CALL(amqpvalue_destroy(cloned_delivery_annotations));
	STRICT_EXPECTED_CALL(amqpvalue_destroy(cloned_message_annotations));
	STRICT_EXPECTED_CALL(properties_destroy(cloned_message_properties));
	STRICT_EXPECTED_CALL(amqpvalue_destroy(cloned_application_properties));
	STRICT_EXPECTED_CALL(amqpvalue_destroy(cloned_footer));
	STRICT_EXPECTED_CALL(amqpvalue_destroy(cloned_sequence_1));
	STRICT_EXPECTED_CALL(amqpvalue_destroy(cloned_sequence_2));
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));
	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG));

	// act
	message_destroy(message);

	// assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* message_set_header */

/* Tests_SRS_MESSAGE_01_022: [ `message_set_header` shall copy the contents of `message_header` as the header for the message instance identified by message. ]*/
/* Tests_SRS_MESSAGE_01_023: [ On success it shall return 0. ]*/
/* Tests_SRS_MESSAGE_01_025: [ Cloning the header shall be done by calling `header_clone`. ]*/
TEST_FUNCTION(message_set_header_copies_the_header)
{
	// arrange
	int result;
	MESSAGE_HANDLE message = message_create();
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(header_clone(test_header));

	// act
	result = message_set_header(message, test_header);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// cleanup
	message_destroy(message);
}

/* Tests_SRS_MESSAGE_01_024: [ If `message` is NULL, `message_set_header` shall fail and return a non-zero value. ]*/
TEST_FUNCTION(message_set_header_with_NULL_message_fails)
{
	// arrange
	int result;

	// act
	result = message_set_header(NULL, test_header);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_MESSAGE_01_024: [ If `message` is NULL, `message_set_header` shall fail and return a non-zero value. ]*/
TEST_FUNCTION(message_set_header_with_NULL_is_allowed)
{
	// arrange
	int result;
	MESSAGE_HANDLE message = message_create();
	umock_c_reset_all_calls();

	// act
	result = message_set_header(message, NULL);

	// assert
	ASSERT_ARE_EQUAL(int, 0, result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// cleanup
	message_destroy(message);
}

/* Tests_SRS_MESSAGE_01_026: [ If `header_clone` fails, `message_set_header` shall fail and return a non-zero value. ]*/
TEST_FUNCTION(when_header_clone_fails_message_set_header_fails)
{
	// arrange
	int result;
	MESSAGE_HANDLE message = message_create();
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(header_clone(test_header))
		.SetReturn(NULL);

	// act
	result = message_set_header(message, test_header);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// cleanup
	message_destroy(message);
}

/* Tests_SRS_MESSAGE_01_138: [ If setting the header fails, the previous value shall be preserved. ]*/
TEST_FUNCTION(when_header_clone_fails_previous_header_is_kept)
{
	// arrange
	HEADER_HANDLE result_header;
	int result;
	MESSAGE_HANDLE message = message_create();
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(header_clone(test_header))
		.SetReturn(cloned_header);
	(void)message_set_header(message, test_header);
	STRICT_EXPECTED_CALL(header_clone(another_test_header))
		.SetReturn(NULL);
	(void)message_set_header(message, another_test_header);
	STRICT_EXPECTED_CALL(header_clone(cloned_header))
		.SetReturn(cloned_header);

	// act
	result = message_get_header(message, &result_header);

	// assert
	ASSERT_ARE_EQUAL(void_ptr, cloned_header, result_header);
	ASSERT_ARE_EQUAL(int, 0, result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// cleanup
	message_destroy(message);
}

/* Tests_SRS_MESSAGE_01_139: [ If `message_header` is NULL, the previously stored header associated with `message` shall be freed. ]*/
TEST_FUNCTION(when_setting_a_NULL_header_previous_header_is_freed)
{
	// arrange
	HEADER_HANDLE result_header;
	int result;
	MESSAGE_HANDLE message = message_create();
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(header_clone(test_header))
		.SetReturn(cloned_header);
	(void)message_set_header(message, test_header);
	STRICT_EXPECTED_CALL(header_destroy(cloned_header));
	(void)message_set_header(message, NULL);

	// act
	result = message_get_header(message, &result_header);

	// assert
	ASSERT_IS_NULL(result_header);
	ASSERT_ARE_EQUAL(int, 0, result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// cleanup
	message_destroy(message);
}

/* message_get_header */

/* Tests_SRS_MESSAGE_01_027: [ `message_get_header` shall copy the contents of header for the message instance identified by `message` into the argument `message_header`. ]*/
/* Tests_SRS_MESSAGE_01_028: [ On success, `message_get_header` shall return 0.]*/
/* Tests_SRS_MESSAGE_01_030: [ Cloning the header shall be done by calling `header_clone`. ]*/
TEST_FUNCTION(message_get_header_gets_the_value)
{
	// arrange
	HEADER_HANDLE expected_header = (HEADER_HANDLE)0x5678;
	HEADER_HANDLE result_header;
	int result;
	MESSAGE_HANDLE message = message_create();
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(header_clone(test_header))
		.SetReturn(cloned_header);
	(void)message_set_header(message, test_header);

	STRICT_EXPECTED_CALL(header_clone(cloned_header))
		.SetReturn(expected_header);

	// act
	result = message_get_header(message, &result_header);

	// assert
	ASSERT_ARE_EQUAL(void_ptr, expected_header, result_header);
	ASSERT_ARE_EQUAL(int, 0, result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// cleanup
	message_destroy(message);
}

/* Tests_SRS_MESSAGE_01_027: [ `message_get_header` shall copy the contents of header for the message instance identified by `message` into the argument `message_header`. ]*/
/* Tests_SRS_MESSAGE_01_028: [ On success, `message_get_header` shall return 0.]*/
TEST_FUNCTION(message_get_header_when_no_header_was_set_yields_NULL)
{
	// arrange
	HEADER_HANDLE result_header;
	int result;
	MESSAGE_HANDLE message = message_create();
	umock_c_reset_all_calls();

	// act
	result = message_get_header(message, &result_header);

	// assert
	ASSERT_IS_NULL(result_header);
	ASSERT_ARE_EQUAL(int, 0, result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// cleanup
	message_destroy(message);
}

/* Tests_SRS_MESSAGE_01_029: [ If `message` or `message_header` is NULL, `message_get_header` shall fail and return a non-zero value. ]*/
TEST_FUNCTION(message_get_header_with_NULL_message_header_fails)
{
	// arrange
	int result;
	MESSAGE_HANDLE message = message_create();
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(header_clone(test_header))
		.SetReturn(cloned_header);
	(void)message_set_header(message, test_header);

	// act
	result = message_get_header(message, NULL);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// cleanup
	message_destroy(message);
}

/* Tests_SRS_MESSAGE_01_029: [ If `message` or `message_header` is NULL, `message_get_header` shall fail and return a non-zero value. ]*/
TEST_FUNCTION(message_get_header_with_NULL_message_fails)
{
	// arrange
	int result;
	HEADER_HANDLE result_header;

	// act
	result = message_get_header(NULL, &result_header);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_MESSAGE_01_031: [ If `header_clone` fails, `message_get_header` shall fail and return a non-zero value. ]*/
TEST_FUNCTION(when_header_clone_fails_message_get_header_fails)
{
	// arrange
	HEADER_HANDLE result_header;
	int result;
	MESSAGE_HANDLE message = message_create();
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(header_clone(test_header))
		.SetReturn(cloned_header);
	(void)message_set_header(message, test_header);

	STRICT_EXPECTED_CALL(header_clone(cloned_header))
		.SetReturn(NULL);

	// act
	result = message_get_header(message, &result_header);

	// assert
	ASSERT_ARE_NOT_EQUAL(int, 0, result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	// cleanup
	message_destroy(message);
}

END_TEST_SUITE(message_ut)
