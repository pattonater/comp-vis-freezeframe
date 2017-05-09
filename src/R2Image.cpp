// Source file for image class



// Include files

#include "R2/R2.h"
#include "R2Pixel.h"
#include "R2Image.h"
#include "svd.h"
#include <cmath>
#include <vector>
#include <pthread.h>
// #include <stdlib.h>     /* srand, rand */
// #include <time.h>       /* time */


///////////////////////
// Macros (functionally)
//////////////////////
bool R2Image:: inBounds(const Point p) const { return inBounds(p.x, p.y); }
bool R2Image:: inBounds(const int x, const int y) const { return (x >= 0) && (x < width) && (y >= 0) && (y < height); }
const bool MULTI_THREAD = true;

///////////////////////
// Freeze Frame
//////////////////////
void R2Image::
placeImageInFrame(std::vector<Point>& markerLocations, R2Image& otherImage) {
  // dependent on order of marker images... potentially fix later
  Point& markerBottomLeft  = markerLocations[0];
  Point& markerBottomRight = markerLocations[1];
  Point& markerTopLeft     = markerLocations[2];
  Point& markerTopRight    = markerLocations[3];

  Point imageBottomLeft  = Point(0, 0);
  Point imageBottomRight = Point(otherImage.width, 0);
  Point imageTopLeft     = Point(0, otherImage.height);
  Point imageTopRight    = Point(otherImage.width, otherImage.height);

  std::vector<double> H;
  std::vector<PointMatch> pointMatches;

  pointMatches.push_back(PointMatch(markerBottomLeft, imageBottomLeft));
  pointMatches.push_back(PointMatch(markerBottomRight, imageBottomRight));
  pointMatches.push_back(PointMatch(markerTopLeft, imageTopLeft));
  pointMatches.push_back(PointMatch(markerTopRight, imageTopRight));

  computeHomographyMatrixWithDLT(pointMatches, H);

  Frame frame = Frame(markerBottomLeft, markerBottomRight, markerTopLeft, markerTopRight);

  warpImageIntoFrame(H, otherImage, frame);
}

void R2Image::
warpImageIntoFrame(const std::vector<double>& homographyMatrix, R2Image& otherImage, Frame& frame) {
  int xLower = fmin(frame.topLeft.x, frame.bottomLeft.x);
  int xUpper = fmax(frame.topRight.x, frame.bottomRight.x) + 1;
  int yLower = fmin(frame.bottomLeft.y, frame.bottomRight.y);
  int yUpper = fmax(frame.topLeft.y, frame.topRight.y) + 1;

  for (int i = xLower; i < xUpper; i++) {
    for (int j =  yLower;  j <= yUpper; j++) {
      const Point p = transformPoint(i, j, homographyMatrix);
      const int x0 = p.x;
      const int y0 = p.y;
      
      const int x1 = x0 + 1;
      const int y1 = y0 + 1;

      if (otherImage.inBounds(x0, y0) && otherImage.inBounds(x1, y1)) {
        const double alphaX = p.x - x0;
        const double alphaY = p.y - y0;

        R2Pixel upperHalf = (1 - alphaX) * otherImage.Pixel(x0, y0) + alphaX * otherImage.Pixel(x1, y0);
        R2Pixel lowerHalf = (1 - alphaX) * otherImage.Pixel(x0, y1) + alphaX * otherImage.Pixel(x1, y1);

        Pixel(i, j) = (1 - alphaY) * upperHalf + alphaY * lowerHalf;
      } else if (otherImage.inBounds(x0, y0)) {
        Pixel(i, j) = otherImage.Pixel(x0, y0);
      } else if (otherImage.inBounds(x1,y1)) {
        Pixel(i, j) = otherImage.Pixel(x1, y1);
      }
    }
  }
}

void R2Image::
identifyCorners(std::vector<R2Image>& markers, std::vector<Point>& oldMarkerLocations) {
   // printf("markers size %lu\n", markers.size());
  // fill markerLocations with matched locations to the marker images
  std::vector<Point> markerLocations;
  findMarkers(markers, markerLocations, oldMarkerLocations);

  // Mark each location
  for (int i = 0; i < markerLocations.size(); i++) {
    Point& p = markerLocations[i];
    drawFilledSquare(p.x, p.y, 10, 1.0, 0.0, 0.0);
    oldMarkerLocations[i] = p;
  }
}

void* globalHelper(void * inputPointer) {
  // printf("entering globalHelper\n");
  // speed optimization: can safely step by an 8th of the marker without missing

  R2Image::Marker* markerPointer = (R2Image::Marker*)inputPointer;

  R2Image*             frame          = markerPointer->frame;
  R2Image*             marker         = markerPointer->marker;
  Point*               markerLocation = markerPointer->markerLocation; 
  Point*               oldLocation    = markerPointer->oldMarkerLocation;
  std::vector<Point*>* locs           = markerPointer->markerLocs;
  int                  i              = markerPointer->index;      

  size_t width  = frame->Width();
  size_t height = frame->Height();

  int xStepSize = marker->Width() / 8;
  int yStepSize = marker->Height() / 8;

  int FINE_X = xStepSize * 2;
  int FINE_Y = yStepSize * 2;

  float bestSSD = marker->Width() * marker->Height() * 3;
    int bestX = -1;
    int bestY = -1;

    // remove for final
    // const int a = ((i % 2) * width / 2);
    // const int b = width / (2 - (i % 2));
    // const int c = ((i / 2) * height / 2);
    // const int d = height / (2 - (i / 2));
    
    // use oldLocation to improve search speed
    const int searchWidthReach = width * 0.1;
    const int searchHeightReach = height * 0.1;
    
    // const Point& oldLocation = oldMarkerLocations[i];
    //printf("(%f, %f) \n", oldLocation.x, oldLocation.y);
    
    const bool pastLocExists = oldLocation->x != -1;
    // initialize search bounds to 20% of image around
    int xMin = pastLocExists ? oldLocation->x - searchWidthReach : 0; //CHANGE TO 0
    int xMax = pastLocExists ? oldLocation->x + searchWidthReach : width; //CHANGE TO width
    int yMin = pastLocExists ? oldLocation->y - searchHeightReach : 0; //CHANGE TO 0
    int yMax = pastLocExists ? oldLocation->y + searchHeightReach : height; //CHANGE TO height
    
  // printf("setup complete\n");

  // Iterate over image
  while (xStepSize != 0) {
    // printf("xStep: %d    yStep: %d\n", xStepSize, yStepSize);
    for (int x = xMin; x < xMax; x += xStepSize) {
      //printf("Reached row %d... \n", x + 1);
      for (int y = yMin; y < yMax; y += yStepSize) {
        // See if point is better match for any of markers
        const float ssd = frame->calculateSSD(x, y, *marker);
        if (ssd < bestSSD) {
            //printf("Marker %d Better ssd: %f... \n", i + 1, ssd);
            //printf("Better ssd: %f... \n", ssd);
            bestSSD = ssd;
            bestX = x;
            bestY = y;
        }
      }
    }

    if (xStepSize == 1 && yStepSize == 1) {
      xStepSize = 0;
      yStepSize = 0;
    } else {
      xStepSize = (xStepSize > 1) ? xStepSize / 2 : 1;
      yStepSize = (yStepSize > 1) ? yStepSize / 2 : 1;
      FINE_X = xStepSize * 2;
      FINE_Y = yStepSize * 2;

      xMin = fmax(0, bestX - FINE_X);
      xMax = fmin(width, bestX + FINE_X);
      yMin = fmax(0, bestY - FINE_Y);
      yMax = fmin(height, bestY + FINE_Y);
    }
  }

  // make new search window around best SSD 
  // TODO: will this give us issues re: local minima?
  // TODO: recursive descent
  // const size_t lowX = fmax(0, bestX - FINE_X);
  // const size_t hiX  = fmin (width, bestX + FINE_X);
  // const size_t lowY = fmax(0, bestY - FINE_Y);
  // const size_t hiY  = fmin (height, bestY + FINE_Y);

  // for (int x = lowX; x < hiX; ++ x) {
  //   for (int y = lowY; y < hiY; ++y) {
  //       // See if point is better match for any of markers
  //     const float ssd = calculateSSD(x, y, marker);
  //     if (ssd < bestSSD) {
  //       //printf("Marker %d Better ssd: %f... \n", i + 1, ssd);      
  //       bestSSD = ssd;
  //       bestX = x;
  //       bestY = y;
  //     }
  //   }
  // }

  // printf("match found\n");
  (*locs)[i] = new Point(bestX, bestY);
  // printf("wrote marker location\n");
  return NULL;
}


