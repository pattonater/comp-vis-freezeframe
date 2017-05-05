// Include file for image class
#ifndef R2_IMAGE_INCLUDED
#define R2_IMAGE_INCLUDED



// Constant definitions

typedef enum {
  R2_IMAGE_RED_CHANNEL,
  R2_IMAGE_GREEN_CHANNEL,
  R2_IMAGE_BLUE_CHANNEL,
  R2_IMAGE_ALPHA_CHANNEL,
  R2_IMAGE_NUM_CHANNELS
} R2ImageChannel;

typedef enum {
  R2_IMAGE_POINT_SAMPLING,
  R2_IMAGE_BILINEAR_SAMPLING,
  R2_IMAGE_GAUSSIAN_SAMPLING,
  R2_IMAGE_NUM_SAMPLING_METHODS
} R2ImageSamplingMethod;

typedef enum {
  R2_IMAGE_OVER_COMPOSITION,
  R2_IMAGE_IN_COMPOSITION,
  R2_IMAGE_OUT_COMPOSITION,
  R2_IMAGE_ATOP_COMPOSITION,
  R2_IMAGE_XOR_COMPOSITION,
} R2ImageCompositeOperation;

struct Point {
  double x;
  double y;
  Point(double a, double b) {
    x = a; y = b;
  }
};


struct Feature {
  R2Pixel pixel;
  int x;
  int y;
  double charScale;

  Feature() {}

  Feature(int xi, int yi) {
    charScale = 1.0;
    pixel = R2Pixel(0,0,0,0);
    x = xi;
    y = yi;
  }

  Feature(R2Pixel p, int xi, int yi) {
    charScale = 1.0;
    pixel = p;
    x = xi;
    y = yi;
  }

  Feature(R2Pixel p, int xi, int yi, double scale) { 
    charScale = scale;
    pixel = p;
    x = xi;
    y = yi;
  }

   bool operator<(const Feature& feature) const {
     R2Pixel a = pixel;
     R2Pixel b = feature.pixel; 
     return (a.Red() + a.Blue() + a.Green()) < (b.Red() + b.Blue() + b.Green());
  }

  bool closeTo(const Feature& feature, double radius) const {
    bool close = (x - feature.x) * (x - feature.x) + (y - feature.y) * (y - feature.y) < radius * radius;

    return close;
  }
};


struct FeatureMatch {
  Feature a;
  Feature b;
  float ssd;

  bool verifiedMatch;

  FeatureMatch(Feature a1, float ssd2) {
    a = a1;
    ssd = ssd2;
  }

  void flipDirection() {
    Feature tmp = a;
    a = b;
    b = tmp;
  }

 bool operator<(const FeatureMatch& match) const {
     return (match.ssd < ssd);
  }
};


// Class definition

#include <vector>

class R2Image {
 public:
  // Constructors/destructor
  R2Image(void);
  R2Image(const char *filename);
  R2Image(int width, int height);
  R2Image(int width, int height, const R2Pixel *pixels);
  R2Image(const R2Image& image);
  ~R2Image(void);

  // Image properties
  int NPixels(void) const;
  int Width(void) const;
  int Height(void) const;

  // Pixel access/update
  R2Pixel& Pixel(int x, int y);
  R2Pixel *Pixels(void);
  R2Pixel *Pixels(int row);
  R2Pixel *operator[](int row);
  const R2Pixel *operator[](int row) const;
  void SetPixel(int x, int y,  const R2Pixel& pixel);

  // Image processing
  R2Image& operator=(const R2Image& image);

  // Per-pixel operations
  void Brighten(double factor);
  void ChangeSaturation(double factor);

  // show how SVD works
  void svdTest();

  // Linear filtering operations
  void applyFilter3x3(int sobel[3][3]);
  void SobelX();
  void SobelY();
  void LoG();
  void Blur(double sigma, bool clamped);
  void Harris(double sigma, bool clamped);
  void FeatureDetector(double numFeatures);
  void TrackFeatures(int numFeatures, R2Image& originalImage);
  void MatchImage(R2Image& originalImage);
  void Sharpen(void);
  void HighPass(double sigma, double contrast);

