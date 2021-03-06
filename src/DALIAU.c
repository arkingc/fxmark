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
  	char *page = NULL;
    char path_upper[PATH_MAX];
    char path_lower[PATH_MAX];
    char path_merged[PATH_MAX];
	char file[PATH_MAX];
    char cmd[PATH_MAX];
	int fd = -1, rc = 0;
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

	/* create a test file */ 
	snprintf(file, PATH_MAX, "%s/%d/lower/n_blk_alloc-%d.dat", 
		 fx_opt->root, worker->id, worker->id);

	if ((fd = open(file, O_CREAT | O_RDWR | O_LARGEFILE, S_IRWXU)) == -1)
	  goto err_out;
	
    fsync(fd);
    close(fd);

    /* allocate data buffer aligned with pagesize*/
	if(posix_memalign((void **)&(worker->page), PAGE_SIZE, PAGE_SIZE))
	  goto err_out;
	page = worker->page;
	if (!page)
		goto err_out;

    /* mount before return */
    snprintf(cmd,PATH_MAX,"sudo mount -t aufs -o dirs=%s:%s=ro none %s",path_upper,path_lower,path_merged);
    if(system(cmd))
        goto err_out;
	
    /* open the test file */ 
	snprintf(file, PATH_MAX, "%s/%d/merged/n_blk_alloc-%d.dat", 
		 fx_opt->root, worker->id, worker->id);
	
    if ((fd = open(file, O_RDWR | O_LARGEFILE, S_IRWXU)) == -1)
	  goto err_out;
	
    /*set flag with O_DIRECT if necessary*/
	if(bench->directio && (fcntl(fd, F_SETFL, O_DIRECT)==-1))
	  goto err_out;
out:
    /* put fd to worker's private */
    worker->private[0] = (uint64_t)fd;
	return rc;
err_out:
	bench->stop = 1;
	rc = errno;
	free(page);
	goto out;
}

static int main_work(struct worker *worker)
{
  	char *page = worker->page;
	struct bench *bench = worker->bench;
	int fd = -1, rc = 0;
	uint64_t iter = 0;

	assert(page);

    fd = (int)worker->private[0];
    /* append */
	for (iter = 0; iter < 100000 && !bench->stop; ++iter) {
	        if (write(fd, page, PAGE_SIZE) != PAGE_SIZE)
			goto err_out;
	}
out:
	close(fd);
	worker->works = (double)iter;
	return rc;
err_out:
	bench->stop = 1;
	rc = errno;
    free(page);
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

struct bench_operations append_l_i_au_ops = {
	.pre_work  = pre_work, 
	.main_work = main_work,
    .post_work = post_work,
};
