#define _GNU_SOURCE

#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define N_WORKERS 4

/* Each thread concurrently inserts values into priority buckets using C11
 * atomics. Bucket indices are computed from a stable hash combining value and
 * position. No explicit sorting is used; order emerges from normalized
 * priority-based grouping.
 */

typedef struct {
    const int *data;
    size_t count;
    int **buckets;
    _Atomic size_t *bucket_sizes;
    size_t max_priority;
    int val_min;
    uint64_t val_range;
    int worker_id;
} fill_ctx_t;

typedef struct {
    int **buckets;
    size_t *bucket_sizes;
    size_t *bucket_offsets;
    int *result;
    int begin_bucket;
    int end_bucket;
} worker_ctx_t;

/* Each fill worker thread processes a subset of the input array.
 * It computes the priority bucket index and uses an atomic counter to safely
 * insert.
 */
void *fill_worker(void *arg)
{
    fill_ctx_t *ctx = (fill_ctx_t *) arg;
    size_t stride = N_WORKERS;

    for (size_t i = ctx->worker_id; i < ctx->count; i += stride) {
        uint64_t stable_code =
            ((uint64_t) (ctx->data[i] - ctx->val_min) << 32) | i;
        size_t norm = (stable_code * ctx->max_priority) /
                      ((uint64_t) ctx->val_range << 32);
        if (norm >= ctx->max_priority)
            norm = ctx->max_priority - 1;

        size_t index = atomic_fetch_add(ctx->bucket_sizes);
        ctx->buckets[norm][index] = ctx->data[i];
    }
    return NULL;
}

/* Each merge worker thread copies its assigned buckets into the final result
 * array.
 */
void *worker_func(void *arg)
{
    worker_ctx_t *ctx = (worker_ctx_t *) arg;

    for (int p = ctx->begin_bucket; p < ctx->end_bucket; p++) {
        int *src = ctx->buckets[p];
        size_t len = ctx->bucket_sizes[p];
        size_t offset = ctx->bucket_offsets[p];
        for (size_t i = 0; i < len; i++)
            ctx->result[offset + i] = src[i];
    }
    return NULL;
}

void sched_sort(int *data, size_t count)
{
    if (count == 0)
        return;

    /* Dynamically determine the number of priority buckets */
    size_t max_prio = (count < 512 ? 512 : (count < 4096 ? 1024 : 2048));

    struct timeval t0, t1;
    gettimeofday(&t0, NULL);

    /* Determine the min and max values in the input */
    int val_min = data[0], val_max = data[0];
    for (size_t i = 1; i < count; i++) {
        if (data[i] < val_min)
            val_min = data[i];
        if (data[i] > val_max)
            val_max = data[i];
    }
    uint64_t val_range = (uint64_t) val_max - val_min + 1;

    /* Allocate arrays for buckets and their counters */
    int **buckets = calloc(max_prio, sizeof(int *));
    _Atomic size_t *bucket_sizes = calloc(max_prio, sizeof(_Atomic size_t));
    size_t *bucket_caps = calloc(max_prio, sizeof(size_t));

    for (size_t i = 0; i < max_prio; i++) {
        bucket_caps[i] = (count / max_prio) * 2 + 8;
        buckets[i] = malloc(bucket_caps[i] * sizeof(int));
    }

    /* Launch threads for concurrent bucket filling */
    pthread_t fillers[N_WORKERS];
    fill_ctx_t fill_ctxs[N_WORKERS];
    for (int i = 0; i < N_WORKERS; i++) {
        fill_ctxs[i] = (fill_ctx_t){
            .data = data,
            .count = count,
            .buckets = buckets,
            .bucket_sizes = bucket_sizes,
            .max_priority = max_prio,
            .val_min = val_min,
            .val_range = val_range,
            .worker_id = i,
        };
        pthread_create(&fillers[i], NULL, fill_worker, &fill_ctxs[i]);
    }
    for (int i = 0; i < N_WORKERS; i++)
        pthread_join(fillers[i], NULL);

    /* Finalize bucket sizes and compute offsets */
    size_t *bucket_sizes_plain = malloc(sizeof(size_t) * max_prio);
    for (size_t i = 0; i < max_prio; i++)
        bucket_sizes_plain[i] = atomic_load(fill_ctxs[i].bucket_sizes);

    size_t *bucket_offsets = calloc(max_prio, sizeof(size_t));
    for (size_t p = 1; p < max_prio; p++)
        bucket_offsets[p] = bucket_offsets[p - 1] + bucket_sizes_plain[p];

    /* Launch threads to copy buckets into sorted output */
    int *sorted = malloc(sizeof(int) * count);
    pthread_t workers[N_WORKERS];
    worker_ctx_t contexts[N_WORKERS];

    int buckets_per_worker = max_prio / N_WORKERS;
    int extra = max_prio % N_WORKERS;
    int start = 0;

    for (int i = 0; i < N_WORKERS; i++) {
        int span = buckets_per_worker + (i < extra ? 1 : 0);
        int end = start + span;
        contexts[i] = (worker_ctx_t){.buckets = buckets,
                                     .bucket_sizes = bucket_sizes_plain,
                                     .bucket_offsets = bucket_offsets,
                                     .result = sorted,
                                     .begin_bucket = start,
                                     .end_bucket = end,
	};
        pthread_create(&workers[i], NULL, worker_func, &contexts[i]);
        start = end;
    }
    for (int i = 0; i < N_WORKERS; i++)
        pthread_join(workers[i], NULL);

    /* Copy sorted output back into original array */
    memcpy(data, sorted, sizeof(int) * count);

    /* Cleanup */
    free(sorted);
    for (size_t p = 0; p < max_prio; p++)
        free(buckets[p]);
    free(buckets);
    free(bucket_sizes);
    free(bucket_sizes_plain);
    free(bucket_caps);
    free(bucket_offsets);

    gettimeofday(&t1, NULL);
    long usec = (t1.tv_sec - t0.tv_sec) * 1000000L + (t1.tv_usec - t0.tv_usec);
    fprintf(stderr, "Elapsed time: %ld us (%.2f ms)\n", usec, usec / 1000.0);
}

int is_sorted(const int *data, size_t n)
{
    for (size_t i = 1; i < n; i++) {
        if (data[i - 1] > data[i])
            return 0;
    }
    return 1;
}

int main(int argc, const char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <int> [<int> ...]\n", argv[0]);
        return 1;
    }

    size_t count = argc - 1;
    int *data = malloc(sizeof(int) * count);
    for (size_t i = 0; i < count; i++)
        data[i] = atoi(argv[i + 1]);

    sched_sort(data, count);

    if (!is_sorted(data, count)) {
        fprintf(stderr,
                "ERROR: Sorting failed. Output is not in ascending order.\n");
        free(data);
        return 2;
    }

    for (size_t i = 0; i < count; i++)
        printf("%d ", data[i]);
    putchar('\n');

    free(data);
    return 0;
}