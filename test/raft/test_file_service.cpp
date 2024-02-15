// Copyright (c) 2015 Baidu.com, Inc. All Rights Reserved


#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include <melon/utility/logging.h>
#include <melon/utility/file_util.h>

#include <melon/rpc/server.h>
#include "melon/raft/file_service.h"
#include "melon/raft/util.h"
#include "melon/raft/remote_file_copier.h"
#include "melon/raft/file_system_adaptor.h"

namespace melon::raft {
DECLARE_bool(raft_file_check_hole);
}

int g_port = 0;
class FileServiceTest : public testing::Test {
protected:
    void SetUp() {
        ASSERT_EQ(0, _server.AddService(melon::raft::file_service(),
                                        melon::SERVER_DOESNT_OWN_SERVICE));
	for (int i = 10000; i < 60000; i++) {
            if (0 == _server.Start(i, NULL)) {
		g_port = i;
		break;
	    }
	}
	ASSERT_NE(0, g_port);
    }
    void TearDown() {
        _server.Stop(0);
        _server.Join();
    }
    melon::Server _server;
};

TEST_F(FileServiceTest, sanity) {
    melon::raft::FileSystemAdaptor* fs = melon::raft::default_file_system();
    scoped_refptr<melon::raft::LocalDirReader> reader(new melon::raft::LocalDirReader(fs, "a"));
    int64_t reader_id = 0;
    ASSERT_EQ(0, melon::raft::file_service_add(reader.get(), &reader_id));
    std::string uri;
    mutil::string_printf(&uri, "remote://127.0.0.1:%d/%" PRId64, g_port, reader_id);
    melon::raft::RemoteFileCopier copier;
    {
	std::string bad_uri;
    	mutil::string_printf(&bad_uri, "local://127.0.0.1:%d/123456", g_port);
    	ASSERT_NE(0, copier.init(bad_uri, fs, NULL));

	bad_uri.clear();
    	mutil::string_printf(&bad_uri, "remote://127.0.0.1:%d//123456", g_port);
    	ASSERT_NE(0, copier.init(bad_uri, fs, NULL));

	bad_uri.clear();
    	mutil::string_printf(&bad_uri, "remote://127.0.1:%d//123456", g_port);
    	ASSERT_NE(0, copier.init(bad_uri, fs, NULL));

    	ASSERT_NE(0, copier.init("remote://127.0.0.1//123456", fs, NULL));
    }
    ASSERT_EQ(0, copier.init(uri, fs, NULL));

    // normal copy dir
    system("chmod -R 755 ./a; chmod -R 755 ./b");
    ASSERT_EQ(0, system("rm -rf a; rm -rf b; mkdir a; mkdir a/b; echo '123' > a/c"));
    ASSERT_TRUE(mutil::CreateDirectory(mutil::FilePath("./b")));
    ASSERT_EQ(0, copier.copy_to_file("c", "./b/c", NULL));
    mutil::IOBuf c_data;
    ASSERT_EQ(0, copier.copy_to_iobuf("c", &c_data, NULL));
    ASSERT_TRUE(c_data.equals("123\n")) << c_data.to_string();
    // Copy Directory is not allowed
    ASSERT_NE(0, copier.copy_to_file("b", "./b/b", NULL));

    // Copy non-existed file
    ASSERT_NE(0, copier.copy_to_file("d", "./b/d", NULL));

    // src no permission read
    ASSERT_EQ(0, system("chmod 000 a/c"));
    ASSERT_NE(0, copier.copy_to_file("c", "./b/cc", NULL));
    ASSERT_EQ(0, system("chmod -R 755 ./a"));

    ASSERT_EQ(0, melon::raft::file_service_remove(reader_id));

    // Copy after reader is remove
    ASSERT_NE(0, copier.copy_to_file("c", "./b/d", NULL));
    ASSERT_EQ(0, system("rm -rf a; rm -rf b;"));
}

TEST_F(FileServiceTest, hole_file) {
    int ret = 0;
    ASSERT_EQ(0, system("rm -rf a; rm -rf b; rm -rf c; mkdir a;"));

    LOG(INFO) << "build hole file";
    int fd = ::open("./a/hole.data", O_CREAT | O_TRUNC | O_WRONLY, 0644);
    ASSERT_GE(fd, 0);
    for (int i = 0; i < 1000; i++) {
        char buf[16*1024] = {0};
        snprintf(buf, sizeof(buf), "hello %d", i);
        ssize_t nwritten = pwrite(fd, buf, strlen(buf), 128 * 1024 * i);
        ASSERT_EQ(static_cast<size_t>(nwritten), strlen(buf));
    }
    ::close(fd);
    melon::raft::FileSystemAdaptor* fs = melon::raft::default_file_system();
    scoped_refptr<melon::raft::LocalDirReader> reader(new melon::raft::LocalDirReader(fs, "a"));
    int64_t reader_id = 0;
    ASSERT_EQ(0, melon::raft::file_service_add(reader.get(), &reader_id));

    melon::raft::RemoteFileCopier copier;
    std::string uri;
    mutil::string_printf(&uri, "remote://127.0.0.1:%d/%" PRId64, g_port, reader_id);
    // normal init
    melon::raft::FLAGS_raft_file_check_hole = false;
    ASSERT_EQ(0, copier.init(uri, fs, NULL));
    ASSERT_TRUE(mutil::CreateDirectory(mutil::FilePath("./b")));
    ASSERT_EQ(0, copier.copy_to_file("hole.data", "./b/hole.data", NULL));
    ret = system("diff ./a/hole.data ./b/hole.data");
    ASSERT_EQ(0, ret);

    melon::raft::FLAGS_raft_file_check_hole = true;
    ASSERT_TRUE(mutil::CreateDirectory(mutil::FilePath("./c")));
    ASSERT_EQ(0, copier.copy_to_file("hole.data", "./c/hole.data", NULL));
    ret = system("diff ./a/hole.data ./c/hole.data");
    ASSERT_EQ(0, ret);
}
