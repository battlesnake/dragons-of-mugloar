#!/bin/bash

set -ex

./mugcollect -o training.csv -p 20

./muglearn -i training.csv -o feature_score.lst

./mugomatic -i feature_score.lst
