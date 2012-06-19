#ifndef CRTOOLS_H_
#define CRTOOLS_H_

#include <sys/types.h>

#include "list.h"
#include "types.h"
#include "list.h"
#include "util.h"
#include "image.h"

#define CR_FD_PERM		(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)
#define CR_FD_PERM_DUMP		(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

enum {
	/*
	 * Task entries
	 */

	_CR_FD_TASK_FROM,
	CR_FD_FDINFO,
	CR_FD_PAGES,
	CR_FD_CORE,
	CR_FD_MM,
	CR_FD_VMAS,
	CR_FD_SIGACT,
	CR_FD_ITIMERS,
	CR_FD_CREDS,
	CR_FD_FS,
	_CR_FD_TASK_TO,

	/*
	 * NS entries
	 */

	_CR_FD_NS_FROM,
	CR_FD_UTSNS,
	CR_FD_IPCNS_VAR,
	CR_FD_IPCNS_SHM,
	CR_FD_IPCNS_MSG,
	CR_FD_IPCNS_SEM,
	CR_FD_MOUNTPOINTS,
	_CR_FD_NS_TO,

	CR_FD_PSTREE,
	CR_FD_SHMEM_PAGES,
	CR_FD_GHOST_FILE,
	CR_FD_TCP_STREAM,

	_CR_FD_GLOB_FROM,
	CR_FD_SK_QUEUES,
	CR_FD_REG_FILES,
	CR_FD_INETSK,
	CR_FD_UNIXSK,
	CR_FD_PIPES,
	CR_FD_PIPES_DATA,
	CR_FD_REMAP_FPATH,
	CR_FD_EVENTFD,
	CR_FD_EVENTPOLL,
	CR_FD_EVENTPOLL_TFD,
	CR_FD_INOTIFY,
	CR_FD_INOTIFY_WD,
	_CR_FD_GLOB_TO,

	CR_FD_MAX
};

struct cr_options {
	int			final_state;
	char			*show_dump_file;
	bool			leader_only;
	bool			show_pages_content;
	bool			restore_detach;
	bool			ext_unix_sk;
	bool			tcp_established_ok;
	unsigned int		namespaces_flags;
	bool			log_file_per_pid;
	char			*output;
};

extern struct cr_options opts;

enum {
	LOG_FD_OFF = 1,
	IMG_FD_OFF,
	SELF_EXE_FD_OFF,
};

int get_service_fd(int type);

/* file descriptors template */
struct cr_fd_desc_tmpl {
	const char	*fmt;			/* format for the name */
	u32		magic;			/* magic in the header */
	void		(*show)(int fd, struct cr_options *o);
};

void show_files(int fd_files, struct cr_options *o);
void show_pages(int fd_pages, struct cr_options *o);
void show_reg_files(int fd_reg_files, struct cr_options *o);
void show_core(int fd_core, struct cr_options *o);
void show_mm(int fd_mm, struct cr_options *o);
void show_vmas(int fd_vma, struct cr_options *o);
void show_pipes(int fd_pipes, struct cr_options *o);
void show_pipes_data(int fd_pipes, struct cr_options *o);
void show_pstree(int fd_pstree, struct cr_options *o);
void show_sigacts(int fd_sigacts, struct cr_options *o);
void show_itimers(int fd, struct cr_options *o);
void show_creds(int fd, struct cr_options *o);
void show_fs(int fd, struct cr_options *o);
void show_remap_files(int fd, struct cr_options *o);
void show_ghost_file(int fd, struct cr_options *o);
void show_fown_cont(fown_t *fown);
void show_eventfds(int fd, struct cr_options *o);

extern void print_data(unsigned long addr, unsigned char *data, size_t size);
extern struct cr_fd_desc_tmpl fdset_template[CR_FD_MAX];

extern int open_image_dir(void);
extern void close_image_dir(void);

int open_image(int type, unsigned long flags, ...);
#define open_image_ro(type, ...) open_image(type, O_RDONLY, ##__VA_ARGS__)

#define LAST_PID_PATH		"/proc/sys/kernel/ns_last_pid"
#define LAST_PID_PERM		0666

struct cr_fdset {
	int fd_off;
	int fd_nr;
	int *_fds;
};

static inline int fdset_fd(const struct cr_fdset *fdset, int type)
{
	int idx;

	idx = type - fdset->fd_off;
	BUG_ON(idx > fdset->fd_nr);

	return fdset->_fds[idx];
}

extern struct cr_fdset *glob_fdset;
extern struct cr_options opts;

int cr_dump_tasks(pid_t pid, const struct cr_options *opts);
int cr_restore_tasks(pid_t pid, struct cr_options *opts);
int cr_show(struct cr_options *opts);
int convert_to_elf(char *elf_path, int fd_core);
int cr_check(void);

#define O_DUMP	(O_RDWR | O_CREAT | O_EXCL)
#define O_SHOW	(O_RDONLY)

struct cr_fdset *cr_task_fdset_open(int pid, int mode);
struct cr_fdset *cr_ns_fdset_open(int pid, int mode);
struct cr_fdset *cr_glob_fdset_open(int mode);

void close_cr_fdset(struct cr_fdset **cr_fdset);

void free_mappings(struct list_head *vma_area_list);

struct vma_area {
	struct list_head	list;
	struct vma_entry	vma;
	int			vm_file_fd;
};

#define vma_area_is(vma_area, s)	vma_entry_is(&((vma_area)->vma), s)
#define vma_area_len(vma_area)		vma_entry_len(&((vma_area)->vma))

struct rst_info {
	struct list_head	fds;
	struct list_head	eventpoll;
};

struct pid
{
	u32 real_pid;
	u32 pid;
};

struct pstree_item {
	struct list_head	list;
	struct pid		pid;		/* leader pid */
	struct pstree_item	*parent;
	struct list_head	children;	/* array of children */
	pid_t			pgid;
	pid_t			sid;
	int			state;		/* TASK_XXX constants */
	int			nr_threads;	/* number of threads */
	struct pid		*threads;	/* array of threads */
	struct rst_info		rst[0];
};

extern struct pstree_item *__alloc_pstree_item(bool rst);
#define alloc_pstree_item() __alloc_pstree_item(false)
#define alloc_pstree_item_with_rst() __alloc_pstree_item(true)
extern struct pstree_item *pstree_item_next(struct pstree_item *item);

#define for_each_pstree_item(pi) \
	for (pi = root_item; pi != NULL; pi = pstree_item_next(pi))

static inline int in_vma_area(struct vma_area *vma, unsigned long addr)
{
	return addr >= (unsigned long)vma->vma.start &&
		addr < (unsigned long)vma->vma.end;
}

#endif /* CRTOOLS_H_ */
