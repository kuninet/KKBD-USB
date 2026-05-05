#include "test_framework.h"
#include "config.h"
#include <stdint.h>

/* config_decode_baudrate のテスト */

TEST(test_decode_baudrate_9600) {
    EXPECT_EQ(9600, config_decode_baudrate(0x00));
}
TEST(test_decode_baudrate_19200) {
    EXPECT_EQ(19200, config_decode_baudrate(0x01));
}
TEST(test_decode_baudrate_38400) {
    EXPECT_EQ(38400, config_decode_baudrate(0x02));
}
TEST(test_decode_baudrate_115200) {
    EXPECT_EQ(115200, config_decode_baudrate(0x03));
}
TEST(test_decode_baudrate_ignores_upper_bits) {
    /* 上位ビットが汚れていても下位2bitだけ評価（4パターン全網羅） */
    EXPECT_EQ(9600,   config_decode_baudrate(0xFC));  /* lower bits 0b00 */
    EXPECT_EQ(19200,  config_decode_baudrate(0xFD));  /* lower bits 0b01 */
    EXPECT_EQ(38400,  config_decode_baudrate(0xFE));  /* lower bits 0b10 */
    EXPECT_EQ(115200, config_decode_baudrate(0xFF));  /* lower bits 0b11 */
}

/* config_decode_line_ending のテスト */

TEST(test_decode_line_ending_cr) {
    EXPECT_EQ(LINE_ENDING_CR, config_decode_line_ending(0x00));
}
TEST(test_decode_line_ending_lf) {
    EXPECT_EQ(LINE_ENDING_LF, config_decode_line_ending(0x01));
}
TEST(test_decode_line_ending_crlf) {
    EXPECT_EQ(LINE_ENDING_CRLF, config_decode_line_ending(0x02));
}
TEST(test_decode_line_ending_reserved_falls_back_to_cr) {
    /* JP1=SHORT, JP2=SHORT は予約パターン → CR にフォールバック */
    EXPECT_EQ(LINE_ENDING_CR, config_decode_line_ending(0x03));
}
TEST(test_decode_line_ending_ignores_upper_bits) {
    /* 上位ビットが汚れていても下位2bitだけ評価（4パターン全網羅） */
    EXPECT_EQ(LINE_ENDING_CR,   config_decode_line_ending(0xFC));  /* lower bits 0b00 */
    EXPECT_EQ(LINE_ENDING_LF,   config_decode_line_ending(0xFD));  /* lower bits 0b01 */
    EXPECT_EQ(LINE_ENDING_CRLF, config_decode_line_ending(0xFE));  /* lower bits 0b10 */
    EXPECT_EQ(LINE_ENDING_CR,   config_decode_line_ending(0xFF));  /* lower bits 0b11 (予約→CR) */
}

int main(void) {
    RUN_TEST(test_decode_baudrate_9600);
    RUN_TEST(test_decode_baudrate_19200);
    RUN_TEST(test_decode_baudrate_38400);
    RUN_TEST(test_decode_baudrate_115200);
    RUN_TEST(test_decode_baudrate_ignores_upper_bits);

    RUN_TEST(test_decode_line_ending_cr);
    RUN_TEST(test_decode_line_ending_lf);
    RUN_TEST(test_decode_line_ending_crlf);
    RUN_TEST(test_decode_line_ending_reserved_falls_back_to_cr);
    RUN_TEST(test_decode_line_ending_ignores_upper_bits);

    TEST_SUMMARY();
    return 0;
}
