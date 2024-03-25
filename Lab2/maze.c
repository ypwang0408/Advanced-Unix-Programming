#include <linux/module.h>	// included for all kernel modules
#include <linux/kernel.h>	// included for KERN_INFO
#include <linux/init.h>		// included for __init and __exit macros
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/errno.h>
#include <linux/sched.h>	// task_struct requried for current_uid()
#include <linux/cred.h>		// for current_uid();
#include <linux/slab.h>		// for kmalloc/kfree
#include <linux/uaccess.h>	// copy_to_user
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include "maze.h"

#define	DEVFILE	"/dev/maze_dev"


static dev_t devnum;
static struct cdev c_dev[_MAZE_MAXUSER];
static struct class *clazz;
static maze_t mazes[_MAZE_MAXUSER];
static int major;
static int dev_num;
DEFINE_MUTEX(maze_mutex);
static int pid[_MAZE_MAXUSER];
static int num_of_user = 0;
static long mazemod_dev_ioctl(struct file *fp, unsigned int cmd, unsigned long arg);

static int mazemod_dev_open(struct inode *i, struct file *f) {
	sscanf(f->f_path.dentry->d_iname, "maze%d", &dev_num);
	mutex_lock(&maze_mutex);
	if (num_of_user < _MAZE_MAXUSER) {
		for (int i=0; i<_MAZE_MAXUSER; i++) {
			if (pid[i] == 0) {
				pid[i] = current->pid;
				num_of_user++;
				mutex_unlock(&maze_mutex);
				break;
			}
		}
	} else {
		mutex_unlock(&maze_mutex);
		return -ENOMEM;
	}
	/*
	printk(KERN_INFO "maze: device %d open() by pid %d\n", dev_num, current->pid);
	for (int i=0; i<_MAZE_MAXUSER; i++) 
		printk(KERN_INFO "maze: pid[%d] = %d\n", i, pid[i]);
	*/
	return 0;
}
static int mazemod_dev_close(struct inode *i, struct file *f) {
	sscanf(f->f_path.dentry->d_iname, "maze%d", &dev_num);
	//printk(KERN_INFO "maze: device %d close() by pid %d\n", dev_num, current->pid);
	mutex_lock(&maze_mutex);
	for (int i=0; i<_MAZE_MAXUSER; i++) {
		if (pid[i] == current->pid) {
			pid[i] = 0;
			mazes[i].exist = 0;
			num_of_user--;
			break;
		}
	}
	mutex_unlock(&maze_mutex);
	return 0;
}
static ssize_t mazemod_dev_read(struct file *f, char __user *buf, size_t len, loff_t *off) {
	for (int i=0; i<_MAZE_MAXUSER; i++) 
		if (pid[i] == current->pid){
			dev_num = i;
			break;
		}
	if (dev_num >= _MAZE_MAXUSER)
		return -ENOMEM;
	char* tmp = (char *)kzalloc(mazes[dev_num].w * mazes[dev_num].h, GFP_KERNEL);
	for (int i=0; i<mazes[dev_num].h; i++)
		for (int j=0; j<mazes[dev_num].w; j++) 
			tmp[i*mazes[dev_num].w+j] = mazes[dev_num].blk[j][i] == '#' ? 1 : 0;
	int ret = copy_to_user(buf, tmp, mazes[dev_num].w * mazes[dev_num].h);
	if (ret) 
		return -EBUSY;
	return mazes[dev_num].w * mazes[dev_num].h;
}
static ssize_t mazemod_dev_write(struct file *f, const char __user *buf, size_t len, loff_t *off) {
	
	for (int i=0; i<_MAZE_MAXUSER; i++) 
		if (pid[i] == current->pid){
			dev_num = i;
			break;
		}
	if (dev_num >= _MAZE_MAXUSER)
		return -ENOMEM;
	if (!mazes[dev_num].exist)
		return -EBADFD;
	coord_t *tmp = (coord_t *)kzalloc(len, GFP_KERNEL);
	int ret = copy_from_user(tmp, buf, len);
	if (ret) 
		return -EBUSY;
	if (ret % sizeof(coord_t) != 0) 
		return -EINVAL;
	for (int i=0; i<len/sizeof(coord_t); i++)
		mazemod_dev_ioctl(f, MAZE_MOVE, (unsigned long)&tmp[i]);
	return len;
}
void random_dfs(int x, int y, int w, int h, char blk[_MAZE_MAXX][_MAZE_MAXY], int num) {
	if (num > 100)
		return;
	int dx[4] = {0, 0, 1, -1};
	int dy[4] = {1, -1, 0, 0};
	int d[4] = {0, 1, 2, 3};
	for (int i=0; i<4; i++) {
		int j = get_random_u32() % 4;
		int tmp = d[i];
		d[i] = d[j];
		d[j] = tmp;
	}
	blk[x][y] = '.';
	for (int i=0; i<4; i++) {
		int nx = x + dx[d[i]] * 2;
		int ny = y + dy[d[i]] * 2;
		if (nx >= 0 && nx < w && ny >= 0 && ny < h && blk[nx][ny] == '#') {
			blk[x+dx[d[i]]][y+dy[d[i]]] = '.';
			random_dfs(nx, ny, w, h, blk, num+1);
		}
	}
}
void get_start_end(int w, int h, int *sx, int *sy, int *ex, int *ey, char blk[_MAZE_MAXX][_MAZE_MAXY]) {
	do {
		*sx = get_random_u32() % (w-2) + 1;
		*sy = get_random_u32() % (h-2) + 1;
	} while (blk[*sx][*sy] != '.');
	do {
		*ex = get_random_u32() % (w-2) + 1;
		*ey = get_random_u32() % (h-2) + 1;
	} while ( blk[*ex][*ey] != '.' || (*sx == *ex && *sy == *ey));
}
static long mazemod_dev_ioctl(struct file *fp, unsigned int cmd, unsigned long arg) {
	int dev_num = -1;
	mutex_lock(&maze_mutex);
	for (int i=0; i<_MAZE_MAXUSER; i++) 
		if (pid[i] == current->pid)
			dev_num = i;
	mutex_unlock(&maze_mutex);
	if (dev_num == -1)
		return -ENOMEM;
	if (cmd == MAZE_CREATE) {
		if (mazes[dev_num].exist) 
			return -EEXIST;
		else if (dev_num >= _MAZE_MAXUSER)
			return -ENOMEM;
		coord_t *coor = (coord_t *)arg;
		mazes[dev_num].w = coor->x;
		mazes[dev_num].h = coor->y;
		//printk(KERN_INFO "maze: create %d x %d maze for pid %d\n", mazes[dev_num].w, mazes[dev_num].h, current->pid);
		if (mazes[dev_num].w > _MAZE_MAXX || mazes[dev_num].h > _MAZE_MAXY || mazes[dev_num].w < 0 || mazes[dev_num].h < 0)
			return -EINVAL;
		mazes[dev_num].exist = 1;
		for (int i=0; i<mazes[dev_num].w; i++) 
			for (int j=0; j<mazes[dev_num].h; j++) {
				if (i == 0 || i == mazes[dev_num].w-1 || j == 0 || j == mazes[dev_num].h-1)
					mazes[dev_num].blk[i][j] = '#';
				else
					mazes[dev_num].blk[i][j] = '#';
			}
		mutex_lock(&maze_mutex);
		random_dfs(1, 1, mazes[dev_num].w-1, mazes[dev_num].h-1, mazes[dev_num].blk, 1);
		mutex_unlock(&maze_mutex);
		get_start_end(mazes[dev_num].w, mazes[dev_num].h, &mazes[dev_num].sx, &mazes[dev_num].sy, &mazes[dev_num].ex, &mazes[dev_num].ey, mazes[dev_num].blk);
		mazes[dev_num].blk[mazes[dev_num].sx][mazes[dev_num].sy] = '*';
		mazes[dev_num].blk[mazes[dev_num].ex][mazes[dev_num].ey] = 'E';
		mazes[dev_num].x = mazes[dev_num].sx;
		mazes[dev_num].y = mazes[dev_num].sy;
		


	} else if (cmd == MAZE_RESET) {
		if (!mazes[dev_num].exist)
			return -ENOENT;
	} else if (cmd == MAZE_DESTROY) {
		if (!mazes[dev_num].exist)
			return -ENOENT;
		mazes[dev_num].exist = 0;
	} else if (cmd == MAZE_GETSIZE) {
		if (!mazes[dev_num].exist)
			return -ENOENT;
		coord_t *coor = (coord_t *)kzalloc(sizeof(coord_t), GFP_KERNEL);
		coor->x = mazes[dev_num].w;
		coor->y = mazes[dev_num].h;
		int ret = copy_to_user((coord_t *)arg, coor, sizeof(coord_t));
		if (ret) 
			return -EBUSY;
	} else if (cmd == MAZE_MOVE) {
		if (!mazes[dev_num].exist)
			return -ENOENT;
		coord_t *coor = (coord_t *)arg;
		coord_t *pos = (coord_t *)kzalloc(sizeof(coord_t), GFP_KERNEL);
		pos->x = mazes[dev_num].x + coor->x;
		pos->y = mazes[dev_num].y + coor->y;
		if (mazes[dev_num].blk[pos->x][pos->y] != '#') {
			mazes[dev_num].blk[mazes[dev_num].x][mazes[dev_num].y] = '.';
			mazes[dev_num].blk[pos->x][pos->y] = '*';
			mazes[dev_num].x = pos->x;
			mazes[dev_num].y = pos->y;
		}
		int ret = copy_to_user((coord_t *)arg, pos, sizeof(coord_t));
		if (ret)
			return -EBUSY;
	} else if (cmd == MAZE_GETPOS) {
		if (!mazes[dev_num].exist)
			return -ENOENT;
		coord_t *coor = (coord_t *)kzalloc(sizeof(coord_t), GFP_KERNEL);
		coor->x = mazes[dev_num].x;
		coor->y = mazes[dev_num].y;
		int ret = copy_to_user((coord_t *)arg, coor, sizeof(coord_t));
		if (ret) 
			return -EBUSY;
	} else if (cmd == MAZE_GETSTART) {
		if (!mazes[dev_num].exist)
			return -ENOENT;
		coord_t *coor = (coord_t *)kzalloc(sizeof(coord_t), GFP_KERNEL);
		coor->x = mazes[dev_num].sx;
		coor->y = mazes[dev_num].sy;
		int ret = copy_to_user((coord_t *)arg, coor, sizeof(coord_t));
		if (ret)
			return -EBUSY;
	} else if (cmd == MAZE_GETEND) {
		if (!mazes[dev_num].exist)
			return -ENOENT;
		coord_t *coor = (coord_t *)kzalloc(sizeof(coord_t), GFP_KERNEL);
		coor->x = mazes[dev_num].ex;
		coor->y = mazes[dev_num].ey;
		int ret = copy_to_user((coord_t *)arg, coor, sizeof(coord_t));
		if (ret) 
			return -EBUSY;
	}
	
	return 0;
}

