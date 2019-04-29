/**
 * Nanobenchmark: ADD
 *   BA. PROCESS = {append files at /test/$PROCESS}
 *       - TEST: block alloc
 */	      
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdlib.h>
#include <assert.h>
#include "fxmark.h"
#include "util.h"

static int pre_work(struct worker *worker)
{
	struct bench *bench = worker->bench;
    char path[PATH_MAX];
	char file[PATH_MAX];
    char cmd[PATH_MAX];
	int fd = -1, rc = 0;
    int ncpu = bench->ncpu;
    int total = TOTAL_INODES / ncpu / 2;
    struct fx_opt *fx_opt = fx_opt_worker(worker);

	snprintf(cmd,PATH_MAX,"sudo btrfs subvolume snapshot %s/subv %s/%d",fx_opt->root, fx_opt->root, worker->id);
    if(system(cmd))
        goto err_out;

    sprintf(path, "%s/%d/dir", fx_opt->root, worker->id);
    rc = mkdir_p(path);
    if (rc) goto err_out;

	/* create test file */
    for(;worker->private[0] < total;++worker->private[0]){
	    sprintf(file,"%s/%d/dir/n-%" PRIu64 ".dat", fx_opt->root,worker->id,worker->private[0]);
	    if ((fd = open(file, O_CREAT | O_RDWR, S_IRWXU)) == -1)
        {
            if(errno == ENOSPC){
                --worker->private[0];
                rc = 0;
                goto out;
            }
            goto err_out;
        }
        close(fd);
    }

    snprintf(cmd,PATH_MAX,"sudo btrfs subvolume snapshot %s/%d %s/%dsnap",fx_opt->root, worker->id, fx_opt->root, worker->id);
    if(system(cmd))
        goto err_out;

out:
	return rc;
err_out:
	bench->stop = 1;
	rc = errno;
	goto out;
}

static int main_work(struct worker *worker)
{
	struct bench *bench = worker->bench;
	int rc = 0;
	uint64_t iter = 0;
    struct fx_opt *fx_opt = fx_opt_worker(worker);

	for (iter = 0; iter < worker->private[0] && !bench->stop; ++iter) {
        char file[PATH_MAX];
	    sprintf(file, "%s/%dsnap/dir/n-%" PRIu64 ".dat", fx_opt->root,worker->id,iter);
	    if(unlink(file))
            goto err_out;
    }
out:
	worker->works = (double)iter;
	return rc;
err_out:
	bench->stop = 1;
	rc = errno;
	goto out;
}

static int post_work(struct worker *worker){
    int rc = 0;
    char cmd[PATH_MAX];
    struct fx_opt *fx_opt = fx_opt_worker(worker);

    snprintf(cmd,PATH_MAX,"sudo btrfs subvolume delete %s/%dsnap",fx_opt->root, worker->id);
    if(system(cmd))
        goto err_out;
    snprintf(cmd,PATH_MAX,"sudo btrfs subvolume delete %s/%d",fx_opt->root, worker->id);
    if(system(cmd))
        goto err_out;
out:
    return rc;
err_out:
    rc = errno;
    goto out;
}

struct bench_operations unlink_l_i_bt_ops = {
	.pre_work  = pre_work, 
	.main_work = main_work,
	.post_work = post_work,
};
