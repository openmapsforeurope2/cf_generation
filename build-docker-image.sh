#!/bin/sh
PROJECT_NAME=cf_generation
DOCKER_TAG="latest"

docker build --no-cache -t $PROJECT_NAME:$DOCKER_TAG .