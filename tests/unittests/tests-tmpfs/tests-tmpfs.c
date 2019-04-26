/*
 * Copyright (C) 2018 OTA keys S.A.
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 *
 * @file
 */
#include "fs/tmpfs.h"
#include "vfs.h"

#include <fcntl.h>
#include <errno.h>

#include "embUnit/embUnit.h"

#include "tests-tmpfs.h"

static tmpfs_t tmpfs_desc = {
    .lock = MUTEX_INIT,
};

static vfs_mount_t _test_tmpfs_mount = {
    .fs = &tmpfs_file_system,
    .mount_point = "/tmp",
    .private_data = &tmpfs_desc,
};

static void tests_tmpfs_mount_umount(void)
{
    int res;
    res = vfs_mount(&_test_tmpfs_mount);
    TEST_ASSERT_EQUAL_INT(0, res);

    res = vfs_umount(&_test_tmpfs_mount);
    TEST_ASSERT_EQUAL_INT(0, res);
}

static void test_tmpfs_open_close(void)
{
    int res;
    int fd;
    res = vfs_mount(&_test_tmpfs_mount);
    TEST_ASSERT_EQUAL_INT(0, res);

    fd = vfs_open("/tmp/test", O_RDONLY, 0);
    TEST_ASSERT_EQUAL_INT(-EINVAL, fd);

    fd = vfs_open("/tmp/test", O_RDWR, 0);
    TEST_ASSERT_EQUAL_INT(-EINVAL, fd);

    fd = vfs_open("/tmp/test", O_RDWR | O_CREAT, 0);
    TEST_ASSERT(fd >= 0);

    res = vfs_close(fd);
    TEST_ASSERT_EQUAL_INT(0, res);

    fd = vfs_open("/tmp/test", O_RDONLY, 0);
    TEST_ASSERT(fd >= 0);

    res = vfs_close(fd);
    TEST_ASSERT_EQUAL_INT(0, res);

    res = vfs_umount(&_test_tmpfs_mount);
    TEST_ASSERT_EQUAL_INT(0, res);
}

static void test_tmpfs_write_read(void)
{
    const char w_buf[] = "ABCDEFG01234567890";
    char r_buf[2 * sizeof(w_buf) + 4];
    int res;
    int fd;
    res = vfs_mount(&_test_tmpfs_mount);
    TEST_ASSERT_EQUAL_INT(0, res);

    fd = vfs_open("/tmp/test", O_RDWR | O_CREAT, 0);
    TEST_ASSERT(fd >= 0);

    res = vfs_write(fd, w_buf, sizeof(w_buf));
    TEST_ASSERT_EQUAL_INT(sizeof(w_buf), res);

    res = vfs_write(fd, w_buf, sizeof(w_buf));
    TEST_ASSERT_EQUAL_INT(sizeof(w_buf), res);

    res = vfs_close(fd);
    TEST_ASSERT_EQUAL_INT(0, res);

    fd = vfs_open("/tmp/test", O_RDONLY, 0);
    TEST_ASSERT(fd >= 0);

    res = vfs_read(fd, r_buf, sizeof(r_buf));
    TEST_ASSERT_EQUAL_INT(2 * sizeof(w_buf), res);
    for (size_t i = 0; i < sizeof(w_buf); i++) {
        TEST_ASSERT_EQUAL_INT(w_buf[i], r_buf[i]);
        TEST_ASSERT_EQUAL_INT(w_buf[i], r_buf[i + sizeof(w_buf)]);
    }

    res = vfs_close(fd);
    TEST_ASSERT_EQUAL_INT(0, res);

    fd = vfs_open("/tmp/test", O_RDWR, 0);
    TEST_ASSERT(fd >= 0);

    res = vfs_lseek(fd, 5, SEEK_SET);
    TEST_ASSERT_EQUAL_INT(5, res);

    res = vfs_write(fd, w_buf, sizeof(w_buf));
    TEST_ASSERT_EQUAL_INT(sizeof(w_buf), res);

    res = vfs_lseek(fd, 0, SEEK_SET);
    TEST_ASSERT_EQUAL_INT(0, res);

    res = vfs_read(fd, r_buf, sizeof(r_buf));
    TEST_ASSERT_EQUAL_INT(2 * sizeof(w_buf), res);
    for (size_t i = 0; i < 5; i++) {
        TEST_ASSERT_EQUAL_INT(w_buf[i], r_buf[i]);
    }
    for (size_t i = 5; i < 5 + sizeof(w_buf); i++) {
        TEST_ASSERT_EQUAL_INT(w_buf[i - 5], r_buf[i]);
    }

    res = vfs_close(fd);
    TEST_ASSERT_EQUAL_INT(0, res);

    res = vfs_umount(&_test_tmpfs_mount);
    TEST_ASSERT_EQUAL_INT(0, res);
}

