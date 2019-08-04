#pragma once
#include <functional>
#include <atomic>

/* Set when we want the workers to exit */
extern std::atomic<bool> stopping;

void run_parallel(int worker_count, std::function<void(int)> task);
