#include <fstream>
#include "Image.h"

void Image::setData(unsigned int x, unsigned int y, char value) {
  target[y*w+x] = value;
}

char Image::getData(unsigned int x, unsigned int y) const {
  return target[y*w+x];
}

std::string Image::getExt(const std::string& filename) const {
  size_t i = filename.find_last_of('.');
  
  if (i != std::string::npos) {
    return filename.substr(i+1);
  } else {
    return "";
  }
}

std::string Image::toString() const {
  std::string s = "";
  for (int y = 0;y<h;++y) {
    for (int x = 0;x<w;++x) {
      char c = getData(x,y);
      s += (c < 50) ? "." : "X";
    }
    s += "\n";
  }
  return s;
}

bool Image::save(const std::string& filename, bool ignoreSize) {
  const uint8_t iComponentCount = 3;

  std::ofstream outStream(filename.c_str(), std::ofstream::binary);
  if (!outStream.is_open()) return false;
  
  // write BMP-Header
  outStream.write((char*)"BM", 2); // all BMP-Files start with "BM"
  uint32_t header[3];
  int64_t rowPad= 4-((w*8*iComponentCount)%32)/8;
  if (rowPad == 4) rowPad = 0;
  
  // filesize = 54 (header) + sizeX * sizeY * numChannels
  size_t filesize = 54+size_t(w)*size_t(h)*size_t(iComponentCount)+size_t(rowPad)*size_t(h);
  
  if (!ignoreSize && uint32_t(filesize) != filesize)
    throw std::runtime_error("File to big for BMP format");
  
  header[0] = uint32_t(filesize);
  header[1] = 0;                  // reserved = 0 (4 Bytes)
  
  
  header[2] = 54;                  // File offset to Raster Data
  outStream.write((char*)header, 4*3);
  // write BMP-Info-Header
  uint32_t infoHeader[10];
  infoHeader[0] = 40;            // size of info header
  infoHeader[1] = w;            // Bitmap Width
  infoHeader[2] = h;//uint32_t(-(int32_t)h);           // Bitmap Height (negative to flip image)
  infoHeader[3] = 1+65536*8*iComponentCount;
  // first 2 bytes=Number of Planes (=1)
  // next  2 bytes=BPP
  infoHeader[4] = 0;            // compression (0 = none)
  infoHeader[5] = 0;            // compressed file size (0 if no compression)
  infoHeader[6] = 11810;        // horizontal resolution: Pixels/meter (11810 = 300 dpi)
  infoHeader[7] = 11810;        // vertical resolution: Pixels/meter (11810 = 300 dpi)
  infoHeader[8] = 0;            // Number of actually used colors
  infoHeader[9] = 0;            // Number of important colors  0 = all
  outStream.write((char*)infoHeader, 4*10);
  
  // data in BMP is stored BGR, so convert scalar BGR
  const size_t totalSize = size_t(iComponentCount)*size_t(w)*size_t(h);
  std::vector<uint8_t> pData(totalSize);

  uint32_t i = 0;
  for (int y = 0;y<h;++y) {
    for (int x = 0;x<w;++x) {
      
      uint8_t r = getData(x,y);
      uint8_t g = (uint8_t)(r*3);
      uint8_t b = (uint8_t)(r*25);
      uint8_t a = 255;
      
      pData[i++] = b;
      pData[i++] = g;
      pData[i++] = r;
      if (iComponentCount==4) pData[i++] = a;
    }
  }
  
  // write data (pad if necessary)
  if (rowPad==0) {
      outStream.write((char*)pData.data(), totalSize);
  }
  else {
    uint8_t zeroes[9]={0,0,0,0,0,0,0,0,0};
    for (size_t i=0; i<h; i++) {
      outStream.write((char*)&(pData[iComponentCount*i*w]), size_t(iComponentCount)*size_t(w));
      outStream.write((char*)zeroes, rowPad);
    }
  }
  
  outStream.close();
  return true;
}
