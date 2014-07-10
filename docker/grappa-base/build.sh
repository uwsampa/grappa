#!/bin/bash
##############################################
# Commands to build 'grappa-base'
# Also publishes to Docker Registry
##############################################

docker build -t uwsampa/grappa-base ./grappa-base
docker push uwsampa/grappa-base
