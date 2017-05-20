
// Computer Vision for Digital Post-Production
// Lecturer: Gergely Vass - vassg@vassg.hu
//
// Skeleton Code for programming assigments
//
// Code originally from Thomas Funkhouser
// main.c
// original by Wagner Correa, 1999
// modified by Robert Osada, 2000
// modified by Renato Werneck, 2003
// modified by Jason Lawrence, 2004
// modified by Jason Lawrence, 2005
// modified by Forrester Cole, 2006
// modified by Tom Funkhouser, 2007
// modified by Chris DeCoro, 2007
//



// Include files

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "Timer.cpp"
#include "R2/R2.h"
#include "R2Pixel.h"
#include "R2Image.h"

// Added for processing image sequences
#include <string>
#include <sys/stat.h>

// Program arguments

static char options[] =
"  -help\n"
"  -svdTest\n"
"  -sobelX\n"
"  -sobelY\n"
"  -log\n"
"  -harris <real:sigma>\n"
"  -saturation <real:factor>\n"
"  -brightness <real:factor>\n"
"  -blur <real:sigma>\n"
"  -sharpen \n"
"  -matchTranslation <file:other_image>\n"
"  -matchHomography <file:other_image>\n";


static void
ShowUsage(void)
{
  // Print usage message and exit
  fprintf(stderr, "Usage: imgpro input_image output_image [  -option [arg ...] ...]\n");
  fprintf(stderr, "%s", options);
  exit(EXIT_FAILURE);
}



static void
CheckOption(char *option, int argc, int minargc)
{
  // Check if there are enough remaining arguments for option
  if (argc < minargc)  {
    fprintf(stderr, "Too few arguments for %s\n", option);
    ShowUsage();
    exit(-1);
  }
}

////////////////////////////
// Image Processing Helpers
////////////////////////////
void verifyImageAllocation(R2Image *image) {
  if (!image) {
    fprintf(stderr, "Unable to allocate image\n");
    delete image;
    exit(-1);
  }
}

void writeImage(R2Image* image, const char* name) {
    //printf("in write image\n");
  if (!image->Write(name)) {
     fprintf(stderr, "Unable to read image from %s\n", name);
     delete image;
     exit(-1);
  } 
}

bool checkFileExistance(const std::string& file_name) {
  struct stat buffer;
  return (stat(file_name.c_str(), &buffer) == 0);
}



////////////////////////////
// Image Sequence Processing
////////////////////////////
bool debugMode = false;

void grabImageNames(std::vector<std::string>& inputImageNames, char *input_folder_name, char *image_base_name) {
  const int maxNumberImages = 500;

  const std::string inputNameBase = std::string(input_folder_name) + "/" + std::string(image_base_name);

  for( int i = 1; i < maxNumberImages; i++) {
    const std::string inputName = inputNameBase + std::to_string(i) + ".jpg";

    // this is how you exit the loop (once you get to an invalid file name)
    bool validFileName = checkFileExistance(inputName);
    if (!validFileName) { break; } 

    inputImageNames.push_back(inputName);
  }
}

void generateOutputNames(std::vector<std::string>& inputImageNames, std::vector<std::string>& outputImageNames, char *output_folder_name, char *image_base_name) {
  const std::string outputNameBase = std::string(output_folder_name) + "/" + std::string(image_base_name);

  for (int i = 0; i < inputImageNames.size(); i++) {
    std::string outputName = outputNameBase + std::to_string(i) + ".jpg";
    outputImageNames.push_back(outputName);
  }
}

void importMarkerImages(std::vector<R2Image>& markerImages, char* marker_folder_name, char* marker_base_name) {
  std::vector<std::string> markerImageNames;
  grabImageNames(markerImageNames, marker_folder_name, marker_base_name);

  // import marker images
  for (size_t i = 0; i < markerImageNames.size(); ++i) {
    //printf(markerImageNames[i].c_str()); printf("\n");
    R2Image *marker = new R2Image(markerImageNames[i].c_str());
    verifyImageAllocation(marker);
    markerImages.push_back(*marker);
  }

  assert(markerImages.size() == 4);
}

