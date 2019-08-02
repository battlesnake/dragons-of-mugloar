# "Dragons of Mugloar" challenge


![It's true](falcdt.jpg)


Requires for building:

 * GNU Make v4+

 * C++17 compiler (e.g. recent version of gcc / clang)

 * libcurl

 * libicu

 * basen (already included in lib/ dir)

 * cpr (already included in lib/ dir)

 * rapidjson (already included in lib/ dir)


Requires for running:

 * Internet connection

 * libcurl

 * libicu

 * ANSI/VT100-comptaible terminal for `mugcli` (i.e. not CMD.EXE or Minicom)

 * Linux environment (might also work on OSX)


To build:

    make -j


To run the command-line interactive interface:

	./mugcli


To collect training data for the AI player, using 200 workers:

	./mugcollect -o training.csv -p 200


To train the artificial intelligence using the previously-collected data:

	./TODO_NOT_IMPLEMENTED_YET training.csv


To run the fully-automated luxury cromulent dragon trainer:

	./mugomatic neuralnet.csv


To delete the compiled binaries and intermediate files out:

    make clean


And recklessly nuking the repo has the usual form:

	git reset --hard
    git clean -fdx


