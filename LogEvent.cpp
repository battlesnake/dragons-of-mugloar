#include <mutex>
#include <iostream>
#include "LogEvent.hpp"

using std::unordered_map;
using std::string;
using std::ostream;
using std::mutex;
using std::scoped_lock;
using std::endl;
using std::cerr;

namespace mugloar
{

/* Emit event features to log file */
void log_event(ostream& f, const unordered_map<string, float>& entry)
{
	static mutex io_mutex;
	static size_t count = 0;
	scoped_lock lock(io_mutex);

	for (const auto& [key, value] : entry) {
		f << key << "\t" << value << "\t";
	}
	f << endl;

	++count;
	if (count % 100 == 0) {
		cerr << "Logged " << count << " entries" << endl;
	}
}

}
