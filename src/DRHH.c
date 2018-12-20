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
        char path_upper[PATH_MAX];
        char path_lower[PATH_MAX];
        char path_work[PATH_MAX];
        char path_merged[PATH_MAX];
        char file[PATH_MAX];
        int fd = -1, rc = 0;
        struct fx_opt *fx_opt = fx_opt_worker(worker);

        /*Allocate aligned buffer*/
        if(posix_memalign((void **)&(worker->page), PAGE_SIZE, PAGE_SIZE))
                goto err_out;
        page = worker->page;
        if (!page)
                goto err_out;

        //upper
        sprintf(path_upper, "%s/%d/upper", fx_opt->root, worker->id);
        rc = mkdir_p(path_upper);
        if (rc) goto err_out;

        //merged, work
        sprintf(path_merged, "%s/%d/merged", fx_opt->root, worker->id);
        rc = mkdir_p(path_merged);
        if (rc) goto err_out;
        sprintf(path_work, "%s/%d/work", fx_opt->root, worker->id);
        rc = mkdir_p(path_work);
        if (rc) goto err_out;

        /* a leader takes over all pre_work() */
        if (worker->id != 0)
                goto out;

        //lower
        sprintf(path_lower, "%s/lower", fx_opt->root);
        rc = mkdir_p(path_lower);
        if (rc) goto err_out;

        /* create a test file */
        snprintf(file, PATH_MAX, "%s/lower/n_shblk_rd.dat", fx_opt->root);
        if ((fd = open(file, O_CREAT | O_RDWR, S_IRWXU)) == -1)
                goto err_out;

        /*set flag with O_DIRECT if necessary*/
        if(bench->directio && (fcntl(fd, F_SETFL, O_DIRECT)==-1))
                goto err_out;

        if (write(fd, page, PAGE_SIZE) != PAGE_SIZE)
                goto err_out;

        fsync(fd);
        close(fd);
out:
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
        char path[PATH_MAX];
        char *page=worker->page;
        int fd, rc = 0;
        uint64_t iter = 0;
        char file[PATH_MAX];
        struct fx_opt *fx_opt = fx_opt_worker(worker);

        assert(page);

        snprintf(path, PATH_MAX, "%s/lower/n_shblk_rd.dat", fx_opt->root);
        if ((fd = open(path, O_RDONLY, S_IRWXU)) == -1)
                goto err_out;

        /*set flag with O_DIRECT if necessary*/
        if(bench->directio && (fcntl(fd, F_SETFL, O_DIRECT)==-1))
                goto err_out;

        for (iter = 0; !bench->stop; ++iter) {
                if (pread(fd, page, PAGE_SIZE, 0) != PAGE_SIZE)
                        goto err_out;
        }
        close(fd);
out:
        worker->works = (double)iter;
        return rc;
err_out:
        bench->stop = 1;
        rc = errno;
        free(page);
        goto out;
}

struct bench_operations read_h_h_ops = {
        .pre_work  = pre_work,
        .main_work = main_work,
};