void R2Image::
findMarkers(std::vector<R2Image>& markers, std::vector<Point>& markerLocations, std::vector<Point>& oldMarkerLocations) {
  if (MULTI_THREAD) {
    std::vector<pthread_t> pth;
    std::vector<Point*> locs;

    for (int i = 0; i < markers.size(); i++) {
      pthread_t p;

      pth.push_back(p);
      locs.push_back(new Point(-1, -1));

      pthread_create(&pth[i], NULL, globalHelper, 
        (void*)(new Marker(this, &markers[i], &markerLocations[i], &oldMarkerLocations[i], &locs, i)));
    }

    for (size_t i = 0; i < 4; ++i) {
      // printf("joining thread %d\n", i);
      pthread_join(pth[i], NULL);
      // printf("joined thread %d\n", i);
      markerLocations.push_back(*(locs[i]));
      assert(markerLocations[i].x != -1);
    }
  } 
  else 
  {
    for (size_t i = 0; i < markers.size(); ++ i) {
      // speed optimization: can safely step by an 8th of the marker without missing
      R2Image& marker = markers[i];
      int xStepSize = marker.Width() / 8;
      int yStepSize = marker.Height() / 8;

      int FINE_X = xStepSize * 2;
      int FINE_Y = yStepSize * 2;

      float bestSSD = marker.Width() * marker.Height() * 3;
        int bestX = -1;
        int bestY = -1;

        // remove for final
        const int a = ((i % 2) * width / 2);
        const int b = width / (2 - (i % 2));
        const int c = ((i / 2) * height / 2);
        const int d = height / (2 - (i / 2));
        
        // use oldLocation to improve search speed
        const int searchWidthReach = width * 0.1;
        const int searchHeightReach = height * 0.1;
        
        const Point& oldLocation = oldMarkerLocations[i];
        //printf("(%f, %f) \n", oldLocation.x, oldLocation.y);
        
        const bool pastLocExists = oldLocation.x != -1;
        // initialize search bounds to 20% of image around
        int xMin = pastLocExists ? oldLocation.x - searchWidthReach : 0; //CHANGE TO 0
        int xMax = pastLocExists ? oldLocation.x + searchWidthReach : width; //CHANGE TO width
        int yMin = pastLocExists ? oldLocation.y - searchHeightReach : 0; //CHANGE TO 0
        int yMax = pastLocExists ? oldLocation.y + searchHeightReach : height; //CHANGE TO height
        

      // Iterate over image
      while (xStepSize != 0) {
        // printf("xStep: %d    yStep: %d\n", xStepSize, yStepSize);
        for (int x = xMin; x < xMax; x += xStepSize) {
          //printf("Reached row %d... \n", x + 1);
          for (int y = yMin; y < yMax; y += yStepSize) {
            // See if point is better match for any of markers
            const float ssd = calculateSSD(x, y, marker);
            if (ssd < bestSSD) {
                //printf("Marker %d Better ssd: %f... \n", i + 1, ssd);
                //printf("Better ssd: %f... \n", ssd);
                bestSSD = ssd;
                bestX = x;
                bestY = y;
            }
          }
        }

        if (xStepSize == 1 && yStepSize == 1) {
          xStepSize = 0;
          yStepSize = 0;
        } else {
          xStepSize = (xStepSize > 1) ? xStepSize / 2 : 1;
          yStepSize = (yStepSize > 1) ? yStepSize / 2 : 1;
          FINE_X = xStepSize * 2;
          FINE_Y = yStepSize * 2;

          xMin = fmax(0, bestX - FINE_X);
          xMax = fmin(width, bestX + FINE_X);
          yMin = fmax(0, bestY - FINE_Y);
          yMax = fmin(height, bestY + FINE_Y);
        }
      }

      // make new search window around best SSD 
      // TODO: will this give us issues re: local minima?
      // TODO: recursive descent
      // const size_t lowX = fmax(0, bestX - FINE_X);
      // const size_t hiX  = fmin (width, bestX + FINE_X);
      // const size_t lowY = fmax(0, bestY - FINE_Y);
      // const size_t hiY  = fmin (height, bestY + FINE_Y);
   
      // for (int x = lowX; x < hiX; ++ x) {
      //   for (int y = lowY; y < hiY; ++y) {
      //       // See if point is better match for any of markers
      //     const float ssd = calculateSSD(x, y, marker);
      //     if (ssd < bestSSD) {
      //       //printf("Marker %d Better ssd: %f... \n", i + 1, ssd);      
      //       bestSSD = ssd;
      //       bestX = x;
      //       bestY = y;
      //     }
      //   }
      // }

      markerLocations.push_back(Point(bestX, bestY));
    }
  }
}

float R2Image::
calculateSSD(const int x0, const int y0, R2Image& marker) {
  const int xReach = marker.Width() / 2;
  const int yReach = marker.Height() / 2;

  const int x1 = marker.Width() / 2;
  const int y1 = marker.Height() / 2;

  float sum = 0;

  for (int i = -xReach; i < xReach + 1; i++) {
    for (int j = -yReach; j < yReach + 1; j++) {
      // Don't have to check bounds for marker because work by construction
      if (inBounds(x0 + i, y0 + j)) {
          sum += ssd(Pixel(x0 + i, y0 + j), marker.Pixel(x1 + i, y1 + j));
      } else {
          // account for out of bounds pixels by adding max possible ssd
          sum += 3;
      }
    }
  }
  return sum;
}

///////////////////////
// Function Calls
//////////////////////
void R2Image::
FeatureDetector(double numFeatures) {
  // Find 
  std::vector<Feature> selectedFeatures;
  findFeatures(150, 10, false, *this, selectedFeatures);

  // Mark selected features
  for (int i = 0; i < selectedFeatures.size(); i++) {
    //printf("scale: %f \n", selectedFeatures[i].charScale);
    //drawSquare(selectedFeatures[i].x, selectedFeatures[i].y, 8 * selectedFeatures[i].charScale, 0, 1, 0);
    drawCircle(selectedFeatures[i].x, selectedFeatures[i].y, 10 * selectedFeatures[i].charScale, 0, 1, 0); 
  }
}

void R2Image::
TrackFeatures(int numFeatures, R2Image& originalImage) {
  // Search for matches (a in original image, b in this one)
  std::vector<FeatureMatch> matches;
  findMatches(numFeatures, numFeatures, originalImage, matches);

  classifyMatchesWithDltRANSAC(matches);
  drawMatches(matches);
}


void R2Image::
MatchHomography(R2Image& originalImage) {
  const int numFeatures = 150;
  // Search for matches (a in original image, b in this one)
  std::vector<FeatureMatch> matches;
  findMatches(numFeatures, numFeatures, originalImage, matches);

  // classify matches as good or bad
  classifyMatchesWithDltRANSAC(matches);

  // separate out good matches
  std::vector<FeatureMatch> goodMatches;
  for (int i = 0; i < matches.size(); i++) {
     if (matches[i].verifiedMatch) {
      goodMatches.push_back(matches[i]);
    }
  }

  // Compute homography matrix (this will be the matrix from original Image to this image as that is direction of feature matches)
  std::vector<double> homographyMatrix;
  computeHomographyMatrixWithDLT(goodMatches, homographyMatrix);
  
  transformImage(homographyMatrix);
  //blendWithImage(originalImage);
}


///////////////////////
// Match Finding
//////////////////////
void R2Image::
findMatches(const int numFeatures, const int numMatches, R2Image& originalImage, std::vector<FeatureMatch>& matches) {
  const int minFeatureDistance = 10;

  // Find features in the other image
  std::vector<Feature> selectedFeatures;
  findFeatures(numFeatures, minFeatureDistance, false, originalImage, selectedFeatures);

  // Search for matches
  const double featureSearchAreaPercentage = 0.3; // Don't make this smaller is messes us tracking on the face image
  const int ssdSearchRadius = 3;

  for (int f = 0; f < selectedFeatures.size(); f++) {   
      if (f >= numMatches) {
       break;
     }
      FeatureMatch match = findFeatureMatchConsecutiveImages(selectedFeatures[f], originalImage, featureSearchAreaPercentage, ssdSearchRadius);
      matches.push_back(match);
  }
}

FeatureMatch R2Image::
findFeatureMatchConsecutiveImages(const Feature& feature, R2Image& featureImage, const float searchAreaPercentage, const int ssdSearchRadius) {
  const Point searchOrigin(feature.x, feature.y);
  return findFeatureMatch(feature, featureImage, searchOrigin, searchAreaPercentage, ssdSearchRadius);
}

// Takes in feature from originalImage, looks for it in this image with ssd
FeatureMatch R2Image::
findFeatureMatch(const Feature& feature, R2Image& featureImage, const Point searchOrigin, const float searchAreaPercentage, const int ssdSearchRadius) {
    // Calculate search radii
    const int searchWidthRadius = width * searchAreaPercentage / 2;
    const int searchHeightRadius = height * searchAreaPercentage / 2;

    // initialize match with highest possible SSD
    const int maxPossibleSSD = (2 * ssdSearchRadius + 1) * (2 * ssdSearchRadius + 1) * 3;
    FeatureMatch match(feature, maxPossibleSSD);

    printf("Search Area: (%f, %f) -> (%f, %f) \n",fmax(0, searchOrigin.x - searchWidthRadius), fmax(0, searchOrigin.y - searchHeightRadius),
    fmin(width, searchOrigin.x + searchWidthRadius), fmin(height, searchOrigin.y + searchHeightRadius));
    

    // Search all pixels in search area to find most similar feature
    for (int x = fmax(0, searchOrigin.x - searchWidthRadius); x < fmin(width, searchOrigin.x + searchWidthRadius); x++) {
      for (int y = fmax(0, searchOrigin.y - searchHeightRadius); y < fmin(height, searchOrigin.y + searchHeightRadius); y++) {       
        //printf("Looking at pixel (%d, %d) \n", x, y);
        Pixel(x,y) = R2Pixel(0,1,0,1);
        // calculate ssd for possible match, save it if it is the best yet
        const float ssd = calculateSSD(x, y, feature.x, feature.y, featureImage, ssdSearchRadius);
        if (ssd < match.ssd) {
           match.ssd = ssd;
           match.b = Feature(Pixel(x, y), x, y);
        }
      }
    }
    Pixel(0, 0) = R2Pixel(0,1,0,1);
    printf("SSD: %f\n", match.ssd);
    return match;
}

float R2Image::
ssd(const R2Pixel& a, const R2Pixel& b) const {
  const float rDif = a.Red() - b.Red();
  const float gDif = a.Green() - b.Green();
  const float bDif = a.Blue() - b.Blue();
  return rDif * rDif + gDif * gDif + bDif * bDif;
}

