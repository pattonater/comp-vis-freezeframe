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

# input variables
outputVideo="output/back_half.mp4"
fps=24

# tmp names
inputFolder="input/tmp_stills"
innerInputFolder="input/inner_tmp_stills"
imageBaseName="img_still"
outputFolder="output/tmp_stills"

# these must already exist
outerVideo="input/skit/skit_videos/back_half.mov"
innerVideo="input/skit/article_videos/hungarian.mov"
markersFolder="input/skit/skit_back_markers_hungarian"
markerBaseName="marker"

inputPhoto="input/d_face1.jpg"

# remove any files left over from previous runs
if test -d $innerInputFolder; then 
    rm -r $innerInputFolder
fi
if test -d $inputFolder; then 
    rm -r $inputFolder
fi
if  test -d $outputFolder; then 
    rm -r $outputFolder
fi
if test -e $outputVideo; then
    rm $outputVideo
fi

# make folders
mkdir $inputFolder
mkdir $innerInputFolder
mkdir $outputFolder

# fill input folder with image stills from video
ffmpeg -i $outerVideo -qscale:v 2 $inputFolder/$imageBaseName%d.jpg
ffmpeg -i $innerVideo -qscale:v 2 $innerInputFolder/$imageBaseName%d.jpg
# ffmpeg -i $outerVideo -vf fps=$fps $inputFolder/$imageBaseName%d.jpg
# ffmpeg -i $innerVideo -vf fps=$fps $innerInputFolder/$imageBaseName%d.jpg

# run harryPotterize on folder
src/imgpro $inputFolder $imageBaseName $outputFolder \
    -harryPotterize $markersFolder $markerBaseName $innerInputFolder

# make video out of output images
ffmpeg -framerate $fps -i $outputFolder/$imageBaseName%d.jpg $outputVideo

# erase image stills
rm -r $inputFolder
rm -r $outputFolder
rm -r $innerInputFolder