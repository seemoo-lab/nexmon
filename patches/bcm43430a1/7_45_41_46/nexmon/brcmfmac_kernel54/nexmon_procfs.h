#ifndef NEXMON_PROCFS_H
#define NEXMON_PROCFS_H

#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

extern int procfs_dump_open(struct inode *inode, struct file *file);
extern ssize_t procfs_dump_read(struct file *file, char *buffer, size_t len, loff_t * off);

static const struct file_operations
rom_proc_dump_fops = {
    .owner      = THIS_MODULE,
    .open       = procfs_dump_open,
    .read       = procfs_dump_read,
};

#endif /* NEXMON_PROCFS_H */
