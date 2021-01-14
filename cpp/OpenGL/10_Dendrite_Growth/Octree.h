#pragma once

#include <vector>
#include <array>
#include <memory>

#include <Vec3.h>
#include <Vec4.h>


class OctreeNode;

typedef std::shared_ptr<OctreeNode> OctreeNodePtr;

class OctreeNode {
public:
    OctreeNode(const Vec3& minPos, const Vec3& maxPos);
    OctreeNode(const Vec3& first, const Vec3& minPos, const Vec3& maxPos);
    void add(const Vec3& pos, size_t maxElemCount, size_t maxDepth);

    std::vector<float> toTriList();
    std::vector<float> toLineList() const;

    float minSqDistApprox(const Vec3& pos) const;
    float minSqDist(const Vec3& pos, float radiusSq) const;
    bool isLeaf() const {return children[0] == nullptr;}
    
    void age() {if (warranty>0) warranty--;}

private:
    size_t warranty;
    Vec3 minPos;
    Vec3 maxPos;
    std::vector<Vec3> elements;
    std::array<OctreeNodePtr, 8> children;

    void split(size_t maxElemCount, size_t maxDepth);
    size_t subtreeIndex(const Vec3& pos) const;
    Vec3 computeCenter() const;
    bool intersect(const Vec3& pos, float radiusSq, Vec3 minPos, Vec3 maxPos) const;
    static float sq(float x) {return x*x;}
    static void pushColor(std::vector<float>& v, const Vec4& c);

};

class Octree {
public:
    Octree(float size, const Vec3& first, size_t maxElemCount=10, size_t maxDepth=20);
    
    void add(const Vec3& pos);
    float minDist(const Vec3& pos) const;
    
    std::vector<float> toTriList();
    std::vector<float> toLineList() const;
        
private:
    size_t maxElemCount;
    size_t maxDepth;
    OctreeNodePtr root;
};
