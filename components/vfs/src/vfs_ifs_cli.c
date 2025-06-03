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
#include <string.h>

#define SHM_ALIGN(addr, align) ((void *)(((uint32_t)(addr) + align - 1U) & (~(uint32_t)(align - 1U))))
#define SHM_ALIGN_SIZE(size, align) (((uint32_t)(size) + align - 1U) & (~(align - 1U)))

static char *ifs_mount_path;

typedef struct {
    uint64_t addr;
    int len;
} msg_t;

typedef struct {
    ipc_t              *ipc;
    aos_mutex_t        lock;
} ifs_manager_t;

static ifs_manager_t g_ifs_manager = {0};

/* Global FS lock init */
static void ifs_lock_create(aos_mutex_t *lock)
{
    aos_mutex_new(lock);
}

/* Global FS lock destroy */
static void ifs_lock_destory(aos_mutex_t *lock)
{
    if (lock) {
        aos_mutex_free(lock);
    }
}

static void ifs_lock(aos_mutex_t *lock)
{
    if (lock) {
        aos_mutex_lock(lock, WAIT_FOREVER);
    }
}

static void ifs_unlock(aos_mutex_t *lock)
{
    if (lock) {
    	aos_mutex_unlock(lock);
    }
}

static void _msg_process(ipc_t *ipc, message_t *msg, void *priv)
{
    msg_t *m = (msg_t *)msg->req_data;

    printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx 0x%08x %d\r\n", (size_t)m->addr, m->len);
    //if (m && m->addr) {
    //    csi_dcache_invalid_range((size_t *)m->addr, m->len);
    //}

    if (msg->flag & MESSAGE_SYNC) {
        ipc_message_ack(ipc, msg, AOS_WAIT_FOREVER);
    }
}

static int ifs_ipc_send(int cmd, void *req_data, int req_size, 
                       void *resp_data, int resp_size)
{
    int res = 0;

    message_t send_msg;

    ifs_lock(&g_ifs_manager.lock);
    memset(&send_msg, 0, sizeof(message_t));

    send_msg.command    = cmd;
    send_msg.flag       = MESSAGE_SYNC;
    send_msg.service_id = IFS_SERIVCE_ID;
    send_msg.req_data   = (uint64_t)req_data;
    send_msg.req_len    = req_size;
    send_msg.resp_data  = (uint64_t)resp_data;
    send_msg.resp_len   = resp_size;

	ipc_message_send(g_ifs_manager.ipc, &send_msg, AOS_WAIT_FOREVER);
	ifs_unlock(&g_ifs_manager.lock);
    return res;
}

static int32_t ifs_vfs_open(vfs_file_t *fp, const char *path, int flags)
{
    struct ifs_open_arg req;
    int64_t fd = 0;

    strncpy((char *)req.path, path, sizeof(req.path));
    req.flags = flags;

    ifs_ipc_send(IFS_CMD_OPEN, &req, sizeof(req), &fd, sizeof(fd));
	fp->f_arg = (void *)(int32_t)fd;

	printf("%s %s fd:%d\r\n", __func__, path, (int)fd);

    return 0;
}

static int32_t ifs_vfs_close(vfs_file_t *fp)
{
    int res;
    int64_t fd = (int64_t)fp->f_arg;

    ifs_ipc_send(IFS_CMD_CLOSE, &fd, sizeof(fd), &res, sizeof(res));

	printf("%s fd:%d res:%d\r\n", __func__, (int)fd, res);

    return res;
}

static int32_t ifs_vfs_read(vfs_file_t *fp, char *buf, uint32_t len)
{
    int64_t fd;
    char *resp_buf;
    uint32_t resp_size;

    fd = (int64_t)fp->f_arg;

    resp_buf = aos_zalloc(len + sizeof(resp_size));

    ifs_ipc_send(IFS_CMD_READ, &fd, sizeof(fd), resp_buf, len + sizeof(resp_size));
    resp_size = *(uint32_t *)resp_buf;
    memcpy(buf, resp_buf + sizeof(resp_size), resp_size);

    aos_free(resp_buf);

    printf("%s fd:%d size:%d\r\n", __func__, (int)fd, resp_size);

    return resp_size;
}

static int32_t ifs_vfs_write(vfs_file_t *fp, const char *buf, uint32_t len)
{
    uint32_t resp_size;
    char *origin_buf, *write_buf;
    struct ifs_write_arg req_data;

    uint32_t align_size = SHM_ALIGN_SIZE(len, 64) + 64;
    origin_buf = aos_malloc(align_size);
    write_buf = SHM_ALIGN(origin_buf, 64);
    memcpy(write_buf, buf, len);
    csi_dcache_clean_range((size_t *)write_buf, SHM_ALIGN_SIZE(len, 64));

    req_data.buffer = (int64_t)write_buf;
    req_data.fd = (int)fp->f_arg;
    req_data.len = len;
    ifs_ipc_send(IFS_CMD_WRITE, &req_data, sizeof(req_data), &resp_size, sizeof(resp_size));

    aos_free(origin_buf);
    printf("%s fd:%d origin:%p,%d req:%p,%d\r\n", __func__, (int)req_data.fd, origin_buf, len, write_buf, align_size);

    return resp_size;
}

