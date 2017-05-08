#!/bin/bash
#
# Create all output results
#

# Useful shell settings:

# abort the script if a command fails
set -e

# abort the script if an unitialized shell variable is used
set -u

# make sure the code is up to date
pushd src
make
popd

# run harryPotterize
# src/imgpro input/screen_corners screen_corners output/screen_corners \
    # -harryPotterize input/markers marker