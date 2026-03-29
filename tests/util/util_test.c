#include <font.h>
#include <util.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>


void test_truncate_uint_to_suffixed_str()
{
    /*
     * I want to avoid testing the rounding so it can be easily changed
     * so all tests are numbers that are rounded down regardless of rounding method.
     * That way the function can be modified to round to nearest integer easily.
     */

    char suffixed_str_buff[UINT_MAX_DIGITS + 1] = {'\0'};

    truncate_uint_to_suffixed_str(100, 3, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "100") == 0);

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(1000, 3, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "1K") == 0);
    
    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(1000, 2, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "1K") == 0);

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(1000, 1, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "1K") == 0);

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(1000, 0, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "1K") == 0);

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(0, 0, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "0") == 0);

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(100, 2, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "100") == 0);

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(100, 1, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "100") == 0);

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(123, 1, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "123") == 0);

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(1234, 3, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "1" FP2_STR "K") == 0); // "1.2K"

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(1600, 3, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "1" FP6_STR "K") == 0); // "1.6K"


    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(1000, 4, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "1000") == 0);

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(1000, 5, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "1000") == 0);

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(12123, 4, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "12" FP1_STR "K") == 0);   // "12.1K"

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(123123, 4, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "123K") == 0);

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(123123, 5, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "123" FP1_STR "K") == 0);  // "123.1K"

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(123123, 6, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "123123") == 0);

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(123123, 7, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "123123") == 0);

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(12345123, 6, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "12" FP3_STR "45M") == 0); // "12.345M"

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(12123123, 5, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "12" FP1_STR "2M") == 0); // "12.12M"

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(1008000, 5, suffixed_str_buff);

    assert(strcmp(suffixed_str_buff, "1" FP0_STR "08M") == 0); // "1.008M"

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(3029000, 5, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "3" FP0_STR "29M") == 0); // "3.029M"

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(10007000, 5, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "10M") == 0); // Decimal point fully truncated

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(10005123, 6, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "10" FP0_STR "05M") == 0); // "10.005M"

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(12123123, 4, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "12" FP1_STR "M") == 0); // "12.1M"

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(54123123, 4, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "54" FP1_STR "M") == 0); // "54.1M"

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(123123123, 4, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "123M") == 0);

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(123123123, 6, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "123" FP1_STR "2M") == 0);  // "123.12M"

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(987123123, 6, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "987" FP1_STR "2M") == 0);  // "987.12M"

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(123123123, 7, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "123" FP1_STR "23M") == 0); // "123.123M"

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(123123123, 8, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "123" FP1_STR "231M") == 0); // "123.1231M"

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(123123123, 9, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "123123123") == 0);

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(1123123123, 4, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "1" FP1_STR "2B") == 0); // "1.12B"

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str((uint32_t)3012012012, 4, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "3" FP0_STR "1B") == 0); // "3.01B"

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(1234123123, 5, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "1" FP2_STR "34B") == 0); // "1.234B"

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(1000512345, 6, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "1" FP0_STR "005B") == 0); // "1.0005B"

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(1234561234, 7, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "1" FP2_STR "3456B") == 0); // "1.23456B"

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(1000061234, 7, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "1" FP0_STR "0006B") == 0); // "1.00006B"
    
    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(1234567123, 8, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "1" FP2_STR "34567B") == 0); // "1.234567B"

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(1000007123, 8, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "1" FP0_STR "00007B") == 0); // "1.000007B"

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(1234567812, 9, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "1" FP2_STR "345678B") == 0); // "1.2345678B"

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(1000000812, 9, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "1" FP0_STR "000008B") == 0); // "1.0000008B"

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(1123123123, 10, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "1123123123") == 0);

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(1123123123, UINT_MAX_DIGITS, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "1123123123") == 0);

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(1123123123, 100, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "1123123123") == 0);

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(UINT32_MAX, 4, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "4" FP2_STR "9B") == 0); // "4.29B"

    // This is one of the few tests that checks rounding down, try not to add many more
    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(UINT32_MAX, 5, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, "4" FP2_STR "94B") == 0); // "4.294B"

    char max_uint_str_buff[UINT_MAX_DIGITS + 1] = {'\0'};
    snprintf(max_uint_str_buff, sizeof(max_uint_str_buff), "%lu", UINT32_MAX);

    suffixed_str_buff[0] = '\0';
    truncate_uint_to_suffixed_str(UINT32_MAX, UINT_MAX_DIGITS, suffixed_str_buff);
    assert(strcmp(suffixed_str_buff, max_uint_str_buff) == 0);
}

int main()
{
    test_truncate_uint_to_suffixed_str();
    return 0;
}