// x0 and y0 associated with this image, x1 and y1 associated with the passed in "otherImage"
float R2Image::
calculateSSD(const int x0, const int y0, const int x1, const int y1, R2Image& otherImage, const int ssdSearchRadius) {
  float sum = 0;
  for (int i = -ssdSearchRadius; i < ssdSearchRadius + 1; i++) {
    for (int j = -ssdSearchRadius; j < ssdSearchRadius + 1; j++) {

      if (inBounds(x0 + i, y0 + j) && otherImage.inBounds(x1 + i, y1 + j)) {
          const R2Pixel& a = Pixel(x0 + i, y0 + j);
          const R2Pixel& b = otherImage.Pixel(x1 + i, y1 + j);
          
          const float rDif = a.Red() - b.Red();
          const float gDif = a.Green() - b.Green();
          const float bDif = a.Blue() - b.Blue();

          sum += (rDif * rDif + gDif * gDif + bDif * bDif);
        } else {
          // account for out of bounds pixels by assigning them a difference of half maximum
          sum += 3;
        }    
    }
  }   
    return sum;
}


///////////////////////
// Feature Finding
//////////////////////
void R2Image::
findFeatures(double numFeatures, double minDistance, bool scaleInvariant, R2Image& image, std::vector<Feature>& selectedFeatures) {
  std::vector<Feature> features;

  const double sigma = 2.0;
  R2Image harris(image);
  harris.Harris(sigma, false);

  // Grab all features above a certain luminance threshold (anything Harris identifies as as not a line or background)
  for (int i = 0; i < image.Width(); i++) {
    for (int j = 0; j < image.Height(); j++) {    
      if (scaleInvariant || (harris.Pixel(i, j).Luminance() > 0.5)) {
          features.push_back(Feature(harris.Pixel(i, j), i, j, sigma));
      }     
    }
  }
  // Finds characteristic scale for each point, sort them into feature scale groups (which are sorted from smallest to biggest)
  if (scaleInvariant) {
    findScaleInvariantHarrisFeaturePoints(features, image);
  }

  // Sort features from smallest to biggest
  std::sort(features.begin(), features.end()); 
  
  const int numFeaturesWanted = fmin(numFeatures, features.size());

  // Take most prominent features that are far enough apart
  for (int i = features.size() - 1; i > 0 ; i--) {
    bool shouldInsert = shouldInsertFeature(selectedFeatures, features[i], minDistance);
    if (shouldInsert) {
      selectedFeatures.push_back(features[i]);
    } 

    if (selectedFeatures.size() >= numFeaturesWanted) {
      break;
    }
  }
}


bool R2Image::
shouldInsertFeature(const std::vector<Feature>& features, const Feature& feature, const int minDistance) const {
  // Features array should be sorted from largest to smallest

  // Check to see if new feature smaller than any conflicting existing features
  for (int i = 0; i < features.size(); i++) {
    const Feature& existingFeature = features[i];

    // if larger existing feature conflicts return
    if ((feature.charScale == existingFeature.charScale) && feature.closeTo(existingFeature, minDistance * (feature.charScale - 1))) {
      return false;
    }
  }
  return true;
}

void R2Image::
findScaleInvariantHarrisFeaturePoints(std::vector<Feature>& features, R2Image& image) {
  const int size = features.size();

  // loop over sigma values, keeping track of largest harris values and associated sigmas
  for (int s = 1; s < 10; s++) {
    const double sigma = 2 * s;
    R2Image harris(image);
    harris.Harris(sigma, false);

    for (int i = 0; i < size; i++) {
      Feature& f = features[i];
      const R2Pixel& p = harris.Pixel(f.x, f.y);

      const double newL = p.Luminance();
      const double oldL = f.pixel.Luminance();

      if (newL > oldL) {
        f.pixel = p;
        f.charScale = sigma;
       }     
    }
  }
}

///////////////////////
// DLT
//////////////////////
void R2Image::
classifyMatchesWithDltRANSAC(std::vector<FeatureMatch>& matches) const {
  srand (time(NULL));

  std::vector<double> bestHomographyMatrix;
  int numGoodMatches = 0;

  const int numTrials = 1000;
  for (int i = 0; i < numTrials; i++) {
    int tempNumGoodMatches = 0;
    std::vector<double> tempHomographyMatrix;

    // Pick 4 matches randomly
    std::vector<FeatureMatch> correspondences;
    std::vector<int> selectedMatches;
    for (int j = 0; j < 4; j++) {
      int randMatch = rand() % matches.size();
      while (contains(randMatch, selectedMatches)) {
        randMatch = rand() % matches.size();
      }
      selectedMatches.push_back(randMatch);
      correspondences.push_back(matches[randMatch]);
    }

    // calculate tempHomographyMatrix with dlt
    computeHomographyMatrixWithDLT(correspondences, tempHomographyMatrix);

    // Find how many other vectors have similar motion vectors (keep track of largest set)
    for (int j = 0; j < matches.size(); j++) {
      const FeatureMatch& match = matches[j];
      const bool goodTransform = checkHomographySimilarity(match, tempHomographyMatrix);

      tempNumGoodMatches += (goodTransform ? 1 : 0);
    }

    if (tempNumGoodMatches > numGoodMatches) {
        numGoodMatches = tempNumGoodMatches;
        bestHomographyMatrix = tempHomographyMatrix;
    }
  }

  // Copy info about biggest set of good matches over to actual matches
  for (int a = 0; a < matches.size(); a++) {
    matches[a].verifiedMatch = checkHomographySimilarity(matches[a], bestHomographyMatrix);
  }
}   

void R2Image::
computeHomographyMatrixWithDLT(const std::vector<FeatureMatch>& matches, std::vector<double>& homographyMatrix) const {
  const bool shouldPrint = false;

  const int width = 9;
  int height = matches.size() * 2;

  // Fill line equations matrix from given point correspondences 
  double** lineEquations = dmatrix(1, height, 1, width);
  if (shouldPrint) { printf("Points: \n"); }
  // Construct matrix A (lineEquations)
  for (int i = 0; i < height; i += 2) {
     const int match = i / 2;
     const Feature& fa = matches[match].a;
     const Feature& fb = matches[match].b;

     // Not sure if it matters if are doubles so convert to points for now to ensure doubles (easy to change)
     const Point a(fa.x, fa.y);//a(matchesA[match][0], matchesA[match][1]); //a(fb.x, fb.y);
     const Point b(fb.x, fb.y);//b(matchesA[match][2], matchesA[match][3]);  //b(fa.x, fa.y);

     if (shouldPrint) { printf("(%f, %f) -> (%f, %f) \n", a.x, a.y, b.x, b.y); }

     const double row1[9] = { 0, 0, 0, -a.x, -a.y, -1, b.y * a.x, b.y * a.y, b.y };
     const double row2[9] = { a.x, a.y, 1, 0, 0, 0, -a.x * b.x, -b.x * a.y, -b.x };

     for (int j = 0; j < 9; j++) {
        lineEquations[i + 1][j + 1] = row1[j];
        lineEquations[i + 2][j + 1] = row2[j];
     }
  }

  if (shouldPrint) {
  printf("\n A: \n");
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      printf("%f ", lineEquations[i+1][j+1]);
    }
    printf("\n");
  }
  }
  
  // compute the SVD
  double** nullspaceMatrix = dmatrix(1, 10, 1, 10);
  const int numSingularValues = 9;
	double singularValues[numSingularValues + 1]; // 1..numSingularValues
   
	svdcmp(lineEquations, height, width, singularValues, nullspaceMatrix);

  if (shouldPrint) {
    printf("\n Singular Values: \n");
    for (int i = 1; i <= numSingularValues; i++) {
       printf("%f ", singularValues[i]);
    }
    printf("\n");

     printf("\n Nullspace Matrix: \n");
    for (int i = 0; i < 10; i++) {
      for (int j = 0; j < 10; j++) {
        printf("%f ", nullspaceMatrix[i + 1][j + 1]);
      }
      printf("\n");
    }
  }

  // Find row of smallest singular value 
  int solutionCol = 0;
  for (int i = 1; i < numSingularValues; i++) {
    if (singularValues[i + 1] < singularValues[solutionCol + 1]) {
      solutionCol = i;
    }
  }

  // Map solution column to 3x3 solution matrix (outdated description)
  for (int i = 0; i < 9; i++) {
    homographyMatrix.push_back(nullspaceMatrix[i + 1][solutionCol + 1]);
  }

  if (shouldPrint) {
      printf("\n H: \n");
      for (int i = 0; i < 3; i++) {
        printf("%f, %f, %f \n", homographyMatrix[3 * i], homographyMatrix[3 * i + 1], homographyMatrix[3 * i + 2]);
      }
  } 
}

void R2Image::
computeHomographyMatrixWithDLT(const std::vector<PointMatch>& matches, std::vector<double>& homographyMatrix) const {
  const bool shouldPrint = false;

  const int width = 9;
  int height = matches.size() * 2;

  // Fill line equations matrix from given point correspondences 
  double** lineEquations = dmatrix(1, height, 1, width);
  if (shouldPrint) { printf("Points: \n"); }
  // Construct matrix A (lineEquations)
  for (int i = 0; i < height; i += 2) {
     const int match = i / 2;
     const PointMatch& pm = matches[match];

     const Point a = pm.a;
     const Point b = pm.b;

     if (shouldPrint) { printf("(%f, %f) -> (%f, %f) \n", a.x, a.y, b.x, b.y); }

     const double row1[9] = { 0, 0, 0, -a.x, -a.y, -1, b.y * a.x, b.y * a.y, b.y };
     const double row2[9] = { a.x, a.y, 1, 0, 0, 0, -a.x * b.x, -b.x * a.y, -b.x };

     for (int j = 0; j < 9; j++) {
        lineEquations[i + 1][j + 1] = row1[j];
        lineEquations[i + 2][j + 1] = row2[j];
     }
  }

  if (shouldPrint) {
  printf("\n A: \n");
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      printf("%f ", lineEquations[i+1][j+1]);
    }
    printf("\n");
  }
  }
  
  // compute the SVD
  double** nullspaceMatrix = dmatrix(1, 10, 1, 10);
  const int numSingularValues = 9;
	double singularValues[numSingularValues + 1]; // 1..numSingularValues
   
	svdcmp(lineEquations, height, width, singularValues, nullspaceMatrix);

  if (shouldPrint) {
    printf("\n Singular Values: \n");
    for (int i = 1; i <= numSingularValues; i++) {
       printf("%f ", singularValues[i]);
    }
    printf("\n");

     printf("\n Nullspace Matrix: \n");
    for (int i = 0; i < 10; i++) {
      for (int j = 0; j < 10; j++) {
        printf("%f ", nullspaceMatrix[i + 1][j + 1]);
      }
      printf("\n");
    }
  }

  // Find row of smallest singular value 
  int solutionCol = 0;
  for (int i = 1; i < numSingularValues; i++) {
    if (singularValues[i + 1] < singularValues[solutionCol + 1]) {
      solutionCol = i;
    }
  }

  // Map solution column to 3x3 solution matrix (outdated description)
  for (int i = 0; i < 9; i++) {
    homographyMatrix.push_back(nullspaceMatrix[i + 1][solutionCol + 1]);
  }

  if (shouldPrint) {
      printf("\n H: \n");
      for (int i = 0; i < 3; i++) {
        printf("%f, %f, %f \n", homographyMatrix[3 * i], homographyMatrix[3 * i + 1], homographyMatrix[3 * i + 2]);
      }
  } 
}


