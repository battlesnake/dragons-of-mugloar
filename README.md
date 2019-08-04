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


To prepare:

	# Downloads nested dependency repos
	git submodule update --init


To build:

    make -j


To run the command-line interactive interface:

	./mugcli


To collect training data for the AI player, using 20 workers:

	# This will run endlessly unless you quit it with <q> <ENTER>
	./mugcollect -o training.dat -p 20


To train the artificial intelligence using the previously-collected data:

	# This uses a dumb linear model and manually-weighted costfunction
	# This also uses insane amounts of RAM.  I run it on a 64GB cloud server.
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


# Costed feature-set approach

 * We represent the state of a game as a feature vector.

 * Each possible action is also represented as a feature vector.

 * Each feature has a "cost".

 * The cost of an action is the sum of costs of all features in the game-state and in the specific action.

 * Each turn, the highest-cost action is chosen (with cost-check to eliminate purchases with insufficient gold, to prevent loops).

Failed purchase attempts could be useful for skipping turns, in the hope that bad messages get replaced with good ones.
But we haven't implemented that decision-making yet.
We do have the necessary training data collected to approach this though.

While this approach is unlikely to topple the current high-score (no normalisation, no covariance / feature correlation, no memory), it has given some useful information:

 * The "probability" values for solving messages, and the relative risk of each one.

 * "Help defend" tasks are practically suicide.  This may not be true if you have lots of high-value items, but the AI doesn't buy much besides health potions.


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

It also will not be able to learn that buying some item can increase the chance of success later on.
This could be determined from the training data fairly easily, but the AI won't learn to actually buy the item to provide long-term gains - only to increase the instantaneous cost.

Due to the time required to implement this approach, and the questionable quality of the return, I decided to abort this approach and go for a 1980s-style deterministic model instead.

The issues with a simple feedforward neural network not being able to learn the value of long-term strategic actions (e.g. buying certain items) could be fixed by using an LSTM architecture (long short-term memory).
I believe that this is quite a common solution to the strategy/tactics issue for deep-learning approaches, but I haven't used an LSTM directly before.
