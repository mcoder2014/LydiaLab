#pragma once

#include <cmath>
#include <set>
#include <stack>
#include <vector>
#include <list>
#include <stack>

#include <QSet>
#include <QtOpenGL>

#include "surface_mesh/Surface_mesh.h"
#include "SurfaceMeshModel.h"
#include "primitives.h"

typedef std::set<int> IndexSet;
typedef IndexSet::iterator IndexSetIter;

// Ray-triangle acceleration
#include "TriAccel.h"

#define USE_TRI_ACCEL 1
#define DEFAULT_OCTREE_NODE_SIZE 64

class Octree
{
public:
    // 包围盒-立方体-一定比例扩大
    BoundingBox boundingBox;
    BoundingBox originBoundingBox;
    // 子节点
    std::vector<Octree> children;
    // 面片
    std::vector<Surface_mesh::Face> triangleData;
    // 父节点
    Octree * parent;
    // Debug 使用的信息
    std::vector<Octree *> selectedChildren;


private:

    int trianglePerNode;
    Surface_mesh * mesh;
    Surface_mesh::Vertex_property<Eigen::Vector3d> points;

public:

    /**
     * @brief Octree
     * 空构造函数
     */
    Octree()
    {
        trianglePerNode = -1;
        parent = NULL;
        mesh = NULL;
    }

    /**
     * @brief Octree 最经常使用的构造函数
     * @param useMesh
     * @param triPerNode
     */
    Octree( Surface_mesh * useMesh, int triPerNode = DEFAULT_OCTREE_NODE_SIZE )
    {
        this->parent = NULL;
        this->mesh = useMesh;
        this->trianglePerNode = triPerNode;

        // Using all tris
        std::vector<Surface_mesh::Face> allTris;
        Surface_mesh::Face_iterator fit, fend = useMesh->faces_end();
        for(fit = useMesh->faces_begin(); fit != fend; ++fit)
            allTris.push_back(fit);

        this->initBuild(allTris, trianglePerNode);
    }

    Octree( int triPerNode, const BoundingBox& bb, const std::vector<Surface_mesh::Face>& tris, Surface_mesh * useMesh )
    {
        this->parent = NULL;
        this->mesh = useMesh;
        this->boundingBox = bb;
        this->trianglePerNode = triPerNode;
        this->triangleData = tris;
    }
    
    /**
     * @brief closestIntersectionPoint
     * @param ray Uses eigen ray type
     * @param faceIndex
     * @return
     */
    Eigen::Vector3d closestIntersectionPoint( const Eigen::ParametrizedLine<double,3>& ray, int* faceIndex ){
        Ray _ray(ray.origin(), ray.direction());
        return closestIntersectionPoint(_ray, faceIndex);
    }
    
    /**
     * @brief closestIntersectionPoint 射线相交的最近点
     * @param ray
     * @param faceIndex
     * @param isRobsut
     * @return
     */
    Eigen::Vector3d closestIntersectionPoint( const Ray & ray, int * faceIndex, bool isRobsut = false )
    {
        HitResult res, best_res;
        Eigen::Vector3d isetpoint(0.0,0.0,0.0);
        double minDistance = DBL_MAX;
        if(faceIndex) *faceIndex = -1;

        // We use two tests, first is much faster but has misses.
        // The second is executed only if first fails

        // Fast, not robust tests
        foreach( int i, intersectRay( ray, 0, false ) )
        {
            intersectionTestAccelerated(SurfaceMesh::Model::Face(i), ray, res);

            // find the nearest intersection point
            if(res.hit)
            {
                if (res.distance < minDistance)
                {
                    minDistance = res.distance;
                    isetpoint = ray.origin + (ray.direction * res.distance);
                    if(faceIndex) *faceIndex = i;
                    best_res = res;
                }
            }
        }

        if( isRobsut )
        {
            // Slower, more robust tests
            if(!best_res.hit){
                foreach( int i, intersectRay( ray, 0.01, false ) ){
                    rayTriangleIntersectionTest(SurfaceMesh::Model::Face(i), ray, res, false);
                    if(res.hit){
                        if (res.distance < minDistance){
                            minDistance = res.distance;
                            isetpoint = ray.origin + (ray.direction * res.distance);
                            if(faceIndex) *faceIndex = i;
                            best_res = res;
                        }
                    }
                }
            }
        }

        //assert(best_res.hit);

        return isetpoint;
    }