bool R2Image::
checkHomographySimilarity(const FeatureMatch& match, const std::vector<double>& homographyMatrix) const {
   const Feature& a = match.a;
   const Feature& real_b = match.b;

   Point calculated_b = transformPoint(a.x, a.y, homographyMatrix);

   const int threshold = 5;
   const int xDiff = std::abs(real_b.x - calculated_b.x);
   const int yDiff = std::abs(real_b.y - calculated_b.y);

  return (xDiff < threshold) && (yDiff < threshold);
}

bool R2Image:: 
contains(const int number, const std::vector<int>& vector) const {
  for (int i = 0; i < vector.size(); i++) {
    if(vector[i] == number) return true;
  }
  return false;
}


///////////////////////
// Image Transformation
//////////////////////

Point R2Image::
transformPoint(const int x0, const int y0, const std::vector<double>& homographyMatrix) const {
  double x = homographyMatrix[0] * x0 + homographyMatrix[1] * y0 + homographyMatrix[2];
  double y = homographyMatrix[3] * x0 + homographyMatrix[4] * y0 + homographyMatrix[5];
  const double w = homographyMatrix[6] * x0 + homographyMatrix[7] * y0 + homographyMatrix[8];

  //printf("x: %f, y: %f, w: %f \n", x, y, w);

  // normalize with w
  x /= w;
  y /= w;

  return Point(x, y);
}

void R2Image::
transformImage(const std::vector<double>& homographyMatrix) {
  R2Image transformedImage(width, height);

  for (int i = 0; i < width; i++) {
    for (int j = 0;  j < height; j++) {
      const Point p = transformPoint(i, j, homographyMatrix);
      const int x0 = p.x;
      const int y0 = p.y;
      
      const int x1 = x0 + 1;
      const int y1 = y0 + 1;

      if (inBounds(x0, y0) && inBounds(x1, y1)) {
        const double alphaX = p.x - x0;
        const double alphaY = p.y - y0;

        R2Pixel upperHalf = (1 - alphaX) * Pixel(x0, y0) + alphaX * Pixel(x1, y0);
        R2Pixel lowerHalf = (1 - alphaX) * Pixel(x0, y1) + alphaX * Pixel(x1, y1);

        transformedImage.Pixel(i, j) = (1 - alphaY) * upperHalf + alphaY * lowerHalf;
      } else if (inBounds(x0, y0)) {
        transformedImage.Pixel(i, j) = Pixel(x0, y0);
      } else if (inBounds(x1,y1)) {
        transformedImage.Pixel(i, j) = Pixel(x1, y1);
      }
    }
  }

  *this = transformedImage;
}


void R2Image::
blendWithImage(R2Image& otherImage) {
  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      Pixel(i,j) = 0.5 * Pixel(i, j) + 0.5 * otherImage.Pixel(i, j);
    }
  }
}


///////////////////////
// Basic Filtering
//////////////////////

void R2Image::
SobelX(void)
{
  int sobelY[3][3] = {
    {1, 0, -1},
    {2, 0, -2},
    {1, 0, -1}
  };

  applyFilter3x3(sobelY);
}

void R2Image::
SobelY(void)
{
	// Apply the Sobel oprator to the image in Y direction
  int sobelX[3][3] = {
    {-1, -2, -1},
    {0, 0, 0},
    {1, 2, 1}
  };

	// Apply the Sobel oprator to the image in X direction
  applyFilter3x3(sobelX);
}

// Apply a 3x3 filter to an image
void R2Image::
applyFilter3x3( int filter[3][3]) {
  R2Image tempImage(width, height);

  for (int i = 1; i < width - 1; i++) {
    for (int j = 1;  j < height - 1; j++) {
      // Save effect of filter into temp image
      R2Pixel tempPixel;
      for (int a = 0; a < 3; a++) {
        for (int b = 0; b < 3; b++) {
          tempPixel += filter[a][b] * Pixel(i - 1 + a, j - 1 + b);
        }
      }
      //tempPixel.Clamp();
      tempImage.Pixel(i, j) = tempPixel;
    }
  }

  // Copy changes back from temp image
  for (int i = 1; i < width; i++) {
    for (int j = 1;  j < height; j++) {
      Pixel(i, j) = tempImage.Pixel(i, j);
    }
  }
}

// Calculates Gaussian distribution value
double gaussianDistribution(double sigma, int x) {
  double frac = 1.0 / (sqrt(2.0 * M_PI) * sigma);
  double expo = exp(-(x * x) / (2.0 * sigma * sigma));
  return frac * expo;
}

// Linear filtering ////////////////////////////////////////////////
void R2Image::
Blur(double sigma, bool clamped)
{
  R2Image tempImage(width, height);
  const int kernelLength = 6 * sigma + 1;
  const int kernelReach = 3 * sigma;

  // Construct kernel
  double gaussianKernel[kernelLength];
  for (int i = 0; i < kernelLength; i++) {
    gaussianKernel[i] = gaussianDistribution(sigma, i - kernelReach);
  }

  // Vertical pass
  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      R2Pixel tempPixel;
      double weightSum = 0;

      const int lowerBound = fmax(-kernelReach, -j);
      const int upperBound = fmin(kernelReach + 1, height - j);
      for (int a = lowerBound; a < upperBound; a++) {
        tempPixel += Pixel(i, j + a) * gaussianKernel[a + kernelReach];
        weightSum += gaussianKernel[a + kernelReach];
      }
      tempPixel /= weightSum;
      if (clamped) {
        tempPixel.Clamp();
      }
      tempImage.Pixel(i, j) = tempPixel;
    }
  }

  // Horizontal pass
  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      R2Pixel tempPixel;
      double weightSum = 0;
      const int lowerBound = fmax(-kernelReach, -i);
      const int upperBound = fmin(kernelReach + 1, width - i);
      for (int a = lowerBound; a < upperBound; a++) {
        tempPixel += tempImage.Pixel(i + a, j) * gaussianKernel[a + kernelReach];
        weightSum += gaussianKernel[a + kernelReach];
      }
      tempPixel /= weightSum;
      if (clamped) {
        tempPixel.Clamp();
      }
      Pixel(i, j) = tempPixel;
    }
  }
}

///////////////////////
// Drawing
//////////////////////

void R2Image::
drawMatches(const std::vector<FeatureMatch> matches) {
  for (int i = 0; i < matches.size(); i++) {
    const FeatureMatch& match = matches[i];
    const Feature& a = matches[i].a;
    const Feature& b = matches[i].b;

    if (match.verifiedMatch) {
      // Green box on new point and line from old point
      drawSquare(b.x, b.y, 5, 0, 1, 0);
      drawLine(a.x, a.y, b.x, b.y, 0, 1, 0);
    } else {
      // Red box on original point
      //drawSquare(a.x, a.y, 2, 1, 0, 0);

      // red box on new point, and line from old point
      drawSquare(b.x, b.y, 5, 1, 0, 0);
      drawLine(a.x, a.y, b.x, b.y, 1, 0, 0);
    }
  }
}

void R2Image::
drawCircle(const int x0, const int y0, const int radius, const float r, const float g, const float b) {
  int x = radius;
  int y = 0;
  int error = 0;

  while (x >= y) {
    if (inBounds(x0 + x, y0 + y)) Pixel(x0 + x, y0 + y) = R2Pixel(r, g, b, 1);
    if (inBounds(x0 + y, y0 + x)) Pixel(x0 + y, y0 + x) = R2Pixel(r, g, b, 1);
    if (inBounds(x0 - x, y0 + y)) Pixel(x0 - x, y0 + y) = R2Pixel(r, g, b, 1);
    if (inBounds(x0 - y, y0 + x)) Pixel(x0 - y, y0 + x) = R2Pixel(r, g, b, 1);
    if (inBounds(x0 - x, y0 - y)) Pixel(x0 - x, y0 - y) = R2Pixel(r, g, b, 1);
    if (inBounds(x0 - y, y0 - x)) Pixel(x0 - y, y0 - x) = R2Pixel(r, g, b, 1);
    if (inBounds(x0 + x, y0 - y)) Pixel(x0 + x, y0 - y) = R2Pixel(r, g, b, 1);
    if (inBounds(x0 + y, y0 - x)) Pixel(x0 + y, y0 - x) = R2Pixel(r, g, b, 1);

    if (error <= 0) {
      y += 1;
      error += 2 * y + 1;
    } else {
      x -= 1;
      error -= 2 * x + 1;
    }
  }
}