void harryPotterizeSequence(std::vector<std::string> &inputImageNames, std::vector<std::string> &outputImageNames, std::vector<std::string> &inputInnerImageNames, std::vector<R2Image> &markerImages, bool multithreaded) {
  const float fps = 24;
  const float videoStartTime = 0 * fps;
  const float videoAnimateTime = 0.75 * fps;
  const float videoEndTime = 1000 * fps;
  
  
  if (debugMode) printf("Sequence loaded... \n");
  
  // initalize to (-1, -1) so we can tell they havent been set yet
  std::vector<Point> cornerCoords;
  for (int i = 0; i < inputImageNames.size(); i++) {
      cornerCoords.push_back(Point(-1, -1));
  }

  // start overall timer
  Timer outerTimer;
  outerTimer.start();
  Timer innerTimer;


// iterate through image frames
  for (int i = 0; i < inputImageNames.size(); i++) {
    printf("%.2f%% Complete\n", 100 * float(i) / float(inputImageNames.size()));

    // allocate image frame
    R2Image *imageFrame = new R2Image(inputImageNames[i].c_str()); 
    verifyImageAllocation(imageFrame);

     if (i < videoStartTime) {
      writeImage(imageFrame, outputImageNames[i].c_str());
      continue;
    }

    //allocate inner image
    R2Image *innerFrame;
    if (i < inputInnerImageNames.size() + videoAnimateTime) {
      // always grab first frame if before videoAnimateTime
      int frameNum = i - videoAnimateTime;
      frameNum = fmax(0, frameNum);

      innerFrame = new R2Image(inputInnerImageNames[frameNum].c_str());
      verifyImageAllocation(innerFrame);

    } else {
      innerFrame = new R2Image(inputInnerImageNames[inputInnerImageNames.size() - 1].c_str());
      verifyImageAllocation(innerFrame);
    }

    // set threading mode
    imageFrame->setMultiThread(multithreaded);

    int numPreviousLocations = 0;
    for (int j = 0; j < cornerCoords.size(); j++) {
      numPreviousLocations += (cornerCoords[j].x != -1);
    }

    if (debugMode) printf("Num Previous Locations: %d \n", numPreviousLocations);
    // Find trackers on image
    if (debugMode) innerTimer.start();
    imageFrame->identifyCorners(markerImages, cornerCoords);
    if (debugMode) printf("Image %d corners found ...%d ms \n", i+1, innerTimer.elapsedTime());

    // Warp in other image
    if (debugMode) innerTimer.start();
    imageFrame->placeImageInFrame(cornerCoords, *innerFrame);
    if (debugMode) printf("Image %d image warped in...%d ms \n", i+1, innerTimer.elapsedTime());
    
    // Write output image
    writeImage(imageFrame, outputImageNames[i].c_str());

    // clean up memory
    delete imageFrame;
  }
  if (debugMode) printf("Sequence done! %lu frames %d seconds\n\n", inputImageNames.size(), outerTimer.elapsedTime() / 1000);
}

void processImageSequence(int argc, char **argv, char *input_folder_name) {
  char *image_base_name = *argv; argv++, argc--;
  char *output_folder_name = *argv; argv++, argc--;

  // extract image names
  std::vector<std::string> inputImageNames;
  grabImageNames(inputImageNames, input_folder_name, image_base_name);
  // generate output names
  std::vector<std::string> outputImageNames;
  generateOutputNames(inputImageNames, outputImageNames, output_folder_name, image_base_name);
  assert(inputImageNames.size() == outputImageNames.size());

  // break if no images
  if (inputImageNames.size() <= 0) return;

  if (debugMode) printf("Found %lu images \n", inputImageNames.size());

  // Parse arguments and perform operations
  while (argc > 0) {
    if (!strcmp(*argv, "-harryPotterize")) {
      CheckOption(*argv, argc, 4);
      char* marker_folder_name = argv[1];
      char* marker_base_name = argv[2];
      char* inner_sequence_folder_name = argv[3];
      //R2Image* other_image = new R2Image(argv[3]);
      argv += 4, argc -= 4;

      std::vector<std::string> inputInnerImageNames;
      grabImageNames(inputInnerImageNames, inner_sequence_folder_name, image_base_name);

      // import marker images
      std::vector<R2Image> markerImages;
      importMarkerImages(markerImages, marker_folder_name, marker_base_name);
      if (debugMode) printf("Found %lu marker images\n", markerImages.size());    

      // for (int i = 0; i < markerImages.size(); i++) {
      //     writeImage(&markerImages[i], outputImageNames[i].c_str());
      // }
      // return;

      // test single vs multithread
      bool testThreadSpeeds = false;
      if (testThreadSpeeds) {
        // Single thread test
        printf("Single thread:\n");
        harryPotterizeSequence(inputImageNames, outputImageNames, inputInnerImageNames, markerImages, false);

        // Multi thread test
        printf("Multi thread:\n");
        harryPotterizeSequence(inputImageNames, outputImageNames, inputInnerImageNames, markerImages, true);
      } else {
        // do the magic
        harryPotterizeSequence(inputImageNames, outputImageNames, inputInnerImageNames, markerImages, false);
      }
    } else if (!strcmp(*argv, "-harryPotterizeImage")) {
      CheckOption(*argv, argc, 4);
      char* marker_folder_name = argv[1];
      char* marker_base_name = argv[2];
      char* other_image = argv[3];
      argv += 4, argc -= 4;

      std::vector<std::string> inputInnerImageNames;
      inputInnerImageNames.push_back(std::string(other_image));

      // import marker images
      std::vector<R2Image> markerImages;
      importMarkerImages(markerImages, marker_folder_name, marker_base_name);
      if (debugMode) printf("Found %lu marker images\n", markerImages.size());    


      // test single vs multithread
      bool testThreadSpeeds = false;
      if (testThreadSpeeds) {
        // Single thread test
        printf("Single thread:\n");
        harryPotterizeSequence(inputImageNames, outputImageNames, inputInnerImageNames, markerImages, false);

        // Multi thread test
        printf("Multi thread:\n");
        harryPotterizeSequence(inputImageNames, outputImageNames, inputInnerImageNames, markerImages, true);
      } else {
        // do the magic
        harryPotterizeSequence(inputImageNames, outputImageNames, inputInnerImageNames, markerImages, false);
      }
    }

    else {
      // Unrecognized program argument
      fprintf(stderr, "image: invalid option: %s\n", *argv);
      ShowUsage();
    }
  }
}





