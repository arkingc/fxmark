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
    char path_upper[PATH_MAX];
    char path_lower[PATH_MAX];
    char path_work[PATH_MAX];
    char path_merged[PATH_MAX];
    char test_path[PATH_MAX];
    char cmd[PATH_MAX];
	int rc = 0;
    int ncpu = bench->ncpu;
    int total = TOTAL_INODES / ncpu / 2;
    struct fx_opt *fx_opt = fx_opt_worker(worker);

    //upper
    sprintf(path_upper, "%s/%d/upper", fx_opt->root, worker->id);
    rc = mkdir_p(path_upper);
    if (rc) goto err_out;

    //lower
    sprintf(path_lower, "%s/%d/lower", fx_opt->root, worker->id);
    rc = mkdir_p(path_lower);
    if (rc) goto err_out;

    //merged, work
    sprintf(path_merged, "%s/%d/merged", fx_opt->root, worker->id);
    rc = mkdir_p(path_merged);
    if (rc) goto err_out;
    sprintf(path_work, "%s/%d/work", fx_opt->root, worker->id);
    rc = mkdir_p(path_work);
    if (rc) goto err_out;

    for(;worker->private[0] < total;++worker->private[0]){
        sprintf(test_path, "%s/%d/lower/dir%lu", fx_opt->root, worker->id, worker->private[0]);
        rc = mkdir_p(test_path);
        if (rc) goto err_out;
    }

    /* mount before return */
    snprintf(cmd,PATH_MAX,"sudo mount -t overlay overlay -olowerdir=%s,upperdir=%s,workdir=%s %s",path_lower,path_upper,path_work,path_merged);
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
	int fd, rc = 0;
	uint64_t iter = 0;
    struct fx_opt *fx_opt = fx_opt_worker(worker);

    char file[PATH_MAX];
	for (iter = 0; iter < worker->private[0] && !bench->stop; ++iter) {
	    sprintf(file, "%s/%d/merged/dir%lu/file", fx_opt->root,worker->id,iter);
	    if ((fd = open(file, O_CREAT | O_RDWR, S_IRWXU)) == -1)
            goto err_out;
        close(fd);
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

struct bench_operations imc_ops = {
	.pre_work  = pre_work, 
	.main_work = main_work,
    .post_work = post_work,
};
