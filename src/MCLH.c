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
    int rc = 0;
    struct fx_opt *fx_opt = fx_opt_worker(worker);

    sprintf(path, "%s/%d/dir", fx_opt->root, worker->id);
    rc = mkdir_p(path);
    if (rc) goto err_out;
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
	    sprintf(file, "%s/%d/dir/n-%" PRIu64 ".dat", fx_opt->root,worker->id,iter);
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

struct bench_operations create_l_h_ops = {
	.pre_work  = pre_work, 
	.main_work = main_work,
};
