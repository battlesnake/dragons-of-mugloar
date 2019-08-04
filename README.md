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


To collect training data for the AI player, using 20 workers:

	# This will run endlessly unless you quit it with <q> <ENTER>
	./mugcollect -o training.dat -p 20


To train the artificial intelligence using the previously-collected data:

	# This uses a dumb linear model and manually-weighted costfunction
	./muglearn -i training.dat -o feature_score.dat


To run the fully-automated luxury cromulent dragon trainer with 20 workers:

	# This will run endlessly unless you quit it with <q> <ENTER>
	./mugomatic -i feature_score.dat -o training.dat -s scores.dat -p 20
	# Resulting scores (and game IDs) are appended to scores.dat


To delete the compiled binaries and intermediate files out:

    make clean


And recklessly nuking the repo has the usual form:

	git reset --hard
    git clean -fdx


# Neural-network approach

We can improve the intelligence of our AI by using a neural network with a single hidden-layer and non-linear transforms (e.g. sigmoid).
The neural network will produce better predictions of output costs from input feature sets, due to the more complex model structures that it can represent (vs. the linear weighted model).

We still have the issue of the manually-specified weights in the costfunction.
Once we have a neural network that's providing reasonably good fits with some manual tweaking of the costfunction, we can add an extra layer above which:

 1. Defines a costfunction (e.g. set of linear weights).

 2. Trains a neural network using this costfunction, using pre-recorded training datasets.

 3. Plays several hundred games using this neural network to decide on actions (based on its cost predictions).  We can add every action/event to the training dataset.

 4. Adjusts the costfunction parameters based on the mean/max scores achieved during the games.

 5. Repeat from step 1.

This would use the end-game scores to tune the costfunction parameters, optimising the costfunction to deliver higher-scoring games (provided the move-based learning/playing is stable).

It still will not be able to learn fairly some simple strategic concepts, such as "Buy health potion if only 1 life remaining" or "Do not buy other items unless it'll leave enough gold for a health potion".
These could possibly be learned by adding extra features (e.g. "1 life left", "2 lives left", "over 150 gold", "over 350 gold"), and by having the learning process punish the previous _N_ actions for a loss of life, rather than just the immediate action.

Due to the time required to implement this, and the questionable quality of the return, I decided to abort this approach and go for a 1980s-style deterministic model instead.
