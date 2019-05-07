#pragma GCC diagnostic ignored "-Wcomment"
/**
 * Nanobenchmark: META
 *   RN. PROCESS = {rename a file name in a private directory}
 *       - TEST: concurrent update of dentry cache
 */	      
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "fxmark.h"
#include "util.h"

static void set_test_root(struct worker *worker, char *test_root)
{
	struct fx_opt *fx_opt = fx_opt_worker(worker);
	sprintf(test_root, "%s/%d", fx_opt->root, worker->id);
}

static void set_test_file(struct worker *worker, 
			  uint64_t file_id, char *test_file)
{
	struct fx_opt *fx_opt = fx_opt_worker(worker);
	sprintf(test_file, "%s/%d/n_file_rename-%" PRIu64 ".dat",
		fx_opt->root, worker->id, file_id);
}

static int pre_work(struct worker *worker)
{
	char path_upper[PATH_MAX];
    char path_lower[PATH_MAX];
    char path_work[PATH_MAX];
    char path_merged[PATH_MAX];
    char test_path[PATH_MAX];
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

    sprintf(test_path, "%s/%d/upper/dir1", fx_opt->root, worker->id);
    rc = mkdir_p(test_path);
    if (rc) goto err_out;

    sprintf(test_path, "%s/%d/upper/dir2", fx_opt->root, worker->id);
    rc = mkdir_p(test_path);
    if (rc) goto err_out;

	/* create files at the private directory */
	sprintf(test_path, "%s/%d/upper/dir1/n_file_rename-%" PRIu64 ".dat",
		fx_opt->root, worker->id, file_id);
	if ((fd = open(path, O_CREAT | O_RDWR, S_IRWXU)) == -1)
		goto err_out;
	fsync(fd);
	close(fd);

	/* mount before return */
    snprintf(cmd,PATH_MAX,"sudo mount -t overlay overlay -olowerdir=%s,upperdir=%s,workdir=%s %s",path_lower,path_upper,path_work,path_merged);
    if(system(cmd))
        goto err_out;

out:
	return rc;
err_out:
	rc = errno;
	goto out;
}

static int main_work(struct worker *worker)
{
	struct bench *bench = worker->bench;
	char old_path[PATH_MAX], new_path[PATH_MAX];
	uint64_t iter;
	int rc = 0;

	sprintf(old_path, "%s/%d/merged/dir1/n_file_rename-%" PRIu64 ".dat",
		fx_opt->root, worker->id, file_id);
	sprintf(new_path, "%s/%d/merged/dir2/n_file_rename-%" PRIu64 ".dat",
		fx_opt->root, worker->id, file_id);
	for (iter = 0; !bench->stop; ++iter) {
		if(iter % 2 == 0)
			rc = rename(old_path, new_path);
		else
			rc = rename(new_path, old_path);
		if (rc) goto err_out;
	}
out:
	bench->stop = 1;
	worker->works = (double)iter;
	return rc;
err_out:
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

struct bench_operations rename_l_c_ops = {
	.pre_work  = pre_work, 
	.main_work = main_work,
	.post_work = post_work,
};