static void test_tmpfs_overwrite(void)
{
    const char w_buf[]  = "ABCDEFG01234567890";
    const char w2_buf[] = "hijklmnopqrstuvwxy";
    char r_buf[3 * sizeof(w_buf) + 4];
    int res;
    int fd, fd1, fd2;
    res = vfs_mount(&_test_tmpfs_mount);
    TEST_ASSERT_EQUAL_INT(0, res);

    /* create file1 32 bytes */    
    fd1 = vfs_open("/tmp/test1", O_RDWR | O_CREAT, 0);
    TEST_ASSERT(fd1 >= 0);
    res = vfs_write(fd1, w_buf, sizeof(w_buf));
    TEST_ASSERT_EQUAL_INT(sizeof(w_buf), res);
    res = vfs_write(fd1, w_buf, sizeof(w_buf));
    TEST_ASSERT_EQUAL_INT(sizeof(w_buf), res);
    res = vfs_close(fd1);
    TEST_ASSERT_EQUAL_INT(0, res);

    /* create file2 32 bytes */    
    fd2 = vfs_open("/tmp/test2", O_RDWR | O_CREAT, 0);
    TEST_ASSERT(fd1 >= 0);
    res = vfs_write(fd2, w2_buf, sizeof(w2_buf));
    TEST_ASSERT_EQUAL_INT(sizeof(w2_buf), res);
    res = vfs_write(fd2, w2_buf, sizeof(w2_buf));
    TEST_ASSERT_EQUAL_INT(sizeof(w2_buf), res);
    res = vfs_close(fd2);
    TEST_ASSERT_EQUAL_INT(0, res);

    /* read 32B file1 */
    fd = vfs_open("/tmp/test1", O_RDONLY, 0);
    TEST_ASSERT(fd >= 0);
    res = vfs_read(fd, r_buf, sizeof(r_buf));
    printf("file1 size=%d\n",res);
    TEST_ASSERT_EQUAL_INT(2 * sizeof(w_buf), res);
    for (size_t i = 0; i < sizeof(w_buf); i++) {
        TEST_ASSERT_EQUAL_INT(w_buf[i], r_buf[i]);
        TEST_ASSERT_EQUAL_INT(w_buf[i], r_buf[i + sizeof(w_buf)]);
    }
    res = vfs_close(fd);
    TEST_ASSERT_EQUAL_INT(0, res);

    /* shrink file1 to 16 bytes */
    /* O_TRUNC not supported => unlink+create */
    res = vfs_unlink("/tmp/test1");
    TEST_ASSERT_EQUAL_INT(0, res);
    fd = vfs_open("/tmp/test1", O_WRONLY | O_CREAT, 0);
    TEST_ASSERT(fd >= 0);
    res = vfs_write(fd, w_buf, sizeof(w_buf));
    TEST_ASSERT_EQUAL_INT(sizeof(w_buf), res);
    res = vfs_close(fd);
    TEST_ASSERT_EQUAL_INT(0, res);

    /* read 16B file1 */
    fd = vfs_open("/tmp/test1", O_RDONLY, 0);
    TEST_ASSERT(fd >= 0);
    res = vfs_read(fd, r_buf, sizeof(r_buf));
    printf("file1 size=%d\n",res);
    TEST_ASSERT_EQUAL_INT(sizeof(w_buf), res);
    for (size_t i = 0; i < sizeof(w_buf); i++) {
        TEST_ASSERT_EQUAL_INT(w_buf[i], r_buf[i]);
        TEST_ASSERT_EQUAL_INT(w_buf[i], r_buf[i + sizeof(w_buf)]);
    }
    res = vfs_close(fd);
    TEST_ASSERT_EQUAL_INT(0, res);

    /* read 32B file2 */
    fd = vfs_open("/tmp/test2", O_RDONLY, 0);
    TEST_ASSERT(fd >= 0);
    res = vfs_read(fd, r_buf, sizeof(r_buf));
    printf("file2 size=%d\n",res);
    TEST_ASSERT_EQUAL_INT(2 * sizeof(w2_buf), res);
    for (size_t i = 0; i < sizeof(w2_buf); i++) {
        TEST_ASSERT_EQUAL_INT(w2_buf[i], r_buf[i]);
        TEST_ASSERT_EQUAL_INT(w2_buf[i], r_buf[i + sizeof(w2_buf)]);
    }
    res = vfs_close(fd);
    TEST_ASSERT_EQUAL_INT(0, res);

    /* extend file1 to 16*3 bytes 0_APPEND */
    fd = vfs_open("/tmp/test1", O_WRONLY | O_APPEND, 0);
    TEST_ASSERT(fd >= 0);
    res = vfs_write(fd, w_buf, sizeof(w_buf));
    TEST_ASSERT_EQUAL_INT(sizeof(w_buf), res);
    res = vfs_write(fd, w_buf, sizeof(w_buf));
    TEST_ASSERT_EQUAL_INT(sizeof(w_buf), res);
    res = vfs_close(fd);
    TEST_ASSERT_EQUAL_INT(0, res);

    /* extend file2 to 16*3 bytes 0_APPEND */
    fd = vfs_open("/tmp/test2", O_WRONLY | O_APPEND, 0);
    TEST_ASSERT(fd >= 0);
    res = vfs_write(fd, w2_buf, sizeof(w2_buf));
    TEST_ASSERT_EQUAL_INT(sizeof(w2_buf), res);
    res = vfs_close(fd);
    TEST_ASSERT_EQUAL_INT(0, res);

    /* read 16*3B file1 */
    fd = vfs_open("/tmp/test1", O_RDONLY, 0);
    TEST_ASSERT(fd >= 0);
    res = vfs_read(fd, r_buf, sizeof(r_buf));
    printf("file1 size=%d\n",res);
    TEST_ASSERT_EQUAL_INT(3* sizeof(w_buf), res);
    for (size_t i = 0; i < sizeof(w_buf); i++) {
        TEST_ASSERT_EQUAL_INT(w_buf[i], r_buf[i]);
        TEST_ASSERT_EQUAL_INT(w_buf[i], r_buf[i + sizeof(w_buf)]);
        TEST_ASSERT_EQUAL_INT(w_buf[i], r_buf[i + 2*sizeof(w_buf)]);
    }
    res = vfs_close(fd);
    TEST_ASSERT_EQUAL_INT(0, res);

    /* read 16*3B file2 */
    fd = vfs_open("/tmp/test2", O_RDONLY, 0);
    TEST_ASSERT(fd >= 0);
    res = vfs_read(fd, r_buf, sizeof(r_buf));
    printf("file2 size=%d\n",res);
    TEST_ASSERT_EQUAL_INT(3* sizeof(w2_buf), res);
    for (size_t i = 0; i < sizeof(w2_buf); i++) {
        TEST_ASSERT_EQUAL_INT(w2_buf[i], r_buf[i]);
        TEST_ASSERT_EQUAL_INT(w2_buf[i], r_buf[i + sizeof(w2_buf)]);
        TEST_ASSERT_EQUAL_INT(w2_buf[i], r_buf[i + 2*sizeof(w2_buf)]);
    }
    res = vfs_close(fd);
    TEST_ASSERT_EQUAL_INT(0, res);

    /* seek test */
    fd1 = vfs_open("/tmp/test1", O_RDWR, 0);
    TEST_ASSERT(fd1 >= 0);
    res = vfs_lseek(fd1, 5, SEEK_SET);
    TEST_ASSERT_EQUAL_INT(5, res);
    res = vfs_write(fd1, w_buf, sizeof(w_buf));
    TEST_ASSERT_EQUAL_INT(sizeof(w_buf), res);
    res = vfs_lseek(fd1, 0, SEEK_SET);
    TEST_ASSERT_EQUAL_INT(0, res);
    res = vfs_read(fd1, r_buf, sizeof(r_buf));
    TEST_ASSERT_EQUAL_INT(3 * sizeof(w_buf), res);
    for (size_t i = 0; i < 5; i++) {
        TEST_ASSERT_EQUAL_INT(w_buf[i], r_buf[i]);
    }
    for (size_t i = 5; i < 5 + sizeof(w_buf); i++) {
        TEST_ASSERT_EQUAL_INT(w_buf[i - 5], r_buf[i]);
    }
    res = vfs_close(fd1);
    TEST_ASSERT_EQUAL_INT(0, res);

    res = vfs_umount(&_test_tmpfs_mount);
    TEST_ASSERT_EQUAL_INT(0, res);
}

