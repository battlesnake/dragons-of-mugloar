#!/bin/bash

set -ex

./mugcollect -o training.dat -p 100

./muglearn -i training.dat -o feature_score.dat

./mugomatic -i feature_score.dat -o training.dat -s scores.dat -p 100
