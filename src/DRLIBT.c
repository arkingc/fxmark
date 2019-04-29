/**
 * Nanobenchmark: Read operation
 *   RSF. PROCESS = {read the same page of /test/test.file}
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
        char file[PATH_MAX];
        char cmd[PATH_MAX];
        int fd = -1, rc = 0;
        struct fx_opt *fx_opt = fx_opt_worker(worker);

        /*Allocate aligned buffer*/
        if(posix_memalign((void **)&(worker->page), PAGE_SIZE, PAGE_SIZE))
                goto err_out;
        page = worker->page;
        if (!page)
                goto err_out;

        snprintf(cmd,PATH_MAX,"sudo btrfs subvolume snapshot %s/subv %s/%d",fx_opt->root, fx_opt->root, worker->id);
        if(system(cmd))
            goto err_out;

        snprintf(file, PATH_MAX, "%s/%d/n_file_rd.dat", fx_opt->root,worker->id);
        if ((fd = open(file, O_CREAT | O_RDWR, S_IRWXU)) == -1)
                goto err_out;

        if (write(fd, page, PAGE_SIZE) != PAGE_SIZE)
                goto err_out;

        close(fd);
        sync();

        snprintf(cmd,PATH_MAX,"sudo btrfs subvolume snapshot %s/%d %s/%dsnap",fx_opt->root, worker->id, fx_opt->root, worker->id);
        if(system(cmd))
            goto err_out;

        snprintf(file, PATH_MAX, "%s/%dsnap/n_file_rd.dat", fx_opt->root,worker->id);
        if ((fd = open(file, O_RDWR, S_IRWXU)) == -1)
            goto err_out;

        /*set flag with O_DIRECT if necessary*/
        if(bench->directio && (fcntl(fd, F_SETFL, O_DIRECT)==-1))
                goto err_out;

out:
        /* put fd to worker's private */
        worker->private[0] = (uint64_t)fd;
        return rc;
err_out:
        rc = errno;
        if(page)
                free(page);
        goto out;
}

static int main_work(struct worker *worker)
{
        struct bench *bench = worker->bench;
        int fd = -1, rc = 0;
        uint64_t iter = 0;
        char *page=worker->page;

        assert(page);
        
        fd = (int)worker->private[0];
        for (iter = 0; !bench->stop; ++iter) {
        if (pread(fd, page, PAGE_SIZE, 0) != PAGE_SIZE)
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

struct bench_operations read_l_i_bt_ops = {
        .pre_work  = pre_work,
        .main_work = main_work,
        .post_work = post_work,
};
