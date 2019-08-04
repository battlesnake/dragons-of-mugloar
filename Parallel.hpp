#pragma once
#include <functional>
#include <atomic>

/* Set when we want the workers to exit */
extern std::atomic<bool> stopping;

/* Run multiple instances of task in separate threads, wait for use to request exit */
void run_parallel(int worker_count, std::function<void(int)> task);
