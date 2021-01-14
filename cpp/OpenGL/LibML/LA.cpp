#include <iostream>
#include <sstream>
#include <random>
#include <algorithm>
#include <cassert>

#include "LA.h"

float sigmoid(float x) {
  return 1.0f/(1+exp(-x));
}

float sigmoidPrime(float x) {
  return sigmoid(x)*(1.0f-sigmoid(x));
}

float reLU(float x) {
  return std::max(0.0f,x);
}

float reLUPrime(float x) {
  return (x>0.0f) ? 1.0f : 0.0f;
}

float lreLU(float x) {
  return std::max(0.1f*x,x);
}

float lreLUPrime(float x) {
  return (x>0.0f) ? 1.0f : 0.1f;
}

float tanhPrime(float x) {
  const float ch = cosh(x);
  return 1.0f/(ch*ch);
}

Vec Vec::uniform(size_t size, float from, float to) {
  Vec v{size};
  std::random_device rd{};
  std::mt19937 gen{rd()};
  std::uniform_real_distribution<float> dist{from, to};
  for (size_t i = 0;i<v.e.size();++i) {
    v.e[i] = dist(gen);
  }
  return v;
}

Vec Vec::gaussian(size_t size, float mean, float stddev) {
  Vec v{size};
  std::random_device rd{};
  std::mt19937 gen{rd()};
  std::normal_distribution<float> dist{mean, stddev};
  for (size_t i = 0;i<v.e.size();++i) {
    v.e[i] = dist(gen);
  }
  return v;
}

Vec::Vec(size_t size) :
  e(size)
{
}

Vec::Vec(const std::vector<float>& e) :
  e(e)
{}

Vec::Vec(const std::vector<uint8_t>& e) :
  e(e.size())
{
  for (size_t i = 0;i<e.size();++i) {
    this->e[i] = e[i]/255.0f;
  }
}

Vec& Vec::operator+=(const Vec& rhs) {
  assert(rhs.size() == e.size());

  for (size_t i = 0;i<e.size();++i) {
    e[i] += rhs.e[i];
  }
  return *this;
}

Vec& Vec::operator-=(const Vec& rhs) {
  assert(rhs.size() == e.size());

  for (size_t i = 0;i<e.size();++i) {
    e[i] -= rhs.e[i];
  }
  return *this;
}

Vec Vec::operator+(const Vec& other) const {
  assert(size() == other.size());
  
  Vec result(e);
  for (size_t i = 0;i<e.size();++i) {
    result.e[i] += other.e[i];
  }
  return result;
}

Vec Vec::operator-(const Vec& other) const {
  assert(size() == other.size());

  Vec result(e);
  for (size_t i = 0;i<e.size();++i) {
    result.e[i] -= other.e[i];
  }
  return result;
}

Vec Vec::operator*(const Vec& other) const {
  assert(size() == other.size());

  Vec result(e);
  for (size_t i = 0;i<e.size();++i) {
    result.e[i] *= other.e[i];
  }
  return result;
}

Vec Vec::operator/(const Vec& other) const {
  assert(size() == other.size());

  Vec result(e);
  for (size_t i = 0;i<e.size();++i) {
    result.e[i] /= other.e[i];
  }
  return result;
}

float Vec::dot(const Vec& a, const Vec& b) {
  assert(a.size() == b.size());

  float result=0;
  for (size_t i = 0;i<a.e.size();++i) {
    result += a.e[i] * b.e[i];
  }
  return result;
}

bool Vec::operator == ( const Vec& other ) const {
  for (size_t i = 0;i<e.size();++i) {
    if (e[i] != other.e[i]) return false;
  }
  return true;
}

bool Vec::operator != ( const Vec& other ) const {
  return ! (*this == other);
}

float Vec::sum() const {
  float s = 0.0f;
  for (size_t i = 0;i<e.size();++i) {
    s += e[i];
  }
  return s;
}

const std::string Vec::toString() const {
  std::stringstream result;
  result << "[";
  for (size_t i = 0;i<e.size();++i) {
    if (i + 1 < e.size())
      result << e[i] << ", ";
    else
      result << e[i];
  }
  result << "]";
  return result.str();
}

Vec Vec::apply(float func(float x)) const {
  Vec result(e.size());
  for (size_t i = 0;i<e.size();++i) {
    result.e[i] = func(e[i]);
  }
  return result;
}

bool Vec::hasNaN() const {
  for (size_t i = 0;i<e.size();++i) {
    if (e[i] != e[i]) return true;
  }
  return false;
}

Vec Vec::operator*(float a) const {
  Vec result(e);
  for (size_t i = 0;i<e.size();++i) {
    result.e[i] *= a;
  }
  return result;
}

std::istream& operator>> (std::istream &is, Vec &vec) {
  std::string data;
  char c;
  while (is.get(c)) {
    if (c == ']') break;
    if (c != '[') {
      if (c != ',') data += c;
    } else {
      data = "";
    }
  }
  vec.e.clear();
  std::stringstream ss(data);
  float elem;
  while (ss >> elem) vec.e.push_back(elem);
  
  return is;
}

Vec Vec::sort(const Vec& v) {
  Vec result(v);
  std::sort(result.e.begin(), result.e.end());
  return result;
}

Vec Vec::softmax() const {
  Vec output{e};

  float m = std::numeric_limits<float>::lowest();
  for (size_t i = 0; i < e.size(); ++i) {
    if (m < e[i]) {
      m = e[i];
    }
  }

  float sum = 0.0f;
  for (size_t i = 0; i < e.size(); ++i) {
    sum += expf(e[i] - m);
  }

  const float constant = m + logf(sum);
  for (size_t i = 0; i < e.size(); ++i) {
    output[i] = expf(e[i] - constant);
  }
 
  return output;
}

