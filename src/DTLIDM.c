/**
 * Nanobenchmark: block write
 *   BW. PROCESS = {ovewrite file at /test/$PROCESS}
 *       - TEST: ideal, no conention
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
    char test_path[PATH_MAX];
    char cmd[PATH_MAX];
	char file[PATH_MAX];
	int fd = -1, rc = 0;
    int ncpu = bench->ncpu;
    struct fx_opt *fx_opt = fx_opt_worker(worker);
    char *page = NULL;

    /*Allocate aligned buffer*/
    if(posix_memalign((void **)&(worker->page), PAGE_SIZE, PAGE_SIZE))
        goto err_out;
    page = worker->page;
    if (!page)
        goto err_out;
    
    //create snapshot
    snprintf(cmd,PATH_MAX,"sudo dmsetup message /dev/mapper/hchen-thin-pool 0 \"create_snap %d 0\"",worker->id + 1);
    if(system(cmd))
        goto err_out;
    snprintf(cmd,PATH_MAX,"sudo dmsetup create mysnap%d --table \"0 41943040 thin /dev/mapper/hchen-thin-pool %d\"",worker->id + 1, worker->id + 1);
    if(system(cmd))
        goto err_out;

    //test path
    sprintf(test_path, "%s/%d", fx_opt->root, worker->id);
    rc = mkdir_p(test_path);
    if (rc) goto err_out;
    sprintf(test_path, "%s/%dsnap", fx_opt->root, worker->id);
    rc = mkdir_p(test_path);
    if (rc) goto err_out;

    snprintf(cmd,PATH_MAX,"sudo mount /dev/mapper/mysnap%d %s/%d",worker->id + 1, fx_opt->root, worker->id);
    if(system(cmd))
        goto err_out;

    /* create a test file */
    snprintf(file, PATH_MAX, "%s/%d/n_file_rd.dat", fx_opt->root,worker->id);
    if ((fd = open(file, O_CREAT | O_RDWR | O_LARGEFILE, S_IRWXU)) == -1)
        goto err_out;
    
    for(;worker->private[0] < 100000;++worker->private[0]) {
      rc = write(fd, page, PAGE_SIZE);
      if (rc != PAGE_SIZE) {
        if (errno == ENOSPC) {
          --worker->private[0];
          rc = 0;
          break;
          //goto out;
        }
        goto err_out;
      }
      rc = 0;
    }
    
    close(fd);
    sync();

    snprintf(cmd,PATH_MAX,"sudo dmsetup message /dev/mapper/hchen-thin-pool 0 \"create_snap %d %d\"", ncpu + worker->id + 1, worker->id + 1);
    if(system(cmd))
        goto err_out;
    snprintf(cmd,PATH_MAX,"sudo dmsetup create mysnapsnap%d --table \"0 41943040 thin /dev/mapper/hchen-thin-pool %d\"",worker->id + 1, ncpu + worker->id + 1);
    if(system(cmd))
        goto err_out;
    
    snprintf(cmd,PATH_MAX,"sudo mount /dev/mapper/mysnapsnap%d %s/%dsnap",worker->id + 1, fx_opt->root, worker->id);
    if(system(cmd))
        goto err_out;

    snprintf(file, PATH_MAX, "%s/%dsnap/n_file_rd.dat", fx_opt->root,worker->id);
    if ((fd = open(file, O_RDWR | O_LARGEFILE, S_IRWXU)) == -1)
        goto err_out;
    
    /* set flag with O_DIRECT if necessary*/
    if(bench->directio && (fcntl(fd, F_SETFL, O_DIRECT) == -1))
        goto err_out;

out:
    /* put fd to worker's private */
    worker->private[1] = (uint64_t)fd;
    free(page);
    worker->page=NULL;
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

    snprintf(cmd,PATH_MAX,"sudo umount %s/%dsnap",fx_opt->root, worker->id);
    if(system(cmd))
        goto err_out;
    snprintf(cmd,PATH_MAX,"sudo dmsetup remove mysnapsnap%d",worker->id + 1);
    if(system(cmd))
        goto err_out;
    snprintf(cmd,PATH_MAX,"sudo dmsetup message /dev/mapper/hchen-thin-pool 0 \"delete %d\"",worker->bench->ncpu + worker->id + 1);
    if(system(cmd))
        goto err_out;
    snprintf(cmd,PATH_MAX,"sudo umount %s/%d",fx_opt->root, worker->id);
    if(system(cmd))
        goto err_out;
    snprintf(cmd,PATH_MAX,"sudo dmsetup remove mysnap%d",worker->id + 1);
    if(system(cmd))
        goto err_out;
    snprintf(cmd,PATH_MAX,"sudo dmsetup message /dev/mapper/hchen-thin-pool 0 \"delete %d\"",worker->id + 1);
    if(system(cmd))
        goto err_out;
out:
    return rc;
err_out:
    rc = errno;
    goto out;
}

struct bench_operations truncate_l_i_dm_ops = {
	.pre_work  = pre_work, 
	.main_work = main_work,
    .post_work = post_work,
};
