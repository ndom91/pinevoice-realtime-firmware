/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <vfs.h>
#include <vfs_inode.h>
#include <vfs_file.h>
#include <vfs_register.h>
#include <ipc.h>
#include "vfs_ifs.h"
#include <csi_core.h>

#define SHM_ALIGN(addr, align) ((void *)(((uint32_t)(addr) + align - 1U) & (~(uint32_t)(align - 1U))))
#define SHM_ALIGN_SIZE(size, align) (((uint32_t)(size) + align - 1U) & (~(align - 1U)))

typedef struct {
    uint64_t addr;
    int len;
} msg_t;

typedef struct {
    ipc_t              *ipc;
} ifs_manager_t;

static ifs_manager_t g_ifs_manager = {0};

static void _msg_process(ipc_t *ipc, message_t *msg, void *priv)
{
    switch (msg->command) {
    case IFS_CMD_OPEN: {
    	struct ifs_open_arg *req = (struct ifs_open_arg *)msg->req_data;
    	int fd = aos_open(req->path, req->flags);
    	*(uint64_t *)msg->resp_data = (uint64_t)fd;
    	msg->resp_len = sizeof(uint64_t);
    	printf("xxxxxxxxxxxxx %s %d\r\n", req->path, fd);
    	break;
    }
    case IFS_CMD_READ: {
    	int fd = (int)*(uint64_t *)msg->req_data;
    	uint32_t ret = aos_read(fd, (void *)(msg->resp_data + sizeof(ret)), msg->resp_len - sizeof(ret));
    	*(uint32_t *)msg->resp_data = ret;
    	//msg->resp_len = sizeof(uint32_t) + ret;
    	//printf("xxxxxxxxxxxxx read size:%d\r\n", ret);
    	break;
    }
    case IFS_CMD_WRITE: {
    	struct ifs_write_arg *req = (struct ifs_write_arg *)msg->req_data;
    	if (req && req->buffer) {
    		csi_dcache_invalid_range((size_t *)req->buffer, SHM_ALIGN_SIZE(req->len, 64));
    	}
    	int ret = aos_write(req->fd, (const void *)req->buffer, req->len);
    	*(uint32_t *)msg->resp_data = ret;
    	msg->resp_len = sizeof(uint32_t);
    	printf("xxxxxxxxxxxxx write fd:%d len:%d\r\n", req->fd, ret);
    	break;
    }
    case IFS_CMD_CLOSE: {
    	int fd = (int)*(uint64_t *)msg->req_data;
    	int ret = aos_close(fd);
    	*(uint32_t *)msg->resp_data = ret;
    	msg->resp_len = sizeof(uint32_t);
    	printf("xxxxxxxxxxxxx close fd:%d\r\n", fd);
    	break;
    }
    case IFS_CMD_LSEEK: {
    	struct ifs_seek_arg *req = (struct ifs_seek_arg *)msg->req_data;
    	off_t ret = aos_lseek(req->fd, req->off, req->whence);
    	*(off_t *)msg->resp_data = ret;
    	msg->resp_len = sizeof(off_t);
    	printf("xxxxxxxxxxxxx lseek fd:%d\r\n", req->fd);
    	break;
    }
    default :
    	break;
    }

    if (msg->flag & MESSAGE_SYNC) {
        ipc_message_ack(ipc, msg, AOS_WAIT_FOREVER);
    }
}

int32_t vfs_ifs_server_init(void)
{
    g_ifs_manager.ipc = ipc_get(0);
    ipc_add_service(g_ifs_manager.ipc, IFS_SERIVCE_ID, _msg_process, &g_ifs_manager);

    return 0;
}