static uint32_t ifs_vfs_lseek(vfs_file_t *fp, int64_t off, int32_t whence)
{
    off_t res = -1;
    struct ifs_seek_arg req_data;

    req_data.off = off;
    req_data.whence = whence;
    req_data.fd = (int32_t)fp->f_arg;

    ifs_ipc_send(IFS_CMD_LSEEK, &req_data, sizeof(req_data), &res, sizeof(res));

    printf("%s fd:%d\r\n", __func__, req_data.fd);

    return res;
}

static int32_t ifs_vfs_sync(vfs_file_t *fp)
{
    int res = -1;

    return res;
}

static int32_t ifs_vfs_fstat(vfs_file_t *fp, vfs_stat_t *st)
{
	int res = -1;

    return res;
}

static int32_t ifs_vfs_stat(vfs_file_t *fp, const char *path, vfs_stat_t *st)
{
	int res = -1;
    return res;
}

static int32_t ifs_vfs_access(vfs_file_t *fp, const char *path, int mode)
{
	int res = -1;

    return res;
}

static int32_t ifs_vfs_statfs(vfs_file_t *fp, const char *path, vfs_statfs_t *sfs)
{
	int res = -1;

    return res;
}

static int32_t ifs_vfs_remove(vfs_file_t *fp, const char *path)
{
	int res = -1;

    return res;
}

static int32_t ifs_vfs_rename(vfs_file_t *fp, const char *oldpath, const char *newpath)
{
	int res = -1;

    return res;
}

static vfs_dir_t *ifs_vfs_opendir(vfs_file_t *fp, const char *path)
{
    return (vfs_dir_t *)NULL;
}

static vfs_dirent_t *ifs_vfs_readdir(vfs_file_t *fp, vfs_dir_t *dir)
{

    return NULL;
}

static int32_t ifs_vfs_closedir(vfs_file_t *fp, vfs_dir_t *dir)
{
	int res = -1;

    return res;
}

static int32_t ifs_vfs_mkdir(vfs_file_t *fp, const char *path)
{
	int res = -1;

    return res;
}

static int32_t ifs_vfs_rmdir (vfs_file_t *fp, const char *path)
{
	int res = -1;

    return res;
}

static void ifs_vfs_rewinddir(vfs_file_t *fp, vfs_dir_t *dir)
{
}

static int32_t ifs_vfs_telldir(vfs_file_t *fp, vfs_dir_t *dir)
{
	int res = -1;

    return res;
}

static void ifs_vfs_seekdir(vfs_file_t *fp, vfs_dir_t *dir, int32_t loc)
{
}

static int32_t ifs_vfs_utime(vfs_file_t *fp, const char *path, const vfs_utimbuf_t *times)
{
	int res = -1;

    return res;
}

static int32_t ifs_vfs_truncate(vfs_file_t *fp, int64_t size)
{
	int res = -1;

    return res;
}


static vfs_filesystem_ops_t ifs_ops = {
    .open       = &ifs_vfs_open,
    .close      = &ifs_vfs_close,
    .read       = &ifs_vfs_read,
    .write      = &ifs_vfs_write,
    .lseek      = &ifs_vfs_lseek,
    .sync       = &ifs_vfs_sync,
    .stat       = &ifs_vfs_stat,
    .unlink     = &ifs_vfs_remove,
    .remove     = &ifs_vfs_remove,
    .rename     = &ifs_vfs_rename,
    .access     = &ifs_vfs_access,
    .fstat      = &ifs_vfs_fstat,
    .statfs     = &ifs_vfs_statfs,
    .opendir    = &ifs_vfs_opendir,
    .readdir    = &ifs_vfs_readdir,
    .closedir   = &ifs_vfs_closedir,
    .mkdir      = &ifs_vfs_mkdir,
    .rmdir      = &ifs_vfs_rmdir,
    .rewinddir  = &ifs_vfs_rewinddir,
    .telldir    = &ifs_vfs_telldir,
    .seekdir    = &ifs_vfs_seekdir,
    .ioctl      = NULL,
    .utime      = &ifs_vfs_utime,
    .truncate   = &ifs_vfs_truncate,
};

int32_t vfs_ifs_cli_register(char *partition_desc)
{
    ifs_mount_path = (char *)aos_malloc(sizeof(IPCFS_MOUNTPOINT) + 1);
    sprintf(ifs_mount_path, "%s", IPCFS_MOUNTPOINT);

    ifs_lock_create(&g_ifs_manager.lock);

    g_ifs_manager.ipc = ipc_get(1);
    ipc_add_service(g_ifs_manager.ipc, IFS_SERIVCE_ID, _msg_process, &g_ifs_manager);

    return vfs_register_fs(ifs_mount_path, &ifs_ops, NULL);
}

int32_t vfs_ifs_cli_unregister(void)
{
    vfs_unregister_fs(ifs_mount_path);
    ifs_lock_destory(&g_ifs_manager.lock);
    free(ifs_mount_path);
    return 0;
}
