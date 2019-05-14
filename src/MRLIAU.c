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
#define __STDC_FORMAT_MACROS
#include <stdlib.h>
#include <inttypes.h>
#include "fxmark.h"
#include "util.h"

static int pre_work(struct worker *worker)
{
	struct bench *bench = worker->bench;
    char path_upper[PATH_MAX];
    char path_lower[PATH_MAX];
    char path_merged[PATH_MAX];
    char cmd[PATH_MAX];
	char file[PATH_MAX];
	int fd, rc = 0;
    int ncpu = bench->ncpu;
    int total = TOTAL_INODES / ncpu;
    struct fx_opt *fx_opt = fx_opt_worker(worker);

    //upper
    sprintf(path_upper, "%s/%d/upper", fx_opt->root, worker->id);
    rc = mkdir_p(path_upper);
    if (rc) goto err_out;

    //lower
    sprintf(path_lower, "%s/%d/lower", fx_opt->root, worker->id);
    rc = mkdir_p(path_lower);
    if (rc) goto err_out;

    //merged
    sprintf(path_merged, "%s/%d/merged", fx_opt->root, worker->id);
    rc = mkdir_p(path_merged);
    if (rc) goto err_out;

	/* create files at the private directory */
    for(;worker->private[0] < total;++worker->private[0]){
	    sprintf(file, "%s/%d/lower/n_dir_rd-%d-%" PRIu64 ".dat", fx_opt->root,worker->id,worker->id,worker->private[0]);
		if ((fd = open(file, O_CREAT | O_RDWR, S_IRWXU)) == -1) {
			if (errno == ENOSPC)
				goto out;
			goto err_out;
		}
		close(fd);
	}
out:
	/* mount before return */
    if(bench->stop != 1){
        snprintf(cmd,PATH_MAX,"sudo mount -t aufs -o dirs=%s:%s=ro none %s",path_upper,path_lower,path_merged);
        if(system(cmd))
            goto err_out;
    }
	return rc; 
err_out:
    bench->stop = 1;
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

	sprintf(dir_path, "%s/%d/merged/", fx_opt->root,worker->id);
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
    char path_merged[PATH_MAX];
    struct fx_opt *fx_opt = fx_opt_worker(worker);

    sprintf(path_merged, "%s/%d/merged", fx_opt->root, worker->id);
    snprintf(cmd,PATH_MAX,"sudo umount %s",path_merged);
    if(system(cmd))
        goto err_out;
out:
    return rc;
err_out:
    rc = errno;
    goto out;
}

struct bench_operations readdir_l_i_au_ops = {
	.pre_work  = pre_work, 
	.main_work = main_work,
    .post_work = post_work,
};
