#include <iostream>
#include <thread>
#include <exception>
#include <unistd.h>
#include <csignal>

#include "Parallel.hpp"

using std::vector;
using std::function;
using std::thread;
using std::cerr;
using std::endl;
using std::atomic;
using std::atomic_flag;

atomic<bool> stopping { false };

atomic_flag status_request = ATOMIC_FLAG_INIT;

thread_local int worker_id { -1 };

static void on_info(int signo)
{
	(void) signo;
	status_request.clear();
}

static void on_exit(int signo)
{
	(void) signo;
	if (stopping) {
		std::abort();
	}
	stopping = true;
}

static void worker_wrapper(int id, function<void()> task)
{
	worker_id = id;
	task();
}

void run_parallel(int worker_count, function<void()> task)
{
	status_request.test_and_set();

	/* Thread pool */
	vector<thread> workers;
	workers.reserve(worker_count);

	cerr << "Installing signal handlers..." << endl;

	std::signal(SIGHUP, on_info);

	std::signal(SIGINT, on_exit);
	std::signal(SIGTERM, on_exit);

	cerr << "Starting " << worker_count << " workers..." << endl;

	/* Start workers */
	for (int i = 0; i < worker_count; i++) {
		workers.emplace_back(worker_wrapper, i, task);
	}

	cerr << "Started." << endl;

	/* Wait for user to request quit */
	if (isatty(STDIN_FILENO)) {
		cerr << endl << "Press <Ctrl+C> to stop." << endl << endl;
		usleep(500000);
	}

	/* Loop until stop requested by signal */
	while (!stopping) {
		usleep(100000);
	}

	/* Co-operatively request workers to stop */
	stopping = true;

	cerr << "Stopping..." << endl;

	/* Wait for workers to stop */
	for (auto& worker : workers) {
		worker.join();
	}

	cerr << "Stopped." << endl;
}
