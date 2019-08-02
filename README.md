# "Dragons of Mugloar" challenge


![It's true](falcdt.jpg)


Requires for building:

 * GNU Make v4+

 * C++17 compiler (e.g. recent version of gcc / clang)

 * libcurl


Requires for running:

 * Internet connection

 * libcurl

 * ANSI/VT100-comptaible terminal for `mugcli` (i.e. not CMD.EXE or minicom)


To build:

    make -j


To run the command-line interactive interface:

	./mugcli


To run the fully-automated luxury cromulent dragon trainer:

	./mugomatic


To clean all the generated stuff out:

    make clean


And recklessly nuking the repo has the usual form:

	git reset --hard
    git clean -fdx


