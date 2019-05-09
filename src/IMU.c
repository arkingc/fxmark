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
	char file[PATH_MAX];
	char cmd[PATH_MAX];
	int fd = -1, rc = 0;
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

	/* create test file */
    for(;worker->private[0] < total;++worker->private[0]){
	    sprintf(file,"%s/%d/lower/n-%" PRIu64 ".dat", fx_opt->root,worker->id,worker->private[0]);
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
	int rc = 0;
	uint64_t iter = 0;
    struct fx_opt *fx_opt = fx_opt_worker(worker);

	for (iter = 0; iter < worker->private[0] && !bench->stop; ++iter) {
        char file[PATH_MAX];
	    sprintf(file, "%s/%d/merged/n-%" PRIu64 ".dat", fx_opt->root,worker->id,iter);
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

struct bench_operations imu_ops = {
	.pre_work  = pre_work, 
	.main_work = main_work,
    .post_work = post_work,
};
