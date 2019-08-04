#!/bin/bash

set -xeuo pipefail

docker build -t mugloar-build - < build.docker

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

declare cpus="$(grep -c ^processor /proc/cpuinfo)"

if [ -z "$cpus" ]; then
	cpus=0
fi

let cpus=cpus+2

run make -j"$cpus"
