#!/usr/bin/env sh
set -ex

podman build -t extend:latest --pull-always --squash-all .

id=$(podman create extend:latest)
rm -rf release
podman cp $id:/opt/extend/release .
podman rm -v $id