void R2Image::
drawFilledSquare(const int x, const int y, const int reach, const float r, const float g, const float b) {
    const int xMax = fmin(width - 1, x + reach);
    const int xMin = fmax(0, x - reach);
    const int yMax = fmin(width - 1, y + reach);
    const int yMin = fmax(0, y - reach);
    
    for (int x = xMin; x < xMax; x++) {
        for (int y = yMin; y < yMax; y++) {
            Pixel(x, y) = R2Pixel(r, g, b, 1);
        }
    }
}


void R2Image::
drawSquare(const int x, const int y, const int reach, const float r, const float g, const float b) {
  const int xMax = fmin(width - 1, x + reach);
  const int xMin = fmax(0, x - reach);
  const int yMax = fmin(width - 1, y + reach);
  const int yMin = fmax(0, y - reach);

  for (int i = -reach; i < reach + 1; i++) {
    const int xi = fmax(0, fmin(width-1, x + i));
    const int yi = fmax(0, fmin(height-1, y + i));

    Pixel(xMin, yi) = R2Pixel(r, g, b, 1);
    Pixel(xMax, yi) = R2Pixel(r, g, b, 1);
    Pixel(xi, yMin) = R2Pixel(r, g, b, 1);
    Pixel(xi, yMax) = R2Pixel(r, g, b, 1);
  }
}

void R2Image::
drawLine(int x0, int y0, int x1, int y1, const float r, const float g, const float b) {
  if (x0 > 3 && x0 < width - 3 && y0 > 3 && y0 < height - 3) {
		 for (int x = x0 - 3; x <= x0 + 3; x++) {
			 for (int y = y0-3 ; y <= y0 + 3; y++) {
				 Pixel(x,y).Reset(r,g,b,1.0);
			 }
		 }
	 }

	if (x0 > x1) {
		int x = y1; y1 = y0; y0 = x;
    x = x1; x1 = x0; x0 = x;
	}
  int deltax = x1 - x0;
  int deltay = y1 - y0;
  float error = 0; 
  float deltaerr = 0.0;
	if(deltax!=0) deltaerr =fabs(float(float(deltay) / deltax));    // Assume deltax != 0 (line is not vertical),
           // note that this division needs to be done in a way that preserves the fractional part
  int y = y0;
  for(int x = x0; x <= x1; x++) {
		Pixel(x,y).Reset(r,g,b,1.0);
    error = error + deltaerr;
    if (error >= 0.5) {
			 if (deltay > 0) y = y + 1;
			 else y = y - 1;

       error = error - 1.0;
		 }
	 }
}






////////////////////////////////////////////////////////////////////////
// Constructors/Destructors
////////////////////////////////////////////////////////////////////////


R2Image::
R2Image(void)
  : pixels(NULL),
    npixels(0),
    width(0),
    height(0)
{
}



R2Image::
R2Image(const char *filename)
  : pixels(NULL),
    npixels(0),
    width(0),
    height(0)
{
  // Read image
  Read(filename);
}



R2Image::
R2Image(int width, int height)
  : pixels(NULL),
    npixels(width * height),
    width(width),
    height(height)
{
  // Allocate pixels
  pixels = new R2Pixel [ npixels ];
  assert(pixels);
}



R2Image::
R2Image(int width, int height, const R2Pixel *p)
  : pixels(NULL),
    npixels(width * height),
    width(width),
    height(height)
{
  // Allocate pixels
  pixels = new R2Pixel [ npixels ];
  assert(pixels);

  // Copy pixels
  for (int i = 0; i < npixels; i++)
    pixels[i] = p[i];
}



R2Image::
R2Image(const R2Image& image)
  : pixels(NULL),
    npixels(image.npixels),
    width(image.width),
    height(image.height)

{
  // Allocate pixels
  pixels = new R2Pixel [ npixels ];
  assert(pixels);

  // Copy pixels
  for (int i = 0; i < npixels; i++)
    pixels[i] = image.pixels[i];
}



R2Image::
~R2Image(void)
{
  // Free image pixels
  if (pixels) delete [] pixels;
}



R2Image& R2Image::
operator=(const R2Image& image)
{
  // Delete previous pixels
  if (pixels) { delete [] pixels; pixels = NULL; }

  // Reset width and height
  npixels = image.npixels;
  width = image.width;
  height = image.height;

  // Allocate new pixels
  pixels = new R2Pixel [ npixels ];
  assert(pixels);

  // Copy pixels
  for (int i = 0; i < npixels; i++)
    pixels[i] = image.pixels[i];

  // Return image
  return *this;
}


void R2Image::
svdTest(void)
{
	// fit a 2D conic to five points
	R2Point p1(1.2,3.5);
	R2Point p2(2.1,2.2);
	R2Point p3(0.2,1.6);
	R2Point p4(0.0,0.5);
	R2Point p5(-0.2,4.2);

	// build the 5x6 matrix of equations
	double** linEquations = dmatrix(1,5,1,6);

	linEquations[1][1] = p1[0]*p1[0];
	linEquations[1][2] = p1[0]*p1[1];
	linEquations[1][3] = p1[1]*p1[1];
	linEquations[1][4] = p1[0];
	linEquations[1][5] = p1[1];
	linEquations[1][6] = 1.0;

	linEquations[2][1] = p2[0]*p2[0];
	linEquations[2][2] = p2[0]*p2[1];
	linEquations[2][3] = p2[1]*p2[1];
	linEquations[2][4] = p2[0];
	linEquations[2][5] = p2[1];
	linEquations[2][6] = 1.0;

	linEquations[3][1] = p3[0]*p3[0];
	linEquations[3][2] = p3[0]*p3[1];
	linEquations[3][3] = p3[1]*p3[1];
	linEquations[3][4] = p3[0];
	linEquations[3][5] = p3[1];
	linEquations[3][6] = 1.0;

	linEquations[4][1] = p4[0]*p4[0];
	linEquations[4][2] = p4[0]*p4[1];
	linEquations[4][3] = p4[1]*p4[1];
	linEquations[4][4] = p4[0];
	linEquations[4][5] = p4[1];
	linEquations[4][6] = 1.0;

	linEquations[5][1] = p5[0]*p5[0];
	linEquations[5][2] = p5[0]*p5[1];
	linEquations[5][3] = p5[1]*p5[1];
	linEquations[5][4] = p5[0];
	linEquations[5][5] = p5[1];
	linEquations[5][6] = 1.0;

	printf("\n Fitting a conic to five points:\n");
	printf("Point #1: %f,%f\n",p1[0],p1[1]);
	printf("Point #2: %f,%f\n",p2[0],p2[1]);
	printf("Point #3: %f,%f\n",p3[0],p3[1]);
	printf("Point #4: %f,%f\n",p4[0],p4[1]);
	printf("Point #5: %f,%f\n",p5[0],p5[1]);

	// compute the SVD
	double singularValues[7]; // 1..6
	double** nullspaceMatrix = dmatrix(1,6,1,6);
  
	svdcmp(linEquations, 5, 6, singularValues, nullspaceMatrix);

	// get the result
	printf("\n Singular values: %f, %f, %f, %f, %f, %f\n",singularValues[1],singularValues[2],singularValues[3],singularValues[4],singularValues[5],singularValues[6]);

	// find the smallest singular value:
	int smallestIndex = 1;
	for(int i=2;i<7;i++) if(singularValues[i]<singularValues[smallestIndex]) smallestIndex=i;

	// solution is the nullspace of the matrix, which is the column in V corresponding to the smallest singular value (which should be 0)
	printf("Conic coefficients: %f, %f, %f, %f, %f, %f\n",nullspaceMatrix[1][smallestIndex],nullspaceMatrix[2][smallestIndex],nullspaceMatrix[3][smallestIndex],nullspaceMatrix[4][smallestIndex],nullspaceMatrix[5][smallestIndex],nullspaceMatrix[6][smallestIndex]);

	// make sure the solution is correct:
	printf("Equation #1 result: %f\n",	p1[0]*p1[0]*nullspaceMatrix[1][smallestIndex] +
										p1[0]*p1[1]*nullspaceMatrix[2][smallestIndex] +
										p1[1]*p1[1]*nullspaceMatrix[3][smallestIndex] +
										p1[0]*nullspaceMatrix[4][smallestIndex] +
										p1[1]*nullspaceMatrix[5][smallestIndex] +
										nullspaceMatrix[6][smallestIndex]);

	printf("Equation #2 result: %f\n",	p2[0]*p2[0]*nullspaceMatrix[1][smallestIndex] +
										p2[0]*p2[1]*nullspaceMatrix[2][smallestIndex] +
										p2[1]*p2[1]*nullspaceMatrix[3][smallestIndex] +
										p2[0]*nullspaceMatrix[4][smallestIndex] +
										p2[1]*nullspaceMatrix[5][smallestIndex] +
										nullspaceMatrix[6][smallestIndex]);

	printf("Equation #3 result: %f\n",	p3[0]*p3[0]*nullspaceMatrix[1][smallestIndex] +
										p3[0]*p3[1]*nullspaceMatrix[2][smallestIndex] +
										p3[1]*p3[1]*nullspaceMatrix[3][smallestIndex] +
										p3[0]*nullspaceMatrix[4][smallestIndex] +
										p3[1]*nullspaceMatrix[5][smallestIndex] +
										nullspaceMatrix[6][smallestIndex]);

	printf("Equation #4 result: %f\n",	p4[0]*p4[0]*nullspaceMatrix[1][smallestIndex] +
										p4[0]*p4[1]*nullspaceMatrix[2][smallestIndex] +
										p4[1]*p4[1]*nullspaceMatrix[3][smallestIndex] +
										p4[0]*nullspaceMatrix[4][smallestIndex] +
										p4[1]*nullspaceMatrix[5][smallestIndex] +
										nullspaceMatrix[6][smallestIndex]);

	printf("Equation #5 result: %f\n",	p5[0]*p5[0]*nullspaceMatrix[1][smallestIndex] +
										p5[0]*p5[1]*nullspaceMatrix[2][smallestIndex] +
										p5[1]*p5[1]*nullspaceMatrix[3][smallestIndex] +
										p5[0]*nullspaceMatrix[4][smallestIndex] +
										p5[1]*nullspaceMatrix[5][smallestIndex] +
										nullspaceMatrix[6][smallestIndex]);

	R2Point test_point(0.34,-2.8);

	printf("A point off the conic: %f\n",	test_point[0]*test_point[0]*nullspaceMatrix[1][smallestIndex] +
											test_point[0]*test_point[1]*nullspaceMatrix[2][smallestIndex] +
											test_point[1]*test_point[1]*nullspaceMatrix[3][smallestIndex] +
											test_point[0]*nullspaceMatrix[4][smallestIndex] +
											test_point[1]*nullspaceMatrix[5][smallestIndex] +
											nullspaceMatrix[6][smallestIndex]);

	return;
}



