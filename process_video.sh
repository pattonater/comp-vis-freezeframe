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
outputVideoName = tracked.mp4
video = input/screen_corners.mp4
markersFolder = input/markers
markerBaseName = marker
inputFolder = input/tmp_stills
imageBaseName = img_still
outputFolder = output/tmp_stills

# make folders and fill input folder with image stills from video
mkdir inputFolder
mkdir outputFolder
ffmpeg -i video -vf fps=24 inputFolder/imageBaseName%d.jpg

# run harryPotterize on folder
src/imgpro inputFolder imageBaseName outputFolder \
    -harryPotterize markersFolder markerBaseName

# make video out of output images
ffmpeg -framerate 24 -i imageBaseName%d.jpg tracked

# move video to output folder
mv outputFolder/outputVideoName output/$outputVideoName

# erase image stills
#rm inputFolder
#rm outputFolder