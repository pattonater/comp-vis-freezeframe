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

# generate the result pictures

#src/imgpro input/globos_de_colores.jpg output/globos_brighntess_0.5.jpg \
#    -brightness 0.5

#src/imgpro input/globos_de_colores.jpg output/globos_brighntess_1.0.jpg \
#    -brightness 1.0

#src/imgpro input/globos_de_colores.jpg output/globos_brighntess_1.5.jpg \
  #  -brightness 1.5


#src/imgpro input/testpattern.jpg output/testpattern_sobel_x.jpg \
#        -sobelX
#src/imgpro input/testpattern.jpg output/testpattern_sobel_y.jpg \
#        -sobelY
#src/imgpro input/testpattern.jpg output/testpattern_sharp.jpg \
#        -sharpen

# src/imgpro input/testpattern.jpg output/testpattern_blur_2.jpg \
#         -blur 2
# src/imgpro input/testpattern.jpg output/testpattern_blur_6.jpg \
#         -blur 6

# src/imgpro input/wall.jpg output/wall_blur_2.jpg \
#         -blur 2
# src/imgpro input/wall.jpg output/wall_blur_6.jpg \
#         -blur 6

# src/imgpro input/alexa.jpg output/alexa_b.jpg \
#         -blur 9
# src/imgpro input/alexa.jpg output/alexa_b2.jpg \
#         -blur 2


# src/imgpro input/wall.jpg output/wall_0.5.jpg \
#     -highpass 0.5 3.0

# src/imgpro input/wall.jpg output/wall_1.0.jpg \
#     -highpass 1.0 3.0

# src/imgpro input/wall.jpg output/wall_3.0.jpg \
#     -highpass 3.0 3.0

# src/imgpro input/wall.jpg output/wall_6.0.jpg \
#     -highpass 6.0 3.0

# src/imgpro input/testpattern.jpg output/test_harris.jpg \
#     -harris 2.0

# src/imgpro input/d_face1.jpg output/face_harris.jpg \
#     -harris 2.0

# src/imgpro input/c_bridge1.jpg output/c_bridge1_features.jpg \
#     -features 10

# src/imgpro input/d_face1.jpg output/d_face1_features.jpg \
#     -features 450

# src/imgpro input/e_sitting1.jpg output/e_sitting_features.jpg \
#     -features 450

# src/imgpro input/testpattern.jpg output/testpattern_f.jpg \
#      -features 450 

# src/imgpro input/alexa.jpg output/alexa_tracked.jpg \
#     -trackfeatures 15 input/alexa2.jpg

# src/imgpro input/d_face1.jpg output/d_face_tracked.jpg \
#     -trackfeatures 150 input/d_face2.jpg

# src/imgpro input/c_bridge1.jpg output/c_bridge_tracked.jpg \
#     -trackfeatures 150 input/c_bridge2.jpg

# src/imgpro input/dolac_market1.jpg output/dolac_market_tracked.jpg \
#     -trackfeatures 150 input/dolac_market2.jpg

# src/imgpro input/TESTA1.jpg output/TESTA_tracked.jpg \
#     -trackfeatures 150 input/TESTA2.jpg

# src/imgpro input/TESTB1.jpg output/TESTB_tracked.jpg \
#     -trackfeatures 150 input/TESTB2.jpg

# src/imgpro input/e_sitting1.jpg output/e_sitting_tracked.jpg \
#     -trackfeatures 150 input/e_sitting2.jpg

# src/imgpro input/coffee2.jpg output/coffee_tracked.jpg \
#     -trackfeatures 150 input/coffee1.jpg


# src/imgpro input/TESTB1.jpg output/TESTB_matched.jpg \
#     -svdTest 

# src/imgpro input/TESTA1.jpg output/TESTA_matched_old.jpg \
#     -matchHomography input/TESTA2.jpg

# src/imgpro input/TESTB1.jpg output/TESTB_matched_old.jpg \
#     -matchHomography input/TESTB2.jpg

# src/imgpro input/e_sitting1.jpg output/e_sitting_matched_old.jpg \
#     -matchHomography input/e_sitting2.jpg

# src/imgpro input/c_bridge1.jpg output/c_bridge_matched_old.jpg \
#     -matchHomography input/c_bridge2.jpg

# src/imgpro input/dolac_market1.jpg output/dolac_market_matched_old.jpg \
#     -matchHomography input/dolac_market2.jpg


# WORKING CALLS:

src/imgpro input/d_face1.jpg output/d_face_matched.jpg \
    -matchHomography input/d_face2.jpg

src/imgpro input/d_face1.jpg output/d_face_tracked.jpg \
    -trackfeatures 150 input/d_face2.jpg


# src/imgpro input/screen_corners screen_corners output/screen_corners \
#       -harryPotterize

