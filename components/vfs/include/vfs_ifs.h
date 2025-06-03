/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <vfs.h>
#include <vfs_inode.h>
#include <vfs_file.h>
#include <vfs_register.h>
#include <ipc.h>

#define WAIT_FOREVER 0xFFFFFFFF
#define IPCFS_MOUNTPOINT "/"

#define IFS_PATH_MAX_SIZE 64

#define IFS_SERIVCE_ID (0x11)

#define IFS_CMD_OPEN    (1)
#define IFS_CMD_READ    (2)
#define IFS_CMD_WRITE   (3)
#define IFS_CMD_CLOSE   (4)
#define IFS_CMD_LSEEK   (5)

struct ifs_open_arg {
	const char path[IFS_PATH_MAX_SIZE];
	int flags;
};

struct ifs_write_arg {
	int64_t buffer;
	int fd;
	int len;
};

struct ifs_seek_arg {
	int64_t off;
	int32_t fd;
	int32_t whence;
};

int32_t vfs_ifs_server_init(void);

int32_t vfs_ifs_cli_register(char *partition_desc);
int32_t vfs_ifs_cli_unregister(void);