    /**
     * @brief isIntersectsWithRay
     * 是否与射线相交
     * @param ray
     * @param distance
     * @return
     */
    bool isIntersectsWithRay(const Ray & ray, double * distance)
    {
        int faceIndex = -1;
        if(distance) *distance = DBL_MAX;

        Eigen::Vector3d p = closestIntersectionPoint(ray, &faceIndex);

        if(faceIndex >= 0){
            if(distance) *distance = (p-ray.origin).norm();
            return true;
        }

        return false;
    }

    /**
     * @brief intersectSphere
     * 是否与球体相交
     * @param sphere_center
     * @param radius
     * @return
     */
    IndexSet intersectSphere( const Eigen::Vector3d& sphere_center, double radius )
    {
        IndexSet tris;

        if (boundingBox.intersectsSphere(sphere_center, radius))
            intersectRecursiveSphere(sphere_center, radius, tris);

        return tris;
    }

    /**
     * @brief getMesh
     * 获得 八叉树中的 mesh
     * @return
     */
    Surface_mesh *getMesh() const
    {
        return mesh;
    }

    /**
     * @brief intersectPoint
     * 获得顶点所在的包围盒的所有面片
     * @param point
     * @return
     */
    IndexSet intersectPoint( const Eigen::Vector3d& point )
    {
        IndexSet tris;

        if (boundingBox.contains(point))
            intersectRecursivePoint(point, tris);

        return tris;
    }

    /**
     * @brief intersectRecursivePoint
     * 迭代获得顶点所在包围盒的所有面片
     * @param point
     * @param tris
     */
    void intersectRecursivePoint( const Eigen::Vector3d& point, IndexSet& tris )
    {
        if (intersectHit(tris))
            return;

        for (std::vector<Octree>::iterator child = children.begin();  child != children.end(); child++)
        {
            if (child->boundingBox.contains(point))
                child->intersectRecursivePoint(point, tris);
        }
    }


    /**
     * @brief intersectHit
     * 如果是叶子节点，将叶子节点的所有面片加入集合
     * @param triangleSet
     * // Leaf node = true
     * @return
     */
    bool intersectHit( IndexSet& triangleSet )
    {
        if( this->children.size() > 0 )
            return false;

        for(std::vector<Surface_mesh::Face>::iterator it = triangleData.begin(); it != triangleData.end(); it++)
        {
            Surface_mesh::Face face = *it;
            triangleSet.insert( face.idx() );
        }

        // Debug:
        //root()->selectedChildren.push_back(this);

        // This is a leaf node
        return true;
    }

    /**
     * @brief intersectRay
     * 找到射线相交到的所有面片
     * @param ray
     * @param rayThickness
     * @param isFullTest
     * @return
     */
    QSet<int> intersectRay( Ray ray, double rayThickness, bool isFullTest) const
    {
        QSet<int> triangleSet;

        ray.thickness = rayThickness;

        //DEBUG:
        //this->selectedChildren.clear();

        if ( this->boundingBox.intersects(ray) )
        {
            std::stack<const Octree*> octreeStack;
            octreeStack.push( this );

            while( !octreeStack.empty() )
            {
                const Octree * curTree = octreeStack.top();
                octreeStack.pop();

                if(curTree->children.size() == 0)
                {
                    // 把最后一层的相交的八叉树盒子中所有面片加入 三角形集合
                    for(std::vector<Surface_mesh::Face>::const_iterator constIter = curTree->triangleData.begin(); constIter != curTree->triangleData.end(); constIter++)
                    {
                        Surface_mesh::Face face = *constIter;
                        triangleSet.insert( face.idx() );
                    }

                    // Debug:
                    //root()->selectedChildren.push_back(curTree);
                }

                // Do following if child size > 0
                for (std::vector<Octree>::const_iterator child = curTree->children.begin();  child != curTree->children.end(); child++)
                {
                    if ( child->boundingBox.intersects(ray) )
                    {
                        octreeStack.push( &(*child) );
                    }
                }
            }

            if(isFullTest)
            {
                // 是否全测试
                QSet<int> exactSet;
                for(int faceIndex : triangleSet)
                {
                    HitResult hitRes;
                    rayTriangleIntersectionTest(Surface_mesh::Face(faceIndex), ray, hitRes, false);
                    if(hitRes.hit) exactSet.insert(faceIndex);
                }
                return exactSet;
            }
        }

        return triangleSet;
    }

