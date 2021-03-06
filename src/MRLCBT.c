/**
 * Nanobenchmark: Read operation
 *   RD. PROCESS = {read entries of its private directory}
 */	      
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "fxmark.h"
#include "util.h"

static int pre_work(struct worker *worker)
{
	struct bench *bench = worker->bench;
    char path[PATH_MAX];
	char file[PATH_MAX];
	char cmd[PATH_MAX];
	int fd, rc = 0;
    int ncpu = bench->ncpu;
    int total = TOTAL_INODES / ncpu;
    struct fx_opt *fx_opt = fx_opt_worker(worker);

    snprintf(cmd,PATH_MAX,"sudo btrfs subvolume snapshot %s/subv %s/%d",fx_opt->root, fx_opt->root, worker->id);
    if(system(cmd))
        goto err_out;

    sprintf(path, "%s/%d/dir", fx_opt->root, worker->id);
    rc = mkdir_p(path);
    if (rc) goto err_out;

	/* create files at the private directory */
    for(;worker->private[0] < total;++worker->private[0]){
	    sprintf(file, "%s/%d/dir/n_dir_rd-%d-%" PRIu64 ".dat", fx_opt->root,worker->id,worker->id,worker->private[0]);
		if ((fd = open(file, O_CREAT | O_RDWR, S_IRWXU)) == -1) {
			if (errno == ENOSPC)
				goto out;
			goto err_out;
		}
		close(fd);
	}
out:
	return rc; 
err_out:
	rc = errno;
	goto out;
}

static int main_work(struct worker *worker)
{
	struct bench *bench = worker->bench;
	char dir_path[PATH_MAX];
	DIR *dir;
	struct dirent entry;
	struct dirent *result;
	uint64_t iter = 0;
	int rc = 0;
    struct fx_opt *fx_opt = fx_opt_worker(worker);

	sprintf(dir_path, "%s/%d/dir/", fx_opt->root,worker->id);
	while (!bench->stop) {
		dir = opendir(dir_path);
		if (!dir) goto err_out;
		for (; !bench->stop; ++iter) {
			rc = readdir_r(dir, &entry, &result);
			if (rc) goto err_out;
		}
		closedir(dir);
	}
out:
	bench->stop = 1;
	worker->works = (double)iter;
	return rc;
err_out:
	rc = errno;
	goto out;
}

static int post_work(struct worker *worker){
    int rc = 0;
    char cmd[PATH_MAX];
    struct fx_opt *fx_opt = fx_opt_worker(worker);

    snprintf(cmd,PATH_MAX,"sudo btrfs subvolume delete %s/%d",fx_opt->root, worker->id);
    if(system(cmd))
        goto err_out;
out:
    return rc;
err_out:
    rc = errno;
    goto out;
}

struct bench_operations readdir_l_c_bt_ops = {
	.pre_work  = pre_work, 
	.main_work = main_work,
	.post_work = post_work,
};
