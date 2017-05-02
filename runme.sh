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

#src/imgpro input/globos_de_colores.jpg output/globos_brighntess_0.5.jpg -brightness 0.5

#src/imgpro input/ClathrusArcheri.jpg output/ClathrusArcheri_Sobel_x.jpg -sobelX

#src/imgpro input/ClathrusArcheri.jpg output/ClathrusArcheri_Sobel_y.jpg -sobelY

#src/imgpro input/globos_de_colores.jpg output/globos_de_colores_Bilateral_6_1p5.jpg -bilateral 6.0 1.5

# src/imgpro -help
# src/imgpro -estimateHomography
#src/imgpro input/grainy.jpg output/grainy_Median_1.jpg -median 1
#src/imgpro input/grainy.jpg output/grainy_TrimmedMean_1_0p2.jpg -trimmedMean 1 0.2
#src/imgpro input/grainy.jpg output/grainy_Median_2.jpg -median 2
#src/imgpro input/grainy.jpg output/grainy_TrimmedMean_2_0p2.jpg -trimmedMean 2 0.2
#src/imgpro input/lakeLouise.jpg output/lakeLouise_Median_5.jpg -median 5
# src/imgpro input/lakeLouise.jpg output/lakeLouise_TrimmedMean_5_0p4.jpg -trimmedMean 5 0.4
# src/imgpro input/test_E_sitting01.jpg output/test_E_sitting_blendHomography.jpg -matchFeatures input/test_E_sitting02.jpg
# src/imgpro input/test_D_face01.jpg output/test_D_face_blendHomography.jpg -matchFeatures input/test_D_face02.jpg
# src/imgpro input/TESTA1.jpg output/TESTA_blendHomography.jpg -matchFeatures input/TESTA2.jpg
# src/imgpro input/test_F_desk01.jpg output/test_F_desk_blendHomography.jpg -matchFeatures input/test_F_desk02.jpg
# src/imgpro input/TESTB1.jpg output/TESTB_blendHomography.jpg -matchFeatures input/TESTB2.jpg
src/imgpro input/test_C_bridge01.jpg output/test_C_bridge_blendSubPixel.jpg -matchFeatures input/test_C_bridge02.jpg
# src/imgpro input/test_C_bridge01.jpg output/test_C_bridge_featureMatchALT.jpg -matchFeatures input/test_C_bridge02.jpg
# src/imgpro input/testpattern.jpg output/testpatternfeatureMatch.jpg -matchFeatures input/testpattern.jpg
# src/imgpro input/testpattern.jpg output/testpattern_testLine.jpg -testLine
# src/imgpro input/test_D_face01.jpg output/test_D_face01_harris_2.jpg -harris 2.0
# src/imgpro input/test_A.jpg output/test_A_features_2.jpg -features 2.0
# src/imgpro input/test_B.jpg output/test_B_features_2.jpg -features 2.0
# src/imgpro input/testpattern.jpg output/testpattern_featuresALTp_2.jpg -features 2.0
# src/imgpro input/grainy.jpg trimmedMean_out/grainy_Median_5.jpg -median 5
# src/imgpro input/grainy.jpg trimmedMean_out/grainy_Median_2.jpg -median 2
# src/imgpro input/grainy.jpg trimmedMean_out/grainy_Median_3.jpg -median 3
# src/imgpro input/grainy.jpg trimmedMean_out/grainy_TrimmedMean_1_0p1.jpg -trimmedMean 1 0.1
# src/imgpro input/grainy.jpg trimmedMean_out/grainy_TrimmedMean_1_0p25.jpg -trimmedMean 1 0.25
# src/imgpro input/grainy.jpg trimmedMean_out/grainy_TrimmedMean_1_0p4.jpg -trimmedMean 1 0.4
# src/imgpro input/grainy.jpg trimmedMean_out/grainy_TrimmedMean_2_0p1.jpg -trimmedMean 2 0.1
# src/imgpro input/grainy.jpg trimmedMean_out/grainy_TrimmedMean_2_0p25.jpg -trimmedMean 2 0.25
# src/imgpro input/grainy.jpg trimmedMean_out/grainy_TrimmedMean_2_0p4.jpg -trimmedMean 2 0.4
# src/imgpro input/grainy.jpg trimmedMean_out/grainylakeLouise_TrimmedMean_3_0p1.jpg -trimmedMean 3 0.1
# src/imgpro input/grainy.jpg trimmedMean_out/grainy_TrimmedMean_3_0p25.jpg -trimmedMean 3 0.25
# src/imgpro input/grainy.jpg trimmedMean_out/grainy_TrimmedMean_5_0p25.jpg -trimmedMean 5 0.25

#src/imgpro input/ClathrusArcheri.jpg output/ClathrusArcheri_HighPass_6_1p6.jpg -highpass 6.0 1.6

#src/imgpro input/ClathrusArcheri.jpg output/ClathrusArcheri_Blur_6.jpg -blur 6.0

#src/imgpro input/globos_de_colores.jpg output/globos_brighntess_1.5.jpg \
#    -brightness 1.5