    /**
     * @brief intersectRecursiveRay
     * 递归的将所有有交集的叶子节点的面片塞入集合
     * @param ray
     * @param triangleSet
     */
    void intersectRecursiveRay( const Ray& ray, IndexSet& triangleSet )
    {
        if(children.size() == 0)
        {
            for(std::vector<Surface_mesh::Face>::iterator it = triangleData.begin(); it != triangleData.end(); it++)
            {
                Surface_mesh::Face face = *it;
                triangleSet.insert( face.idx() );
            }

            // Debug:
            //root()->selectedChildren.push_back(this);
        }

        // Do following if child size > 0
        for (std::vector<Octree>::iterator child = children.begin();  child != children.end(); child++)
        {
            if ( child->boundingBox.intersects(ray) )
            {
                child->intersectRecursiveRay(ray, triangleSet);
            }
        }
    }

    /**
     * @brief intersectRecursiveSphere
     * @param sphere_center
     * @param radius
     * @param tris
     */
    void intersectRecursiveSphere( const Eigen::Vector3d& sphere_center, double radius, IndexSet& triangleSet )
    {
        // Leaf node ?
        if (intersectHit(triangleSet))
            return;

        // Visist children
        for (std::vector<Octree>::iterator child = children.begin();  child != children.end(); child++)
        {
            if (child->boundingBox.intersectsSphere(sphere_center, radius))
                child->intersectRecursiveSphere(sphere_center, radius, triangleSet);
        }
    }

    /**
     * @brief testIntersectHit
     * 测试射线是否与面片相交
     * @param ray
     * @param hitRes
     * @return
     */
    bool testIntersectHit( const Ray& ray, HitResult & hitResult )
    {
        if(this->children.size() > 0)
            return false;

        // Do actual intersection test
        for(std::vector<Surface_mesh::Face>::iterator face = triangleData.begin(); face != triangleData.end(); face++)
        {
            Surface_mesh::Face f = *face;
            rayTriangleIntersectionTest(f, ray, hitResult, true);
            if(hitResult.hit)
                return true;
        }

        return false;
    }

    Octree * root()
    {
        if(parent == NULL)
            return this;
        else
            return parent->root();
    }

    /**
     * @brief triPoints
     * 获得面片的三个顶点
     * @param f
     * @return
     */
    std::vector<Eigen::Vector3d> triPoints(Surface_mesh::Face f) const
    {
        std::vector<Eigen::Vector3d> pnts;
        Surface_mesh::Vertex_around_face_circulator vit = mesh->vertices(f),vend=vit;
        do{ pnts.push_back(points[vit]); } while(++vit != vend);
        return pnts;
    }

    /**
     * @brief rayTriangleIntersectionTest
     * 面片与三角形面片的相交测试
     * @param faceIndex
     * @param ray
     * @param hitResult 返回的测试结果
     * @param allowBack
     */
    void rayTriangleIntersectionTest( Surface_mesh::Face faceIndex, const Ray & ray, HitResult & hitResult, bool allowBack = true ) const
    {
        hitResult.hit = false;
        hitResult.distance = DBL_MAX;

        // 精度值
        double EPS = 1e-7;

        std::vector<Eigen::Vector3d> v = triPoints(faceIndex);

        Eigen::Vector3d vertex1 = v[0];
        Eigen::Vector3d vertex2 = v[1];
        Eigen::Vector3d vertex3 = v[2];

        // Compute vectors along two edges of the triangle.
        Eigen::Vector3d edge1 = vertex2 - vertex1;
        Eigen::Vector3d edge2 = vertex3 - vertex1;

        // Compute the determinant.
        Eigen::Vector3d directionCrossEdge2 = cross(ray.direction, edge2);

        double determinant = dot(edge1, directionCrossEdge2);

        // If the ray is parallel to the triangle plane, there is no collision.
        if (fabs(determinant) < EPS)
            return;

        double inverseDeterminant = 1.0 / determinant;

        // Calculate the U parameter of the intersection point.
        Eigen::Vector3d distVector = ray.origin - vertex1;
        double triangleU = dot(distVector, directionCrossEdge2);
        triangleU *= inverseDeterminant;

        // Make sure it is inside the triangle.
        if (triangleU < 0 - EPS || triangleU > 1 + EPS)
            return;

        // Calculate the V parameter of the intersection point.
        Eigen::Vector3d distanceCrossEdge1 = cross(distVector, edge1);
        double triangleV = dot(ray.direction, distanceCrossEdge1);
        triangleV *= inverseDeterminant;

        // Make sure it is inside the triangle.
        if (triangleV < 0 - EPS || triangleU + triangleV > 1 + EPS)
            return;

        // Compute the distance along the ray to the triangle.
        double rayDistance = dot(edge2, distanceCrossEdge1);
        rayDistance *= inverseDeterminant;

        if(!allowBack){
            // Is the triangle behind the ray origin?
            if (rayDistance < 0)
                return;
        }

        hitResult.hit = true;
        hitResult.distance = rayDistance;

        hitResult.u = triangleU;
        hitResult.v = triangleV;

        hitResult.index = faceIndex.idx();
    }

