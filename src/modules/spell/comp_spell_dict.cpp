/*
 * SPDX-FileCopyrightText: 2012-2012 Yichao Yu <yyc1992@gmail.com>
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <vector> // IWYU pragma: keep
#include "fcitx-utils/endian_p.h"
#include "fcitx-utils/fs.h"
#include "fcitx-utils/standardpaths.h"
#include "fcitx-utils/unixfd.h"
#include "config.h"

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

using namespace fcitx;

#define DICT_BIN_MAGIC "FSCD0000"
const char null_byte = '\0';

static int compile_dict(int ifd, int ofd) {
    struct stat istat_buf;
    uint32_t wcount = 0;
    char *p;
    char *ifend;
    if (fstat(ifd, &istat_buf) == -1) {
        return 1;
    }

#ifdef HAVE_SYS_MMAN_H
    auto unmap = [&istat_buf](void *p) {
        if (p && p != MAP_FAILED) {
            munmap(p, istat_buf.st_size + 1);
        }
    };
    std::unique_ptr<void, std::function<void(void *)>> mmapped(nullptr, unmap);

    mmapped.reset(
        mmap(nullptr, istat_buf.st_size + 1, PROT_READ, MAP_PRIVATE, ifd, 0));
    if (mmapped.get() == MAP_FAILED) {
        return 1;
    }
    p = static_cast<char *>(mmapped.get());
#else
    std::vector<char> buffer;
    buffer.resize(istat_buf.st_size + 1);
    ssize_t read_bytes = fs::safeRead(ifd, buffer.data(), istat_buf.st_size);
    if (read_bytes != istat_buf.st_size) {
        return 1;
    }
    buffer[istat_buf.st_size] = '\0';
    p = buffer.data();
#endif
    ifend = istat_buf.st_size + p;
    fs::safeWrite(ofd, DICT_BIN_MAGIC, strlen(DICT_BIN_MAGIC));
    if (lseek(ofd, sizeof(uint32_t), SEEK_CUR) == static_cast<off_t>(-1)) {
        return 1;
    }
    while (p < ifend) {
        char *start;
        long int ceff;
        uint16_t ceff_buff;
        ceff = strtol(p, &p, 10);
        if (*p != ' ') {
            return 1;
        }
        ceff_buff = htole16(ceff > UINT16_MAX ? UINT16_MAX : ceff);
        fs::safeWrite(ofd, &ceff_buff, sizeof(uint16_t));
        start = ++p;
        p += strcspn(p, "\n");
        fs::safeWrite(ofd, start, p - start);
        fs::safeWrite(ofd, &null_byte, 1);
        wcount++;
        p++;
    }
    if (lseek(ofd, strlen(DICT_BIN_MAGIC), SEEK_SET) ==
        static_cast<off_t>(-1)) {
        return 1;
    }
    wcount = htole32(wcount);
    fs::safeWrite(ofd, &wcount, sizeof(uint32_t));
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc == 4 && strcmp(argv[1], "--comp-dict") == 0) {
        UnixFD ifd = StandardPaths::openPath(argv[2]);
        UnixFD ofd = StandardPaths::openPath(
            argv[3], O_WRONLY | O_TRUNC | O_CREAT, 0644);
        if (!ifd.isValid() || !ofd.isValid()) {
            return 1;
        }
        return compile_dict(ifd.fd(), ofd.fd());
    }
    std::cerr << "Usage: " << argv[0]
              << " --comp-dict <input file> <output file>\n";
    return 1;
}