static void tests_tmpfs_umount_open(void)
{
    const char w_buf[] = "ABCDEFG01234567890";
    char r_buf[2 * sizeof(w_buf) + 4];
    int fd;
    int res;
    res = vfs_mount(&_test_tmpfs_mount);
    TEST_ASSERT_EQUAL_INT(0, res);

    fd = vfs_open("/tmp/test", O_RDWR | O_CREAT, 0);
    TEST_ASSERT(fd >= 0);

    res = vfs_write(fd, w_buf, sizeof(w_buf));
    TEST_ASSERT_EQUAL_INT(sizeof(w_buf), res);

    res = vfs_close(fd);
    TEST_ASSERT_EQUAL_INT(0, res);

    fd = vfs_open("/tmp/test", O_RDONLY, 0);
    TEST_ASSERT(fd >= 0);

    res = vfs_read(fd, r_buf, sizeof(r_buf));
    TEST_ASSERT_EQUAL_INT(sizeof(w_buf), res);

    res = vfs_close(fd);
    TEST_ASSERT_EQUAL_INT(0, res);

    res = vfs_umount(&_test_tmpfs_mount);
    TEST_ASSERT_EQUAL_INT(0, res);

    res = vfs_mount(&_test_tmpfs_mount);
    TEST_ASSERT_EQUAL_INT(0, res);

    fd = vfs_open("/tmp/test", O_RDONLY, 0);
    TEST_ASSERT_EQUAL_INT(-EINVAL, fd);

    res = vfs_umount(&_test_tmpfs_mount);
    TEST_ASSERT_EQUAL_INT(0, res);
}