    /**
     * @brief intersectionTestAccelerated
     * 测试相交，加速版本
     * @param f
     * @param ray
     * @param res
     */
    void intersectionTestAccelerated( Surface_mesh::Face f, const Ray & ray, HitResult & res ) const
    {
        TriAcceld & triAccel = mesh->get_face_property<TriAcceld>("f:TriAccel")[f];
        double u, v, t;
        res.hit = triAccel.rayIntersect(TriAcceld::TriAccelRay(ray.origin, ray.direction), 0, DBL_MAX, u, v, t);

        res.distance = t;
        res.u = u;
        res.v = v;
    }

    void DrawBox(const Eigen::Vector3d& center, float width, float length, float height, float r, float g, float b, float lineWidth)
    {
        Eigen::Vector3d  c1, c2, c3, c4;
        Eigen::Vector3d  bc1, bc2, bc3, bc4;

        c1 = Eigen::Vector3d (width, length, height) + center;
        c2 = Eigen::Vector3d (-width, length, height) + center;
        c3 = Eigen::Vector3d (-width, -length, height) + center;
        c4 = Eigen::Vector3d (width, -length, height) + center;

        bc1 = Eigen::Vector3d (width, length, -height) + center;
        bc2 = Eigen::Vector3d (-width, length, -height) + center;
        bc3 = Eigen::Vector3d (-width, -length, -height) + center;
        bc4 = Eigen::Vector3d (width, -length, -height) + center;

        glDisable(GL_LIGHTING);

        glColor3f(r, g, b);
        glLineWidth(lineWidth);

        glBegin(GL_LINES);
        glVertex3dv(c1.data());glVertex3dv(bc1.data());
        glVertex3dv(c2.data());glVertex3dv(bc2.data());
        glVertex3dv(c3.data());glVertex3dv(bc3.data());
        glVertex3dv(c4.data());glVertex3dv(bc4.data());
        glVertex3dv(c1.data());glVertex3dv(c2.data());
        glVertex3dv(c3.data());glVertex3dv(c4.data());
        glVertex3dv(c1.data());glVertex3dv(c4.data());
        glVertex3dv(c2.data());glVertex3dv(c3.data());
        glVertex3dv(bc1.data());glVertex3dv(bc2.data());
        glVertex3dv(bc3.data());glVertex3dv(bc4.data());
        glVertex3dv(bc1.data());glVertex3dv(bc4.data());
        glVertex3dv(bc2.data());glVertex3dv(bc3.data());
        glEnd();
        glEnable(GL_LIGHTING);
    }

    void draw( double r, double g, double b, double lineWidth )
    {
        if(root() == this)
        {
            foreach(Octree * iset, selectedChildren){
                //iset->draw(1,0,0,lineWidth + 3);
                BoundingBox bb = iset->boundingBox;
                DrawBox(bb.center, bb.xExtent, bb.yExtent, bb.zExtent,1,0,0, lineWidth + 2);
            }
        }

        DrawBox(boundingBox.center, boundingBox.xExtent, boundingBox.yExtent, boundingBox.zExtent,r,g,b, lineWidth);

        for (std::vector<Octree>::iterator child = children.begin();  child != children.end(); child++)
            child->draw(r,g,b, lineWidth);
    }


private:

