#pragma once
/*
 * Provides means to run a task multiple times concurrently in worker-threads,
 * and to request those threads to quit at the user's request.
 */
#include <functional>
#include <atomic>

/* Set when we want the workers to exit */
extern std::atomic<bool> stopping;

/* Status update requested (check with test_and_set, false = print update) */
extern std::atomic_flag status_request;

/* Worker index */
extern thread_local int worker_id;

/* Run multiple instances of task in separate threads, wait for use to request exit */
void run_parallel(int worker_count, std::function<void()> task);
