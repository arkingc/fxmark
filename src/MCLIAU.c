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
    char path_merged[PATH_MAX];
    char test_path[PATH_MAX];
    char cmd[PATH_MAX];
	int rc = 0;
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

    sprintf(test_path, "%s/%d/lower/dir", fx_opt->root, worker->id);
    rc = mkdir_p(test_path);
    if (rc) goto err_out;

    /* mount before return */
    snprintf(cmd,PATH_MAX,"sudo mount -t aufs -o dirs=%s:%s=ro none %s",path_upper,path_lower,path_merged);
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

	for (iter = 0; !bench->stop; ++iter) {
        char file[PATH_MAX];
	    sprintf(file, "%s/%d/merged/dir/n-%" PRIu64 ".dat", fx_opt->root,worker->id,iter);
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

struct bench_operations create_l_i_au_ops = {
	.pre_work  = pre_work, 
	.main_work = main_work,
    .post_work = post_work,
};