    void initBuild( std::vector<Surface_mesh::Face>& tris, int triPerNode )
    {
        // add triangles involved to "triangleData"
        this->triangleData = tris;
        this->trianglePerNode = triPerNode;

        // Create a big box
        BoundingBox bigBoundingBox;

        // Collect faces geometry
        points = mesh->vertex_property<Eigen::Vector3d>("v:point");
        std::vector< std::vector<Eigen::Vector3d> > triangles ( mesh->n_faces() );
        Surface_mesh::Face_iterator fit, fend = mesh->faces_end();
        for(fit = mesh->faces_begin(); fit != fend; ++fit){
            std::vector<Eigen::Vector3d> pnts;
            Surface_mesh::Vertex_around_face_circulator vit = mesh->vertices(fit),vend=vit;
            do{ pnts.push_back(points[vit]); } while(++vit != vend);
            triangles[Surface_mesh::Face(fit).idx()] = pnts;
        }

        // Pre-compute per triangle quantities for fast ray-intersection
#if USE_TRI_ACCEL
        Surface_mesh::Face_property< TriAcceld > tri_accel =
                mesh->face_property<TriAcceld>("f:TriAccel");
        for(fit = mesh->faces_begin(); fit != fend; ++fit){
            int i = Surface_mesh::Face(fit).idx();
            tri_accel[fit].load(triangles[i][0], triangles[i][1], triangles[i][2]);
        }
#endif

        bigBoundingBox.computeFromTris(triangles);

        // 备份模型原始包围盒大小
        originBoundingBox = bigBoundingBox;

        // Transform and scale to node's coordinates
        double largeSize = qMax(bigBoundingBox.xExtent, qMax(bigBoundingBox.yExtent, bigBoundingBox.zExtent));

        largeSize *= 1.25;

        // Define our bounding box
        this->boundingBox = BoundingBox(bigBoundingBox.center, largeSize, largeSize, largeSize);

        // Build the tree
        this->build();

        // Connect children with parent
        std::stack<Octree*> childStack;
        childStack.push(this);
        while(!childStack.empty())
        {
            Octree * curr = childStack.top(); childStack.pop();

            for(int i = 0; i < (int) curr->children.size(); i++)
            {
                curr->children[i].parent = curr;

                childStack.push( &curr->children[i] );
            }
        }
    }

    std::vector<Surface_mesh::Face> getIntersectingTris(const Eigen::Vector3d& v0, const Eigen::Vector3d& v1, const Eigen::Vector3d& v2, bool showIt)
    {
        if(this->triangleData.size() == 0 || this->children.size() == 0)
            return this->triangleData;

        std::vector<Surface_mesh::Face> res;

        for(int i = 0; i < (int) this->children.size(); i++)
        {
            if(children[i].boundingBox.containsTriangle(v0, v1, v2))
            {
                const std::vector<Surface_mesh::Face> tris = children[i].getIntersectingTris(v0, v1, v2, showIt);

                for(int j = 0; j < (int) tris.size(); j++)
                    res.push_back(tris[j]);
            }
        }

        return res;
    }

    void build( int depth = 0 )
    {
        if ((int)triangleData.size() > this->trianglePerNode)
        {
            if(depth < 8)
            {
                // Subdivide to 8 nodes
                newNode(depth, -1, -1, -1);
                newNode(depth, 1, -1, -1);
                newNode(depth, -1, 1, -1);
                newNode(depth, 1, 1, -1);
                newNode(depth, -1, -1, 1);
                newNode(depth, 1, -1, 1);
                newNode(depth, -1, 1, 1);
                newNode(depth, 1, 1, 1);
            }
        }
    }

    void newNode( int depth, double x, double y, double z )
    {
        double newExtent = boundingBox.xExtent / 2.0;

        Eigen::Vector3d center;

        center.x() = boundingBox.center.x() + (newExtent * x);
        center.y() = boundingBox.center.y() + (newExtent * y);
        center.z() = boundingBox.center.z() + (newExtent * z);

        BoundingBox bb(center, newExtent, newExtent, newExtent);

        // Add child
        children.push_back(Octree());
        Octree * child = &children.back();

        child->mesh = mesh;
        child->points = points;
        child->boundingBox = bb;
        child->trianglePerNode = this->trianglePerNode;

        // Collect triangles inside child's bounding box
        for(std::vector<Surface_mesh::Face>::iterator it = this->triangleData.begin(); it != this->triangleData.end(); it++)
        {
            Surface_mesh::Face face = *it;

            std::vector<Eigen::Vector3d> v = triPoints(face);

            if( bb.containsTriangle(v[0], v[1], v[2]) )
            {
                child->triangleData.push_back(face);
            }
        }

        child->build(depth + 1); // build it
    }

};

Q_DECLARE_METATYPE(Octree *)
