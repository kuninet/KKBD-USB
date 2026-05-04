#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

/*
 * 使用方針:
 * - 1 .cファイル = 1 実行バイナリの構成を前提とする。
 * - 複数の test_xxx.c を 1 バイナリにリンクする場合は、static 変数の重複を避けるため
 *   このヘッダをひとつの .c でのみインクルードし、他は extern 宣言で参照すること。
 *   またはこのヘッダをリファクタして実体定義を別の .c に移すこと。
 */

/*
 * 命名: 失敗しても処理を継続する Googletest の EXPECT_* 慣例に従う。
 * 失敗後の続行でクラッシュしうる場合は、呼び出し側で
 *   if (g_current_failed) return;
 * のガードを入れること。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static int g_test_pass = 0;
static int g_test_fail = 0;
static const char *g_current_test = "";
static bool g_current_failed = false;

#define TEST(name) static void name(void)

#define EXPECT_EQ(expected, actual) do { \
    long _e = (long)(expected); \
    long _a = (long)(actual); \
    if (_a != _e) { \
        fprintf(stderr, "  [FAIL] %s:%d: %s == %s -> got %ld, expected %ld\n", \
                __FILE__, __LINE__, #actual, #expected, _a, _e); \
        g_current_failed = true; \
    } \
} while (0)

#define EXPECT_TRUE(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "  [FAIL] %s:%d: %s should be true\n", \
                __FILE__, __LINE__, #cond); \
        g_current_failed = true; \
    } \
} while (0)

#define EXPECT_FALSE(cond) do { \
    if ((cond)) { \
        fprintf(stderr, "  [FAIL] %s:%d: %s should be false\n", \
                __FILE__, __LINE__, #cond); \
        g_current_failed = true; \
    } \
} while (0)

#define EXPECT_STR_EQ(a, b) do { \
    if (strcmp((a), (b)) != 0) { \
        fprintf(stderr, "  [FAIL] %s:%d: \"%s\" != \"%s\"\n", \
                __FILE__, __LINE__, (a), (b)); \
        g_current_failed = true; \
    } \
} while (0)

#define RUN_TEST(name) do { \
    g_current_test = #name; \
    g_current_failed = false; \
    printf("[ RUN  ] %s\n", g_current_test); \
    name(); \
    if (g_current_failed) { \
        g_test_fail++; \
        printf("[ FAIL ] %s\n", g_current_test); \
    } else { \
        g_test_pass++; \
        printf("[  OK  ] %s\n", g_current_test); \
    } \
} while (0)

#define TEST_SUMMARY() do { \
    int total = g_test_pass + g_test_fail; \
    printf("\n========================================\n"); \
    printf("Tests: %d passed, %d failed, %d total\n", \
           g_test_pass, g_test_fail, total); \
    printf("========================================\n"); \
    if (g_test_fail > 0) exit(1); \
} while (0)

#endif /* TEST_FRAMEWORK_H */
