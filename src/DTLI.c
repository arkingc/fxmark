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
    char path_work[PATH_MAX];
    char path_merged[PATH_MAX];
	char file[PATH_MAX];
    char cmd[PATH_MAX];
	int fd, rc = 0;
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

    /* allocate data buffer aligned with pagesize*/
	if(posix_memalign((void **)&(worker->page), PAGE_SIZE, PAGE_SIZE))
	  goto err_out;
	page = worker->page;
	if (!page)
		goto err_out;

	/* create a test file */ 
	snprintf(file, PATH_MAX, "%s/%d/lower/u_file_tr-%d.dat", 
		 fx_opt->root, worker->id, worker->id);

	if ((fd = open(file, O_CREAT | O_RDWR | O_LARGEFILE, S_IRWXU)) == -1)
	  goto err_out;
	
    for(;;++worker->private[0]) {
      rc = write(fd, page, PAGE_SIZE);
      if (rc != PAGE_SIZE) {
        if (errno == ENOSPC) {
          --worker->private[0];
          worker->private[0] = worker->private[0] / 3;
          if (ftruncate(fd, worker->private[0] * PAGE_SIZE) == -1) {
            rc = errno;
            goto err_out;
          }
          rc = 0;
          goto out;
        }
        goto err_out;
      }
    }
    
    close(fd);
out:
	/* mount before return */
    if(bench->stop != 1){
        snprintf(cmd,PATH_MAX,"sudo mount -t overlay overlay -olowerdir=%s,upperdir=%s,workdir=%s %s",path_lower,path_upper,path_work,path_merged);
        if(system(cmd))
            goto err_out;
    }
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
	int fd, rc = 0;
	uint64_t iter = 0;
	char file[PATH_MAX];
    struct fx_opt *fx_opt = fx_opt_worker(worker);

	/* open the test file */ 
	snprintf(file, PATH_MAX, "%s/%d/merged/u_file_tr-%d.dat", 
		 fx_opt->root, worker->id, worker->id);
	
    if ((fd = open(file, O_RDWR | O_LARGEFILE, S_IRWXU)) == -1)
	  goto err_out;
	
	/*set flag with O_DIRECT if necessary*/
	if(bench->directio && (fcntl(fd, F_SETFL, O_DIRECT)==-1))
	  goto err_out;
    
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

struct bench_operations truncate_l_i_ops = {
	.pre_work  = pre_work, 
	.main_work = main_work,
    .post_work = post_work,
};
