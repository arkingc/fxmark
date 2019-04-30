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
    char cmd[PATH_MAX];
	char file[PATH_MAX];
	int fd = -1, rc = 0;
    struct fx_opt *fx_opt = fx_opt_worker(worker);

    snprintf(cmd,PATH_MAX,"sudo btrfs subvolume snapshot %s/subv %s/%d",fx_opt->root, fx_opt->root, worker->id);
    if(system(cmd))
        goto err_out;

    /* allocate data buffer aligned with pagesize*/
	if(posix_memalign((void **)&(worker->page), PAGE_SIZE, PAGE_SIZE))
	  goto err_out;
	page = worker->page;
	if (!page)
		goto err_out;

	/* create a test file */ 
	snprintf(file, PATH_MAX, "%s/%d/u_file_tr-%d.dat", 
		 fx_opt->root, worker->id, worker->id);
	if ((fd = open(file, O_CREAT | O_RDWR | O_LARGEFILE, S_IRWXU)) == -1)
	  goto err_out;
	
	/*set flag with O_DIRECT if necessary*/
	if(bench->directio && (fcntl(fd, F_SETFL, O_DIRECT)==-1))
	  goto err_out;
    
    for(worker->private[0] = 0; worker->private[0] < 100000; ++worker->private[0]) {
      rc = write(fd, page, PAGE_SIZE);
      if (rc != PAGE_SIZE) {
        if (errno == ENOSPC) {
          --worker->private[0];
          break;
          //goto out;
        }
        goto err_out;
      }
    }
    
    rc = 0;
out:
    /* put fd to worker's private */
    worker->private[1] = (uint64_t)fd;
    free(page);
    worker->page = NULL;
	return rc;
err_out:
	bench->stop = 1;
	rc = errno;
	goto out;
}

static int main_work(struct worker *worker)
{
	struct bench *bench = worker->bench;
	int fd = -1, rc = 0;
	uint64_t iter;

    fd = (int)worker->private[1];
    for (iter = --worker->private[0]; iter > 0 && !bench->stop; --iter) {
      if (ftruncate(fd, iter * PAGE_SIZE) == -1) {
        rc = errno;
        goto err_out;
      }
    }
out:
	close(fd);
	worker->works = (double)(worker->private[0] - iter);
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

    snprintf(cmd,PATH_MAX,"sudo btrfs subvolume delete %s/%d",fx_opt->root, worker->id);
    if(system(cmd))
        goto err_out;
out:
    return rc;
err_out:
    rc = errno;
    goto out;
}

struct bench_operations truncate_l_c_bt_ops = {
	.pre_work  = pre_work, 
	.main_work = main_work,
	.post_work = post_work,
};
