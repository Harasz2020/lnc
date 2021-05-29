#include "MosaicMaker.h"

#include <filesystem>
#include <algorithm>
#include <limits>

#include <bmp.h>
#include <ColorConversion.h>

MosaicMaker::MosaicMaker(const std::string& smallDir,
                         const std::string& largeImageFilename,
                         const Vec2ui& smallImageResolution,
                         const Vec2ui& largeImageBlockSize) :
smallDir{smallDir},
largeImageFilename{largeImageFilename},
smallImageResolution{smallImageResolution},
largeImageBlockSize{largeImageBlockSize}
{
}

void MosaicMaker::updateSmallImageCache() {
  for (auto& p: std::filesystem::directory_iterator(smallDir)) {
    try {
      if (p.path().extension() != ".bmp") continue;        
      SmallImageInfo info{ p.path().string()};
      smallImageInfos.push_back(info);
    } catch (...) {
    }
  }
  if (smallImageInfos.empty())
    throw MosaicMakerException("Unable to load small images");
}

void MosaicMaker::loadLargeImage() {
  try {
    largeImage = BMP::load(largeImageFilename);
  } catch (...) {
    throw MosaicMakerException("Unable to load large image");
  }
}

Vec3t<double> MosaicMaker::computeFeatureVec(const uint32_t xBlock, const uint32_t yBlock) const {
  const uint32_t xStart = xBlock*largeImageBlockSize.x;
  const uint32_t yStart = yBlock*largeImageBlockSize.y;
  
  Vec3t<double> featureVec = Vec3t<double>{0,0,0};
  for (uint32_t y = yStart;y<yStart+largeImageBlockSize.y;++y) {
    for (uint32_t x = xStart;x<xStart+largeImageBlockSize.x;++x) {
      const Vec3 rgb{largeImage.getValue(x,y,0)/255.0f,
                     largeImage.getValue(x,y,1)/255.0f,
                     largeImage.getValue(x,y,2)/255.0f};
      featureVec = featureVec + Vec3t<double>(rgb);
    }
  }
  return featureVec / double(largeImageBlockSize.x * largeImageBlockSize.y);
}

const SmallImageInfo& MosaicMaker::findBestSmallImage(const Vec3t<double>& largeImageFeatureVec,
                                                      const std::vector<SmallImageInfo>& recentBricks) const {
  size_t minElementIndex = 0;
  double minDist = std::numeric_limits<double>::max();
  for (size_t i=0;i<smallImageInfos.size();++i) {
    if (std::find(recentBricks.begin(), recentBricks.end(), smallImageInfos[i]) != recentBricks.end())
      continue;
    
    double currentDist = (smallImageInfos[i].featureVec-
                          largeImageFeatureVec).length();
    if (currentDist < minDist) {
      minDist = currentDist;
      minElementIndex = i;
    }
  }
  return smallImageInfos[minElementIndex];
}

void MosaicMaker::placeSmallImageIntoResult(const uint32_t xBlock, const uint32_t yBlock,
                                            const SmallImageInfo& imageInfo) {
  const uint32_t xStart = xBlock*smallImageResolution.x;
  const uint32_t yStart = yBlock*smallImageResolution.y;
  
  // TODO: remove the assumption that all image are squared
  const Image smallImage = BMP::load(imageInfo.filename).resample(smallImageResolution.x);
  
  for (uint32_t y = yStart;y<yStart+smallImageResolution.y;++y) {
    for (uint32_t x = xStart;x<xStart+smallImageResolution.x;++x) {
      resultImage.setValue(x,y,0, smallImage.getValue(x-xStart,y-yStart,0));
      resultImage.setValue(x,y,1, smallImage.getValue(x-xStart,y-yStart,1));
      resultImage.setValue(x,y,2, smallImage.getValue(x-xStart,y-yStart,2));
      resultImage.setValue(x,y,3,255);
    }
  }
  
  usedImages.push_back(imageInfo);
}

const std::vector<SmallImageInfo> MosaicMaker::gatherRecentBricks(uint32_t x, uint32_t y,
                                                                  uint32_t dist) {
  
  const uint32_t xBricks = largeImage.width/largeImageBlockSize.x;
  std::vector<SmallImageInfo> result;
  const int64_t currentIndex = int64_t(x + y * xBricks);
  
  for (int64_t dy = -int64_t(dist);dy<=0;++dy) {
    for (int64_t dx = -int64_t(dist);dx<int64_t(dist);++dx) {
      const int64_t testIndex = (int64_t(x)+dx) + (int64_t(y)+dy) * int64_t(xBricks);
      if (int64_t(x)+dx < 0 || (int64_t(y)+dy) * int64_t(xBricks) < 0 || testIndex >= currentIndex)
        continue;
      result.push_back(usedImages[uint32_t(testIndex)]);
    }
  }
  
  return result;
}

void MosaicMaker::generateResultImage() {
  const uint32_t largeWidth  = (largeImage.width / largeImageBlockSize.x) *
                                smallImageResolution.x;
  const uint32_t largeHeight = (largeImage.height / largeImageBlockSize.y) *
                                smallImageResolution.y;
  resultImage = Image{largeWidth, largeHeight, 4};
  
  const uint32_t yBricks = largeImage.height/largeImageBlockSize.y;
  const uint32_t xBricks = largeImage.width/largeImageBlockSize.x;
  
  for (uint32_t y = 0;y<yBricks;++y) {
    for (uint32_t x = 0;x<xBricks;++x) {
      const Vec3t<double> featureVec = computeFeatureVec(x,y);
      const std::vector<SmallImageInfo> recentBricks = gatherRecentBricks(x,y,5);
      const SmallImageInfo& imageInfo = findBestSmallImage(featureVec, recentBricks);
      placeSmallImageIntoResult(x,y,imageInfo);
    }
  }
}

void MosaicMaker::generate() {
  updateSmallImageCache();
  loadLargeImage();
  generateResultImage();
}

Image MosaicMaker::getResultImage() const {
  return resultImage;
}

Image MosaicMaker::getResultImage(const uint32_t maxWidth) const {
  if (maxWidth < resultImage.width)
    return resultImage.resample(maxWidth);
  else
    return resultImage;
}

SmallImageInfo::SmallImageInfo(const std::string& filename) :
  filename{filename}
{
  computeFeatureVector();
}

void SmallImageInfo::computeFeatureVector() {
  Image image = BMP::load(filename);  
  hash = MD5::computeMD5(image.data);
  
  if (image.componentCount < 3) throw BMP::BMPException("Too few image componets.");
  featureVec = Vec3t<double>{0,0,0};
  for (uint32_t y = 0;y<image.height;++y) {
    for (uint32_t x = 0;x<image.width;++x) {
      const Vec3 rgb{image.getValue(x,y,0)/255.0f,
                     image.getValue(x,y,1)/255.0f,
                     image.getValue(x,y,2)/255.0f};
      featureVec = featureVec + Vec3t<double>(rgb);
    }
  }
  featureVec = featureVec / double(image.height * image.width);
}