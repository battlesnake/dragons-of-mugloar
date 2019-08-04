#!/bin/bash

set -euo pipefail

docker build --quiet -t mugloar-build - < build.docker

function run {
	docker run \
		-it \
		--rm \
		-v "$(realpath "$PWD")":/src \
		--workdir /src \
		--user "$(id -u):$(id -g)" \
		mugloar-build \
		"$@"
}


function show_help {
	echo >&2 "Syntax: ./runner.sh <command>..."
	echo >&2 ""
	echo >&2 "Commands:"
	echo >&2 " * clean - remove build outputs"
	echo >&2 " * build - build project"
	echo >&2 " * collect - collect training data from random play"
	echo >&2 " * learn - generate tactical data from training data"
	echo >&2 " * play - play using tactical data, generates more training data and a score table"
	echo >&2 ""
}

function main {
	local cpus="$(grep -c ^processor /proc/cpuinfo)"

	if [ -z "$cpus" ]; then
		cpus=0
	fi

	let cpus=cpus+2

	while (( $# )); do
		local cmd="$1"
		shift

		case "$cmd" in
		clean) run make clean;;
		build) run make -j"$cpus" O="${O:-2}";;
		collect) run ./mugcollect -o training.dat -p 100;;
		learn) run ./muglearn -i training.dat -o feature_score.dat;;
		play) run ./mugomatic -i feature_score.dat -o training.dat -s scores.dat -p 100;;
		help) show_help;;
		*) show_help; exit 1;;
		esac
	done
}

if ! (( $# )); then
	set -- help
fi

main "$@"