static const struct file_operations mazemod_dev_fops = {
	.owner = THIS_MODULE,
	.open = mazemod_dev_open,
	.release = mazemod_dev_close,
	.read = mazemod_dev_read,
	.write = mazemod_dev_write,
	.unlocked_ioctl = mazemod_dev_ioctl
};

static int mazemod_proc_read(struct seq_file *m, void *v) {
	for(int i=0; i<_MAZE_MAXUSER; i++){
		if (mazes[i].exist){
			seq_printf(m, "#%02d: pid %d - [%d x %d]: (%d, %d) -> (%d, %d) @(%d, %d)\n", i, pid[i], mazes[i].w, mazes[i].h, mazes[i].sx, mazes[i].sy, mazes[i].ex, mazes[i].ey, mazes[i].x, mazes[i].y);
			for (int j=0; j<mazes[i].h; j++) {
				char line[_MAZE_MAXX+1];
				for (int k=0; k<mazes[i].w; k++) 
					line[k] = mazes[i].blk[k][j];
				line[mazes[i].w] = '\0';
				seq_printf(m, "- %03d: %s\n", j, line);
			}
		}
		else
			seq_printf(m, "#%02d: vacancy\n", i);
	seq_printf(m, "\n");
	}
		
	return 0;
}

static int mazemod_proc_open(struct inode *inode, struct file *file) {
	return single_open(file, mazemod_proc_read, NULL);
}