  // Feature helpers
  bool shouldInsertFeature(const std::vector<Feature>& features, const Feature& feature, const int minDistance) const;
  void findScaleInvariantHarrisFeaturePoints(std::vector<Feature>& features, R2Image& image);
  void calculateCharacteristicScales(std::vector<Feature>& features, R2Image& image);
  void findFeatures(double numFeatures, double minDistance, bool scaleInvariant, R2Image& image, std::vector<Feature>& selectedFeatures);
  FeatureMatch findFeatureMatch(const Feature& feature, R2Image& featureImage, const Point searchOrigin, const float searchAreaPercentage, const int ssdSearchRadius);
  FeatureMatch findFeatureMatchConsecutiveImages(const Feature& feature, R2Image& featureImage, const float searchAreaPercentage, const int ssdSearchRadius);
  float calculateSSD(const int x0, const int y0, const int x1, const int y1, R2Image& originalImage, const int ssdCompareReach);
  void classifyMatchesWithRANSAC(std::vector<FeatureMatch>& matches) const;
  bool similarMotion(const FeatureMatch& a, const FeatureMatch& b) const;
  void testDLT();
  void findMatches(const int numFeatures, const int numMatches, R2Image& originalImage, std::vector<FeatureMatch>& matches);
  void drawMatches(const std::vector<FeatureMatch> matches);

  // helpers
  bool inBounds(const int x, const int y) const;
  bool inBounds(const Point p) const;
  bool contains(const int number, const std::vector<int>& vector) const;

  // transform
  void classifyMatchesWithDltRANSAC(std::vector<FeatureMatch>& matches) const;
  void computeHomographyMatrixWithDLT(const std::vector<FeatureMatch>& matches, std::vector<double>& homographyMatrix) const;
  bool checkHomographySimilarity(const FeatureMatch& match, const std::vector<double>& homographyMatrix) const;
  Point transformPoint(const int x0, const int y0, const std::vector<double>& homographyMatrix) const;
  void transformImage(const std::vector<double>& homographyMatrix);
  void blendWithImage(R2Image& otherImage);
  void flipMatchDirections(std::vector<FeatureMatch>& matches) const;

  // Drawing
  void drawSquare(const int x, const int y, const int reach, const float r, const float g, const float b);
  void drawLine(int x0, int y0, int x1, int y1, const float r, const float g, const float b);
  void drawCircle(const int xCenter, const int yCenter, const int radius, const float r, const float g, const float b);

  // Freeze Frame
  void identifyCorners(const std::vector<R2Image>& corners, std::vector<Point>& cornerCoords);
  Point findImageMatch(const Point& searchOrigin, const float searchWindowPercentage, R2Image& comparisonImage);

  // further operations
  void blendOtherImageTranslated(R2Image * otherImage);
  void blendOtherImageHomography(R2Image * otherImage);

  // File reading/writing
  int Read(const char *filename);
  int ReadBMP(const char *filename);
  int ReadPPM(const char *filename);
  int ReadJPEG(const char *filename);
  int Write(const char *filename) const;
  int WriteBMP(const char *filename) const;
  int WritePPM(const char *filename, int ascii = 0) const;
  int WriteJPEG(const char *filename) const;

 private:
  // Utility functions
  void Resize(int width, int height);
  R2Pixel Sample(double u, double v,  int sampling_method);

 private:
  R2Pixel *pixels;
  int npixels;
  int width;
  int height;
};



// Inline functions

inline int R2Image::
NPixels(void) const
{
  // Return total number of pixels
  return npixels;
}



inline int R2Image::
Width(void) const
{
  // Return width
  return width;
}



inline int R2Image::
Height(void) const
{
  // Return height
  return height;
}



inline R2Pixel& R2Image::
Pixel(int x, int y)
{
  // Return pixel value at (x,y)
  // (pixels start at lower-left and go in row-major order)
  return pixels[x*height + y];
}



inline R2Pixel *R2Image::
Pixels(void)
{
  // Return pointer to pixels for whole image
  // (pixels start at lower-left and go in row-major order)
  return pixels;
}



inline R2Pixel *R2Image::
Pixels(int x)
{
  // Return pixels pointer for row at x
  // (pixels start at lower-left and go in row-major order)
  return &pixels[x*height];
}



inline R2Pixel *R2Image::
operator[](int x)
{
  // Return pixels pointer for row at x
  return Pixels(x);
}



inline const R2Pixel *R2Image::
operator[](int x) const
{
  // Return pixels pointer for row at x
  // (pixels start at lower-left and go in row-major order)
  return &pixels[x*height];
}



inline void R2Image::
SetPixel(int x, int y, const R2Pixel& pixel)
{
  // Set pixel
  pixels[x*height + y] = pixel;
}



#endif
