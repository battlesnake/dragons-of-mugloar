#include <iostream>
#include <thread>

#include "Parallel.hpp"

using std::vector;
using std::function;
using std::thread;
using std::cerr;
using std::endl;
using std::atomic;

atomic<bool> stopping { false };

void run_parallel(int worker_count, function<void(int)> task)
{
	/* Thread pool */
	vector<thread> workers;
	workers.reserve(worker_count);

	cerr << "Starting " << worker_count << " workers..." << endl;

	/* Start workers */
	for (int i = 0; i < worker_count; i++) {
		workers.emplace_back(task, i);
	}

	cerr << "Started." << endl;

	/* Wait for user to request quit */
	do {
		cerr << "Press <q> <ENTER> to stop." << endl;
	} while (getchar() != 'q');

	/* Co-operatively request workers to stop */
	stopping = true;

	cerr << "Stopping..." << endl;

	/* Wait for workers to stop */
	for (auto& worker : workers) {
		worker.join();
	}

	cerr << "Stopped." << endl;
}
