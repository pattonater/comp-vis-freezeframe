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

# initialize variable names
outputVideoName="tracked.mp4"
inputFolder="input/tmp_stills"
imageBaseName="img_still"
outputFolder="output/tmp_stills"
fps=24
# these must already exist
video="input/screen_corners.m4v"
markersFolder="input/markers"
markerBaseName="marker"

# make folders and fill input folder with image stills from video
mkdir $inputFolder
mkdir $outputFolder
ffmpeg -i $video -vf fps=$fps $inputFolder/$imageBaseName%d.jpg

# run harryPotterize on folder
src/imgpro $inputFolder $imageBaseName $outputFolder \
    -harryPotterize $markersFolder $markerBaseName

# make video out of output images
ffmpeg -framerate $fps -i $outputFolder/$imageBaseName%d.jpg output/$outputVideoName

# erase image stills
rm -r $inputFolder
rm -r $outputFolder