static const struct proc_ops mazemod_proc_fops = {
	.proc_open = mazemod_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release
};

static char *mazemod_devnode(const struct device *dev, umode_t *mode) {
	if(mode == NULL) return NULL;
	*mode = 0666;
	return NULL;
}

static int __init maze_init(void) {
	dev_t newdev;
	for (int i=0; i<_MAZE_MAXUSER; i++) {
		mazes[i].exist = 0;
		pid[i] = 0;
	}
	num_of_user = 0;

	if (alloc_chrdev_region(&devnum, 0, _MAZE_MAXUSER, "maze_dev") < 0) 
		return -1;
	if ((clazz = class_create("maze_class")) == NULL) 
		goto release_region;

	clazz->devnode = mazemod_devnode;
	
	major = MAJOR(devnum);

	for (int i=0; i<_MAZE_MAXUSER; i++) {
		newdev = MKDEV(major, i);
		if (device_create(clazz, NULL, newdev, NULL, "maze") == NULL) 
			goto release_class;
		cdev_init(&c_dev[i], &mazemod_dev_fops);
		if (cdev_add(&c_dev[i], newdev, 1) == -1) 
			goto release_device;
		
	}
	proc_create("maze", 0, NULL, &mazemod_proc_fops);
	//printk(KERN_INFO "maze: init()\n");
	return 0;

release_device:
	device_destroy(clazz, devnum);
release_class:
	class_destroy(clazz);
release_region:
	unregister_chrdev_region(devnum, 1);
	return -1;
}

static void __exit maze_cleanup(void) {
	for (int i=0; i<_MAZE_MAXUSER; i++) {
		pid[i] = 0;
		mazes[i].exist = 0;
	}
	num_of_user = 0;
	/*
	for (int i=0; i<_MAZE_MAXUSER; i++) 
		printk(KERN_INFO "maze: pid[%d] = %d\n", i, pid[i]);
	printk(KERN_INFO "maze: cleanup() num_of_user = %d\n", num_of_user);
	*/
	remove_proc_entry("maze", NULL);
	for (int i=0; i<_MAZE_MAXUSER; i++) {
		cdev_del(&c_dev[i]);
		device_destroy(clazz, MKDEV(major, i));
	}
	class_destroy(clazz);
	unregister_chrdev_region(devnum, _MAZE_MAXUSER);
	printk(KERN_INFO "maze: cleanup()\n");
}

module_init(maze_init);
module_exit(maze_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("YPW");
MODULE_DESCRIPTION("maze module");