////////////////////////////////////////////////////////////////////////
// Image processing functions
// YOU IMPLEMENT THE FUNCTIONS IN THIS SECTION
////////////////////////////////////////////////////////////////////////

// Per-pixel Operations ////////////////////////////////////////////////

void R2Image::
Brighten(double factor)
{
  // Brighten the image by multiplying each pixel component by the factor.
  // This is implemented for you as an example of how to access and set pixels
  for (int i = 0; i < width; i++) {
    for (int j = 0;  j < height; j++) {
      Pixel(i,j) *= factor;
      Pixel(i,j).Clamp();
    }
  }
}



void R2Image::
HighPass(double sigma, double contrast) {
  R2Image highPass(*this);
  // Put major details in highPass
  highPass.Blur(sigma, true);
  for(int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      highPass.Pixel(i, j) = Pixel(i,j) - highPass.Pixel(i, j); 
    }
  }

// Recombine lowpass with weighted highpass for extra contrast
 Blur(sigma, true);
 for(int i = 0; i < width; i++) {
    for (int j = 0; j< height; j++) {
      Pixel(i, j) += highPass.Pixel(i, j) * contrast;
      Pixel(i, j).Clamp(); 
    }
 }
}

void R2Image::
Harris(double sigma, bool clamped)
{
  // Harris corner detector. Make use of the previously developed filters, such as the Gaussian blur filter
	// Output should be 50% grey at flat regions, white at corners and black/dark near edges

  R2Image xSquared(*this);
  R2Image ySquared(*this);

  // Why can't I initialize this image with just width and height? get segfault
  R2Image xy(*this);

  xSquared.SobelX();
  ySquared.SobelY();

  // Fill images with correct values;
  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      // For now xSquared and ySquared just contain x and y at (i, j)
      xy.Pixel(i, j) = xSquared.Pixel(i, j) * ySquared.Pixel(i, j);
      xSquared.Pixel(i, j) *= xSquared.Pixel(i, j);
      ySquared.Pixel(i, j) *= ySquared.Pixel(i, j);
    }
  }

  xSquared.Blur(sigma, false);
  ySquared.Blur(sigma, false);
  xy.Blur(sigma,false);

  for(int i = 0; i < width; i++) {
    for(int j = 0; j < height; j++) {
      Pixel(i, j) = (xSquared.Pixel(i, j) * ySquared.Pixel(i, j)) - (xy.Pixel(i, j) * xy.Pixel(i, j)) - 0.04 * ((xSquared.Pixel(i, j) + ySquared.Pixel(i, j)) * (xSquared.Pixel(i, j) + ySquared.Pixel(i, j)));
      R2Pixel shift(0.5, 0.5, 0.5, 1);
      Pixel(i, j) += shift;
      if (clamped) {
        Pixel(i, j).Clamp();
      }    
    }
  }
}

bool R2Image::
similarMotion(const FeatureMatch& a, const FeatureMatch& b) const {
  const int threshold = 5;

  const int a_xDiff = a.a.x - a.b.x;
  const int b_xDiff = b.a.x - b.b.x;
  const int xDiff = abs(a_xDiff - b_xDiff);

  const int a_yDiff = a.a.y - a.b.y;
  const int b_yDiff = b.a.y - b.b.y;
  const int yDiff = abs(a_yDiff - b_yDiff);

  return (xDiff < threshold) && (yDiff < threshold);
}


void R2Image::
classifyMatchesWithRANSAC(std::vector<FeatureMatch>& matches) const {
  srand (time(NULL));

  std::vector<bool> goodMatches(matches.size());
  FeatureMatch& bestMotionVector = matches[0];
  int numGoodMatches = 0;

  const int numTrials = 25;
  for (int i = 0; i < numTrials; i++) {
    int tempNumGoodMatches = 0;

    // choose random match vector
    const int randNum = rand() % matches.size();
    const FeatureMatch& a = matches[randNum];

    // Find how many other vectors have similar motion vectors (keep track of largest set)
    for (int j = 0; j < matches.size(); j++) {
      const FeatureMatch& b = matches[j];
      tempNumGoodMatches += (similarMotion(a, b) ? 1 : 0);
    }

    if (tempNumGoodMatches > numGoodMatches) {
        numGoodMatches = tempNumGoodMatches;
        bestMotionVector = a;
    }
  }

  // Copy info about biggest set of good matches over to actual matches
  for (int a = 0; a < matches.size(); a++) {
    matches[a].verifiedMatch = similarMotion(bestMotionVector, matches[a]);
  }
}

