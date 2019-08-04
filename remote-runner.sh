#!/bin/bash

set -euo pipefail

declare -r remote="${1:-}"
shift

if [ -z "$remote" ]; then
	echo >&2 "First argument must specify remote (e.g. user@206.81.22.146)"
	echo >&2 "If remote does not have docker installed, we will install assuming Debain/Ubuntu (apt)"
	exit 1
fi

# Install docker if needed
ssh "$remote" 'if ! which docker &>/dev/null; then sudo apt update && sudo apt install -y docker.io; fi'

# Create working directory if needed
ssh "$remote" mkdir -p mugloar

# Sync files over
rsync -avzl --delete ./ "$remote":mugloar

# Run tasks(s)
ssh "$remote" env -C mugloar ./runner.sh "$@"