Mat Vec::softmaxPrime() const {
  const Vec p = softmax();
  Mat t{p.size(),p.size()};
  for (size_t y = 0;y<p.size();++y) {
    for (size_t x = 0;x<p.size();++x) {
      t[x+y*p.size()] = (x==y) ? (p[x] * (1.0f-p[x])) : (-p[y]*p[x]);
    }
  }
  return t;
}

Mat::Mat(size_t sizeX, size_t sizeY) :
  e(sizeX*sizeY),
  sizeX(sizeX)
{
}

Mat::Mat(size_t sizeX, const std::vector<float>& e) : 
  e(e),
  sizeX(sizeX)
{
}

Mat::Mat(const Mat& other) :
  e(other.e),
  sizeX(other.sizeX)
{
}

Mat Mat::uniform(size_t sizeX, size_t sizeY, float from, float to) {
  Mat m{sizeX, sizeY};
  std::random_device rd{};
  std::mt19937 gen{rd()};
  std::uniform_real_distribution<float> dist{from, to};
  for (size_t i = 0;i<m.e.size();++i) {
    m.e[i] = dist(gen);
  }
  return m;
}

Mat Mat::gaussian(size_t sizeX, size_t sizeY, float mean, float stddev) {
  Mat m{sizeX, sizeY};
  std::random_device rd{};
  std::mt19937 gen{rd()};
  std::normal_distribution<float> dist{mean, stddev};
  for (size_t i = 0;i<m.e.size();++i) {
    m.e[i] = dist(gen);
  }
  return m;
}

Mat Mat::tensorProduct(const Vec& v, const Vec& w) {
  Mat m{v.size(), w.size()};
  for (size_t x = 0;x<v.size();++x) {
    for (size_t y = 0;y<w.size();++y) {
      m.e[x+v.size()*y] = v[x]*w[y];
    }
  }
  return m;
}

Mat& Mat::operator+=(const Mat& rhs) {
  assert(rhs.e.size() == e.size());

  for (size_t i = 0;i<e.size();++i) {
    e[i] += rhs.e[i];
  }
  return *this;
}

Mat& Mat::operator-=(const Mat& rhs) {
  assert(rhs.e.size() == e.size());

  for (size_t i = 0;i<e.size();++i) {
    e[i] -= rhs.e[i];
  }
  return *this;
}

Mat Mat::operator+(const Mat& other) const {
  assert(other.e.size() == e.size() && other.sizeX == sizeX);
  
  Mat result(sizeX, e);
  for (size_t i = 0;i<e.size();++i) {
    result.e[i] += other.e[i];
  }
  return result;

}

Mat Mat::operator-(const Mat& other) const {
  assert(other.e.size() == e.size() && other.sizeX == sizeX);

  Mat result(sizeX, e);
  for (size_t i = 0;i<e.size();++i) {
    result.e[i] -= other.e[i];
  }
  return result;

}

Vec Mat::operator*(const Vec& v) const {
  assert(sizeX == v.size());
  
  Vec result(getHeight());
  for (size_t i = 0;i<getHeight();++i) {
    result[i] = Vec::dot(v,row(i));
  }
  return result;
}

Mat Mat::operator*(float a) const {
  Mat result(sizeX,e);
  for (size_t i = 0;i<e.size();++i) {
    result.e[i] *= a;
  }
  return result;
}


Vec Mat::row(size_t i) const {
  Vec result(sizeX);
  const size_t start = i*sizeX;
  for (size_t i = 0;i<sizeX;++i)
    result[i] = e[i+start];
  return result;
}

const std::string Mat::toString() const {
  std::stringstream result;

  result << "[";
  for (size_t i = 0;i<e.size()/sizeX;++i) {
    
    for (size_t j = 0;j<sizeX;++j) {
      if (j + 1 < sizeX)
        result << e[i*sizeX+j] << ", ";
      else
        result << e[i*sizeX+j];
    }
    if (i + 1 < e.size()/sizeX)
      result << "\n";
    else
      result << "]";
  }
  
  return result.str();
}

std::istream& operator>> (std::istream &is, Mat &mat) {
  std::string data;
  size_t sizeX = 1;
  char c;
  while (is.get(c)) {
    if (c == ']') break;
    if (c == '\n') sizeX=1;

    if (c != '[') {
      if (c != ',')
        data += c;
      else
        sizeX++;
    } else {
      data = "";
    }
  }
  
  mat.e.clear();
  mat.sizeX = sizeX;
  std::stringstream ss(data);
  float elem;
  while (ss >> elem) mat.e.push_back(elem);
  return is;
}

Mat Mat::transpose() const {
  Mat m{getHeight(), getWidth()};

  for (size_t x = 0;x<getWidth();++x) {
    for (size_t y = 0;y<getHeight();++y) {
      m.e[y+getHeight()*x] = e[x+getWidth()*y];
    }
  }

  return m;
}

bool Mat::operator == ( const Mat& other ) const {
  for (size_t i = 0;i<e.size();++i) {
    if (e[i] != other.e[i]) return false;
  }
  return true;
}

bool Mat::operator != ( const Mat& other ) const {
  return ! (*this == other);
}

bool Mat::hasNaN() const {
  for (size_t i = 0;i<e.size();++i) {
    if (e[i] != e[i]) return true;
  }
  return false;
}