//////////////////
/// Main 
/////////////////

int
main(int argc, char **argv)
{
  // Look for help
    for (int i = 0; i < argc; i++) {
      if (!strcmp(argv[i], "-help")) {
        ShowUsage();
      }
	    if (!strcmp(argv[i], "-svdTest")) {
        R2Image *image = new R2Image();
	      image->testDLT();
	      return 0;
      }
    }

  // Read input filename
  if (argc < 3)  ShowUsage();
  argv++, argc--; // First argument is program name
  char *input_name = *argv; argv++, argc--;
  
  // This could probably be made lighter weight by just looking at last 3 characters of char*
  std::string input_string(input_name);
  bool inputIsImage = input_string.find(".jpg") != std::string::npos;

  if (inputIsImage) {
    //processImage(argc, argv, input_name);
  } else {
    processImageSequence(argc, argv, input_name);
  }

  return EXIT_SUCCESS;
}



////////////////////////////
// Single Image Processing
////////////////////////////
void processImage(int argc, char **argv, char *input_image_name) {
  char *output_image_name = *argv; argv++, argc--;

  // Allocate image
  R2Image *image = new R2Image();
  verifyImageAllocation(image);

  // Read input image
  if (!image->Read(input_image_name)) {
    fprintf(stderr, "Unable to read image from %s\n", input_image_name);
    exit(-1);
  }

  // Initialize sampling method
  //int sampling_method = R2_IMAGE_POINT_SAMPLING;

  // Parse arguments and perform operations
  while (argc > 0) {
    if (!strcmp(*argv, "-brightness")) {
      CheckOption(*argv, argc, 2);
      double factor = atof(argv[1]);
      argv += 2, argc -=2;
      image->Brighten(factor);
    }
	else if (!strcmp(*argv, "-sobelX")) {
      argv++, argc--;
      image->SobelX();
    }
	else if (!strcmp(*argv, "-sobelY")) {
      argv++, argc--;
      image->SobelY();
    }
	else if (!strcmp(*argv, "-log")) {
      argv++, argc--;
      image->LoG();
    }
    else if (!strcmp(*argv, "-saturation")) {
      CheckOption(*argv, argc, 2);
      double factor = atof(argv[1]);
      argv += 2, argc -= 2;
      image->ChangeSaturation(factor);
    }
	else if (!strcmp(*argv, "-harris")) {
      CheckOption(*argv, argc, 2);
      double sigma = atof(argv[1]);
      argv += 2, argc -= 2;
      image->Harris(sigma, true);
    }
   else if (!strcmp(*argv, "-features")) {
      CheckOption(*argv, argc, 2);
      double numFeatures = atof(argv[1]);
      argv += 2, argc -= 2;
      image->FeatureDetector(numFeatures);
    }
    else if (!strcmp(*argv, "-trackfeatures")) {
      CheckOption(*argv, argc, 3);
      int numFeatures = atof(argv[1]);
      R2Image* otherImage = new R2Image(argv[2]);
      argv += 3, argc -= 3;
      image->TrackFeatures(numFeatures, *otherImage);
      delete otherImage;
    }
    else if (!strcmp(*argv, "-blur")) {
      CheckOption(*argv, argc, 2);
      double sigma = atof(argv[1]);
      argv += 2, argc -= 2;
      image->Blur(sigma, true);
    }
     else if (!strcmp(*argv, "-highpass")) {
      CheckOption(*argv, argc, 3);
      double sigma = atof(argv[1]);
      double contrast = atof(argv[2]);
      argv += 3, argc -= 3;
      image->HighPass(sigma, contrast);
    }
    else if (!strcmp(*argv, "-sharpen")) {
      argv++, argc--;
      image->Sharpen();
    }
    else if (!strcmp(*argv, "-matchTranslation")) {
      CheckOption(*argv, argc, 2);
      R2Image *other_image = new R2Image(argv[1]);
      argv += 2, argc -= 2;
      image->blendOtherImageTranslated(other_image);
      delete other_image;
    }
    else if (!strcmp(*argv, "-matchHomography")) {
      CheckOption(*argv, argc, 2);
      R2Image* other_image = new R2Image(argv[1]);
      printf("%s \n", argv[1]);
      argv += 2, argc -= 2;
      image->MatchHomography(*other_image);
      delete other_image;
    } else {
      // Unrecognized program argument
      fprintf(stderr, "image: invalid option: %s\n", *argv);
      ShowUsage();
    }
  }

  // Write output image
  writeImage(image, output_image_name);

  // Delete image
  delete image;
}