void R2Image::
testDLT() {
  std::vector<FeatureMatch> hy;
  std::vector<double> h;
  computeHomographyMatrixWithDLT(hy,h);
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

void R2Image::
Sharpen() {
  // Sharpen an image using a linear filter. Use a kernel of your choosing.
  int sharpen[3][3] = {
    {0, -1, 0},
    {-1, 5, -1},
    {0, -1, 0}
  };

  applyFilter3x3(sharpen);
}


void R2Image::
blendOtherImageTranslated(R2Image * otherImage)
{
	// find at least 100 features on this image, and another 100 on the "otherImage". Based on these,
	// compute the matching translation (pixel precision is OK), and blend the translated "otherImage"
	// into this image with a 50% opacity.
	fprintf(stderr, "fit other image using translation and blend imageB over imageA\n");
	return;
}

void R2Image::
blendOtherImageHomography(R2Image * otherImage)
{
	// find at least 100 features on this image, and another 100 on the "otherImage". Based on these,
	// compute the matching homography, and blend the transformed "otherImage" into this image with a 50% opacity.
	fprintf(stderr, "fit other image using a homography and blend imageB over imageA\n");
	return;
}

////////////////////////////////////////////////////////////////////////
// I/O Functions
////////////////////////////////////////////////////////////////////////

int R2Image::
Read(const char *filename)
{
  // Initialize everything
  if (pixels) { delete [] pixels; pixels = NULL; }
  npixels = width = height = 0;

  // Parse input filename extension
  char *input_extension;
  if (!(input_extension = (char*)strrchr(filename, '.'))) {
    fprintf(stderr, "Input file has no extension (e.g., .jpg).\n");
    return 0;
  }

  // Read file of appropriate type
  if (!strncmp(input_extension, ".bmp", 4)) return ReadBMP(filename);
  else if (!strncmp(input_extension, ".ppm", 4)) return ReadPPM(filename);
  else if (!strncmp(input_extension, ".jpg", 4)) return ReadJPEG(filename);
  else if (!strncmp(input_extension, ".jpeg", 5)) return ReadJPEG(filename);

  // Should never get here
  fprintf(stderr, "Unrecognized image file extension");
  return 0;
}



int R2Image::
Write(const char *filename) const
{
  // Parse input filename extension
  char *input_extension;
  if (!(input_extension = (char*)strrchr(filename, '.'))) {
    fprintf(stderr, "Input file has no extension (e.g., .jpg).\n");
    return 0;
  }

  // Write file of appropriate type
  if (!strncmp(input_extension, ".bmp", 4)) return WriteBMP(filename);
  else if (!strncmp(input_extension, ".ppm", 4)) return WritePPM(filename, 1);
  else if (!strncmp(input_extension, ".jpg", 5)) return WriteJPEG(filename);
  else if (!strncmp(input_extension, ".jpeg", 5)) return WriteJPEG(filename);

  // Should never get here
  fprintf(stderr, "Unrecognized image file extension");
  return 0;
}



////////////////////////////////////////////////////////////////////////
// BMP I/O
////////////////////////////////////////////////////////////////////////

#if (RN_OS == RN_LINUX) && !WIN32

typedef struct tagBITMAPFILEHEADER {
  unsigned short int bfType;
  unsigned int bfSize;
  unsigned short int bfReserved1;
  unsigned short int bfReserved2;
  unsigned int bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
  unsigned int biSize;
  int biWidth;
  int biHeight;
  unsigned short int biPlanes;
  unsigned short int biBitCount;
  unsigned int biCompression;
  unsigned int biSizeImage;
  int biXPelsPerMeter;
  int biYPelsPerMeter;
  unsigned int biClrUsed;
  unsigned int biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagRGBTRIPLE {
  unsigned char rgbtBlue;
  unsigned char rgbtGreen;
  unsigned char rgbtRed;
} RGBTRIPLE;

typedef struct tagRGBQUAD {
  unsigned char rgbBlue;
  unsigned char rgbGreen;
  unsigned char rgbRed;
  unsigned char rgbReserved;
} RGBQUAD;

#endif

#define BI_RGB        0L
#define BI_RLE8       1L
#define BI_RLE4       2L
#define BI_BITFIELDS  3L

#define BMP_BF_TYPE 0x4D42 /* word BM */
#define BMP_BF_OFF_BITS 54 /* 14 for file header + 40 for info header (not sizeof(), but packed size) */
#define BMP_BI_SIZE 40 /* packed size of info header */


static unsigned short int WordReadLE(FILE *fp)
{
  // Read a unsigned short int from a file in little endian format
  unsigned short int lsb, msb;
  lsb = getc(fp);
  msb = getc(fp);
  return (msb << 8) | lsb;
}



static void WordWriteLE(unsigned short int x, FILE *fp)
{
  // Write a unsigned short int to a file in little endian format
  unsigned char lsb = (unsigned char) (x & 0x00FF); putc(lsb, fp);
  unsigned char msb = (unsigned char) (x >> 8); putc(msb, fp);
}



static unsigned int DWordReadLE(FILE *fp)
{
  // Read a unsigned int word from a file in little endian format
  unsigned int b1 = getc(fp);
  unsigned int b2 = getc(fp);
  unsigned int b3 = getc(fp);
  unsigned int b4 = getc(fp);
  return (b4 << 24) | (b3 << 16) | (b2 << 8) | b1;
}



static void DWordWriteLE(unsigned int x, FILE *fp)
{
  // Write a unsigned int to a file in little endian format
  unsigned char b1 = (x & 0x000000FF); putc(b1, fp);
  unsigned char b2 = ((x >> 8) & 0x000000FF); putc(b2, fp);
  unsigned char b3 = ((x >> 16) & 0x000000FF); putc(b3, fp);
  unsigned char b4 = ((x >> 24) & 0x000000FF); putc(b4, fp);
}



static int LongReadLE(FILE *fp)
{
  // Read a int word from a file in little endian format
  int b1 = getc(fp);
  int b2 = getc(fp);
  int b3 = getc(fp);
  int b4 = getc(fp);
  return (b4 << 24) | (b3 << 16) | (b2 << 8) | b1;
}



static void LongWriteLE(int x, FILE *fp)
{
  // Write a int to a file in little endian format
  char b1 = (x & 0x000000FF); putc(b1, fp);
  char b2 = ((x >> 8) & 0x000000FF); putc(b2, fp);
  char b3 = ((x >> 16) & 0x000000FF); putc(b3, fp);
  char b4 = ((x >> 24) & 0x000000FF); putc(b4, fp);
}



int R2Image::
ReadBMP(const char *filename)
{
  // Open file
  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    fprintf(stderr, "Unable to open image file: %s", filename);
    return 0;
  }

  /* Read file header */
  BITMAPFILEHEADER bmfh;
  bmfh.bfType = WordReadLE(fp);
  bmfh.bfSize = DWordReadLE(fp);
  bmfh.bfReserved1 = WordReadLE(fp);
  bmfh.bfReserved2 = WordReadLE(fp);
  bmfh.bfOffBits = DWordReadLE(fp);

  /* Check file header */
  assert(bmfh.bfType == BMP_BF_TYPE);
  /* ignore bmfh.bfSize */
  /* ignore bmfh.bfReserved1 */
  /* ignore bmfh.bfReserved2 */
  assert(bmfh.bfOffBits == BMP_BF_OFF_BITS);

  /* Read info header */
  BITMAPINFOHEADER bmih;
  bmih.biSize = DWordReadLE(fp);
  bmih.biWidth = LongReadLE(fp);
  bmih.biHeight = LongReadLE(fp);
  bmih.biPlanes = WordReadLE(fp);
  bmih.biBitCount = WordReadLE(fp);
  bmih.biCompression = DWordReadLE(fp);
  bmih.biSizeImage = DWordReadLE(fp);
  bmih.biXPelsPerMeter = LongReadLE(fp);
  bmih.biYPelsPerMeter = LongReadLE(fp);
  bmih.biClrUsed = DWordReadLE(fp);
  bmih.biClrImportant = DWordReadLE(fp);

  // Check info header
  assert(bmih.biSize == BMP_BI_SIZE);
  assert(bmih.biWidth > 0);
  assert(bmih.biHeight > 0);
  assert(bmih.biPlanes == 1);
  assert(bmih.biBitCount == 24);  /* RGB */
  assert(bmih.biCompression == BI_RGB);   /* RGB */
  int lineLength = bmih.biWidth * 3;  /* RGB */
  if ((lineLength % 4) != 0) lineLength = (lineLength / 4 + 1) * 4;
  assert(bmih.biSizeImage == (unsigned int) lineLength * (unsigned int) bmih.biHeight);

  // Assign width, height, and number of pixels
  width = bmih.biWidth;
  height = bmih.biHeight;
  npixels = width * height;

  // Allocate unsigned char buffer for reading pixels
  int rowsize = 3 * width;
  if ((rowsize % 4) != 0) rowsize = (rowsize / 4 + 1) * 4;
  int nbytes = bmih.biSizeImage;
  unsigned char *buffer = new unsigned char [nbytes];
  if (!buffer) {
    fprintf(stderr, "Unable to allocate temporary memory for BMP file");
    fclose(fp);
    return 0;
  }

  // Read buffer
  fseek(fp, (long) bmfh.bfOffBits, SEEK_SET);
  if (fread(buffer, 1, bmih.biSizeImage, fp) != bmih.biSizeImage) {
    fprintf(stderr, "Error while reading BMP file %s", filename);
    return 0;
  }

  // Close file
  fclose(fp);

  // Allocate pixels for image
  pixels = new R2Pixel [ width * height ];
  if (!pixels) {
    fprintf(stderr, "Unable to allocate memory for BMP file");
    fclose(fp);
    return 0;
  }

  // Assign pixels
  for (int j = 0; j < height; j++) {
    unsigned char *p = &buffer[j * rowsize];
    for (int i = 0; i < width; i++) {
      double b = (double) *(p++) / 255;
      double g = (double) *(p++) / 255;
      double r = (double) *(p++) / 255;
      R2Pixel pixel(r, g, b, 1);
      SetPixel(i, j, pixel);
    }
  }

  // Free unsigned char buffer for reading pixels
  delete [] buffer;

  // Return success
  return 1;
}



int R2Image::
WriteBMP(const char *filename) const
{
  // Open file
  FILE *fp = fopen(filename, "wb");
  if (!fp) {
    fprintf(stderr, "Unable to open image file: %s", filename);
    return 0;
  }

  // Compute number of bytes in row
  int rowsize = 3 * width;
  if ((rowsize % 4) != 0) rowsize = (rowsize / 4 + 1) * 4;

  // Write file header
  BITMAPFILEHEADER bmfh;
  bmfh.bfType = BMP_BF_TYPE;
  bmfh.bfSize = BMP_BF_OFF_BITS + rowsize * height;
  bmfh.bfReserved1 = 0;
  bmfh.bfReserved2 = 0;
  bmfh.bfOffBits = BMP_BF_OFF_BITS;
  WordWriteLE(bmfh.bfType, fp);
  DWordWriteLE(bmfh.bfSize, fp);
  WordWriteLE(bmfh.bfReserved1, fp);
  WordWriteLE(bmfh.bfReserved2, fp);
  DWordWriteLE(bmfh.bfOffBits, fp);

  // Write info header
  BITMAPINFOHEADER bmih;
  bmih.biSize = BMP_BI_SIZE;
  bmih.biWidth = width;
  bmih.biHeight = height;
  bmih.biPlanes = 1;
  bmih.biBitCount = 24;       /* RGB */
  bmih.biCompression = BI_RGB;    /* RGB */
  bmih.biSizeImage = rowsize * (unsigned int) bmih.biHeight;  /* RGB */
  bmih.biXPelsPerMeter = 2925;
  bmih.biYPelsPerMeter = 2925;
  bmih.biClrUsed = 0;
  bmih.biClrImportant = 0;
  DWordWriteLE(bmih.biSize, fp);
  LongWriteLE(bmih.biWidth, fp);
  LongWriteLE(bmih.biHeight, fp);
  WordWriteLE(bmih.biPlanes, fp);
  WordWriteLE(bmih.biBitCount, fp);
  DWordWriteLE(bmih.biCompression, fp);
  DWordWriteLE(bmih.biSizeImage, fp);
  LongWriteLE(bmih.biXPelsPerMeter, fp);
  LongWriteLE(bmih.biYPelsPerMeter, fp);
  DWordWriteLE(bmih.biClrUsed, fp);
  DWordWriteLE(bmih.biClrImportant, fp);

  // Write image, swapping blue and red in each pixel
  int pad = rowsize - width * 3;
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      const R2Pixel& pixel = (*this)[i][j];
      double r = 255.0 * pixel.Red();
      double g = 255.0 * pixel.Green();
      double b = 255.0 * pixel.Blue();
      if (r >= 255) r = 255;
      if (g >= 255) g = 255;
      if (b >= 255) b = 255;
      fputc((unsigned char) b, fp);
      fputc((unsigned char) g, fp);
      fputc((unsigned char) r, fp);
    }

    // Pad row
    for (int i = 0; i < pad; i++) fputc(0, fp);
  }

  // Close file
  fclose(fp);

  // Return success
  return 1;
}


////////////////////////////////////////////////////////////////////////
// PPM I/O
////////////////////////////////////////////////////////////////////////

