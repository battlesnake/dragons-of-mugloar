# "Dragons of Mugloar" challenge


![It's true](falcdt.jpg)


Requires for building:

 * GNU Make v4+

 * C++17 compiler (e.g. recent version of gcc / clang)

 * libcurl


Requires for running:

 * Internet connection

 * libcurl

 * ANSI/VT100-comptaible terminal for `mugcli` (i.e. not CMD.EXE or Minicom)

 * POSIX environment (Linux ideally, may also work on OSX)


To build:

    make -j


To run the command-line interactive interface:

	./mugcli


To collect training data for the AI player, using 20 workers:

	./mugcollect -o training.csv -p 20


To train the artificial intelligence using the previously-collected data:

	./TODO_NOT_IMPLEMENTED_YET training.csv


To run the fully-automated luxury cromulent dragon trainer:

	./mugomatic neuralnet.csv


To delete the compiled binaries and intermediate files out:

    make clean


And recklessly nuking the repo has the usual form:

	git reset --hard
    git clean -fdx