static void tests_tmpfs_unlink(void)
{
    const char buf[] = "TESTSTRING";
    int res;
    int fd;

    res = vfs_mount(&_test_tmpfs_mount);
    TEST_ASSERT_EQUAL_INT(0, res);

    fd = vfs_open("/tmp/test.txt", O_CREAT | O_RDWR, 0);
    TEST_ASSERT(fd >= 0);

    res = vfs_write(fd, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(sizeof(buf), res);

    res = vfs_close(fd);
    TEST_ASSERT_EQUAL_INT(0, res);

    res = vfs_unlink("/tmp/test.txt");
    TEST_ASSERT_EQUAL_INT(0, res);

    fd = vfs_open("/tmp/test.txt", O_RDONLY, 0);
    TEST_ASSERT_EQUAL_INT(-EINVAL, fd);

    res = vfs_umount(&_test_tmpfs_mount);
    TEST_ASSERT_EQUAL_INT(0, res);
}

static void tests_tmpfs_statvfs(void)
{
    const char buf[] = "TESTSTRING";
    struct statvfs stat1;
    struct statvfs stat2;

    int res = vfs_mount(&_test_tmpfs_mount);
    TEST_ASSERT_EQUAL_INT(0, res);

    res = vfs_statvfs("/tmp/", &stat1);
    TEST_ASSERT_EQUAL_INT(0, res);
    TEST_ASSERT_EQUAL_INT(1, stat1.f_bsize);
    TEST_ASSERT_EQUAL_INT(1, stat1.f_frsize);
    TEST_ASSERT_EQUAL_INT(0, stat1.f_bfree);
    TEST_ASSERT_EQUAL_INT(0, stat1.f_blocks);

    int fd = vfs_open("/tmp/test.txt", O_CREAT | O_RDWR, 0);
    TEST_ASSERT(fd >= 0);

    res = vfs_write(fd, buf, sizeof(buf));
    TEST_ASSERT(res == sizeof(buf));

    res = vfs_close(fd);
    TEST_ASSERT_EQUAL_INT(0, res);

    res = vfs_statvfs("/tmp/", &stat2);
    TEST_ASSERT_EQUAL_INT(0, res);
    TEST_ASSERT_EQUAL_INT(1, stat2.f_bsize);
    TEST_ASSERT_EQUAL_INT(1, stat2.f_frsize);
    TEST_ASSERT_EQUAL_INT(0, stat2.f_bfree);
    TEST_ASSERT_EQUAL_INT(sizeof(buf), stat2.f_blocks);

    res = vfs_umount(&_test_tmpfs_mount);
    TEST_ASSERT_EQUAL_INT(0, res);
}

static void tests_tmpfs_rename(void)
{
    const char buf[] = "TESTSTRING";

    int res = vfs_mount(&_test_tmpfs_mount);
    TEST_ASSERT_EQUAL_INT(0, res);

    int fd = vfs_open("/tmp/test.txt", O_CREAT | O_RDWR, 0);
    TEST_ASSERT(fd >= 0);

    res = vfs_write(fd, buf, sizeof(buf));
    TEST_ASSERT(res == sizeof(buf));

    res = vfs_close(fd);
    TEST_ASSERT_EQUAL_INT(0, res);

    fd = vfs_open("/tmp/test2.txt", O_RDONLY, 0);
    TEST_ASSERT(fd < 0);

    res = vfs_rename("/tmp/test.txt", "/tmp/test2.txt");
    TEST_ASSERT_EQUAL_INT(0, res);

    fd = vfs_open("/tmp/test2.txt", O_RDONLY, 0);
    TEST_ASSERT(fd >= 0);

    res = vfs_close(fd);
    TEST_ASSERT_EQUAL_INT(0, res);

    res = vfs_rename("/tmp/test.txt", "/tmp/test2.txt");
    TEST_ASSERT_EQUAL_INT(-ENOENT, res);

    res = vfs_umount(&_test_tmpfs_mount);
    TEST_ASSERT_EQUAL_INT(0, res);
}

Test *tests_tmpfs_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(tests_tmpfs_mount_umount),
        new_TestFixture(test_tmpfs_open_close),
        new_TestFixture(test_tmpfs_write_read),
        new_TestFixture(test_tmpfs_overwrite),
        new_TestFixture(tests_tmpfs_umount_open),
        new_TestFixture(tests_tmpfs_unlink),
        new_TestFixture(tests_tmpfs_statvfs),
        new_TestFixture(tests_tmpfs_rename),
    };

    EMB_UNIT_TESTCALLER(tmpfs_tests, NULL, NULL, fixtures);

    return (Test *)&tmpfs_tests;
}

void tests_tmpfs(void)
{
    TESTS_RUN(tests_tmpfs_tests());
}
/** @} */