int R2Image::
ReadPPM(const char *filename)
{
  // Open file
  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    fprintf(stderr, "Unable to open image file: %s", filename);
    return 0;
  }

  // Read PPM file magic identifier
  char buffer[128];
  if (!fgets(buffer, 128, fp)) {
    fprintf(stderr, "Unable to read magic id in PPM file");
    fclose(fp);
    return 0;
  }

  // skip comments
  int c = getc(fp);
  while (c == '#') {
    while (c != '\n') c = getc(fp);
    c = getc(fp);
  }
  ungetc(c, fp);

  // Read width and height
  if (fscanf(fp, "%d%d", &width, &height) != 2) {
    fprintf(stderr, "Unable to read width and height in PPM file");
    fclose(fp);
    return 0;
  }

  // Read max value
  double max_value;
  if (fscanf(fp, "%lf", &max_value) != 1) {
    fprintf(stderr, "Unable to read max_value in PPM file");
    fclose(fp);
    return 0;
  }

  // Allocate image pixels
  pixels = new R2Pixel [ width * height ];
  if (!pixels) {
    fprintf(stderr, "Unable to allocate memory for PPM file");
    fclose(fp);
    return 0;
  }

  // Check if raw or ascii file
  if (!strcmp(buffer, "P6\n")) {
    // Read up to one character of whitespace (\n) after max_value
    int c = getc(fp);
    if (!isspace(c)) putc(c, fp);

    // Read raw image data
    // First ppm pixel is top-left, so read in opposite scan-line order
    for (int j = height-1; j >= 0; j--) {
      for (int i = 0; i < width; i++) {
        double r = (double) getc(fp) / max_value;
        double g = (double) getc(fp) / max_value;
        double b = (double) getc(fp) / max_value;
        R2Pixel pixel(r, g, b, 1);
        SetPixel(i, j, pixel);
      }
    }
  }
  else {
    // Read asci image data
    // First ppm pixel is top-left, so read in opposite scan-line order
    for (int j = height-1; j >= 0; j--) {
      for (int i = 0; i < width; i++) {
	// Read pixel values
	int red, green, blue;
	if (fscanf(fp, "%d%d%d", &red, &green, &blue) != 3) {
	  fprintf(stderr, "Unable to read data at (%d,%d) in PPM file", i, j);
	  fclose(fp);
	  return 0;
	}

	// Assign pixel values
	double r = (double) red / max_value;
	double g = (double) green / max_value;
	double b = (double) blue / max_value;
        R2Pixel pixel(r, g, b, 1);
        SetPixel(i, j, pixel);
      }
    }
  }

  // Close file
  fclose(fp);

  // Return success
  return 1;
}



int R2Image::
WritePPM(const char *filename, int ascii) const
{
  // Check type
  if (ascii) {
    // Open file
    FILE *fp = fopen(filename, "w");
    if (!fp) {
      fprintf(stderr, "Unable to open image file: %s", filename);
      return 0;
    }

    // Print PPM image file
    // First ppm pixel is top-left, so write in opposite scan-line order
    fprintf(fp, "P3\n");
    fprintf(fp, "%d %d\n", width, height);
    fprintf(fp, "255\n");
    for (int j = height-1; j >= 0 ; j--) {
      for (int i = 0; i < width; i++) {
        const R2Pixel& p = (*this)[i][j];
        int r = (int) (255 * p.Red());
        int g = (int) (255 * p.Green());
        int b = (int) (255 * p.Blue());
        fprintf(fp, "%-3d %-3d %-3d  ", r, g, b);
        if (((i+1) % 4) == 0) fprintf(fp, "\n");
      }
      if ((width % 4) != 0) fprintf(fp, "\n");
    }
    fprintf(fp, "\n");

    // Close file
    fclose(fp);
  }
  else {
    // Open file
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
      fprintf(stderr, "Unable to open image file: %s", filename);
      return 0;
    }

    // Print PPM image file
    // First ppm pixel is top-left, so write in opposite scan-line order
    fprintf(fp, "P6\n");
    fprintf(fp, "%d %d\n", width, height);
    fprintf(fp, "255\n");
    for (int j = height-1; j >= 0 ; j--) {
      for (int i = 0; i < width; i++) {
        const R2Pixel& p = (*this)[i][j];
        int r = (int) (255 * p.Red());
        int g = (int) (255 * p.Green());
        int b = (int) (255 * p.Blue());
        fprintf(fp, "%c%c%c", r, g, b);
      }
    }

    // Close file
    fclose(fp);
  }

  // Return success
  return 1;
}


void R2Image::
LoG(void)
{
  // Apply the LoG oprator to the image

  // FILL IN IMPLEMENTATION HERE (REMOVE PRINT STATEMENT WHEN DONE)
  fprintf(stderr, "LoG() not implemented\n");
}

void R2Image::
ChangeSaturation(double factor)
{
  // Changes the saturation of an image
  // Find a formula that changes the saturation without affecting the image brightness

  // FILL IN IMPLEMENTATION HERE (REMOVE PRINT STATEMENT WHEN DONE)
  fprintf(stderr, "ChangeSaturation(%g) not implemented\n", factor);
}



////////////////////////////////////////////////////////////////////////
// JPEG I/O
////////////////////////////////////////////////////////////////////////


// #define USE_JPEG
#ifdef USE_JPEG
  extern "C" {
#   define XMD_H // Otherwise, a conflict with INT32
#   undef FAR // Otherwise, a conflict with windows.h
#   include "jpeg/jpeglib.h"
  };
#endif



int R2Image::
ReadJPEG(const char *filename)
{
#ifdef USE_JPEG
  // Open file
  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    fprintf(stderr, "Unable to open image file: %s", filename);
    return 0;
  }

  // Initialize decompression info
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, fp);
  jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  // Remember image attributes
  width = cinfo.output_width;
  height = cinfo.output_height;
  npixels = width * height;
  int ncomponents = cinfo.output_components;

  // Allocate pixels for image
  pixels = new R2Pixel [ npixels ];
  if (!pixels) {
    fprintf(stderr, "Unable to allocate memory for BMP file");
    fclose(fp);
    return 0;
  }

  // Allocate unsigned char buffer for reading image
  int rowsize = ncomponents * width;
  if ((rowsize % 4) != 0) rowsize = (rowsize / 4 + 1) * 4;
  int nbytes = rowsize * height;
  unsigned char *buffer = new unsigned char [nbytes];
  if (!buffer) {
    fprintf(stderr, "Unable to allocate temporary memory for JPEG file");
    fclose(fp);
    return 0;
  }

  // Read scan lines
  // First jpeg pixel is top-left, so read pixels in opposite scan-line order
  while (cinfo.output_scanline < cinfo.output_height) {
    int scanline = cinfo.output_height - cinfo.output_scanline - 1;
    unsigned char *row_pointer = &buffer[scanline * rowsize];
    jpeg_read_scanlines(&cinfo, &row_pointer, 1);
  }

  // Free everything
  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  // Close file
  fclose(fp);

  // Assign pixels
  for (int j = 0; j < height; j++) {
    unsigned char *p = &buffer[j * rowsize];
    for (int i = 0; i < width; i++) {
      double r, g, b, a;
      if (ncomponents == 1) {
        r = g = b = (double) *(p++) / 255;
        a = 1;
      }
      else if (ncomponents == 1) {
        r = g = b = (double) *(p++) / 255;
        a = 1;
        p++;
      }
      else if (ncomponents == 3) {
        r = (double) *(p++) / 255;
        g = (double) *(p++) / 255;
        b = (double) *(p++) / 255;
        a = 1;
      }
      else if (ncomponents == 4) {
        r = (double) *(p++) / 255;
        g = (double) *(p++) / 255;
        b = (double) *(p++) / 255;
        a = (double) *(p++) / 255;
      }
      else {
        fprintf(stderr, "Unrecognized number of components in jpeg image: %d\n", ncomponents);
        return 0;
      }
      R2Pixel pixel(r, g, b, a);
      SetPixel(i, j, pixel);
    }
  }

  // Free unsigned char buffer for reading pixels
  delete [] buffer;

  // Return success
  return 1;
#else
  fprintf(stderr, "JPEG not supported");
  return 0;
#endif
}




int R2Image::
WriteJPEG(const char *filename) const
{
#ifdef USE_JPEG
  // Open file
  FILE *fp = fopen(filename, "wb");
  if (!fp) {
    fprintf(stderr, "Unable to open image file: %s", filename);
    return 0;
  }

  // Initialize compression info
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  jpeg_stdio_dest(&cinfo, fp);
  cinfo.image_width = width; 	/* image width and height, in pixels */
  cinfo.image_height = height;
  cinfo.input_components = 3;		/* # of color components per pixel */
  cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */
  cinfo.dct_method = JDCT_ISLOW;
  jpeg_set_defaults(&cinfo);
  cinfo.optimize_coding = TRUE;
  jpeg_set_quality(&cinfo, 95, TRUE);
  jpeg_start_compress(&cinfo, TRUE);

  // Allocate unsigned char buffer for reading image
  int rowsize = 3 * width;
  if ((rowsize % 4) != 0) rowsize = (rowsize / 4 + 1) * 4;
  int nbytes = rowsize * height;
  unsigned char *buffer = new unsigned char [nbytes];
  if (!buffer) {
    fprintf(stderr, "Unable to allocate temporary memory for JPEG file");
    fclose(fp);
    return 0;
  }

  // Fill buffer with pixels
  for (int j = 0; j < height; j++) {
    unsigned char *p = &buffer[j * rowsize];
    for (int i = 0; i < width; i++) {
      const R2Pixel& pixel = (*this)[i][j];
      int r = (int) (255 * pixel.Red());
      int g = (int) (255 * pixel.Green());
      int b = (int) (255 * pixel.Blue());
      if (r > 255) r = 255;
      if (g > 255) g = 255;
      if (b > 255) b = 255;
      *(p++) = r;
      *(p++) = g;
      *(p++) = b;
    }
  }



  // Output scan lines
  // First jpeg pixel is top-left, so write in opposite scan-line order
  while (cinfo.next_scanline < cinfo.image_height) {
    int scanline = cinfo.image_height - cinfo.next_scanline - 1;
    unsigned char *row_pointer = &buffer[scanline * rowsize];
    jpeg_write_scanlines(&cinfo, &row_pointer, 1);
  }

  // Free everything
  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);

  // Close file
  fclose(fp);

  // Free unsigned char buffer for reading pixels
  delete [] buffer;

  // Return number of bytes written
  return 1;
#else
  fprintf(stderr, "JPEG not supported");
  return 0;
#endif
}
