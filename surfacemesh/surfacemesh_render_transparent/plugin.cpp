#include <qgl.h>
#include "plugin.h"
#include "surface_mesh/gl_wrappers.h"

using namespace SurfaceMesh;

// GLU was removed from Qt in version 4.8 
#ifdef Q_OS_MAC
# include <OpenGL/glu.h>
#else
# include <GL/glu.h>
#endif

class SortingTransparentRenderer
        : public SurfaceMeshRenderer{
private:
    ///< index array for buffered OpenGL rendering
    std::vector<unsigned int> triangles;
    
private:
    using FaceItr = Surface_mesh::Face_iterator ;
    using DepthFace = std::pair<double, FaceItr> ;
    static bool depthSorter(DepthFace i, DepthFace j){ return (i.first < j.first); }

    std::vector< DepthFace > depthvec;

public:
    void render() override{
        // 顶点数组
        Surface_mesh::Vertex_property<Point>  points = mesh()->vertex_property<Point>("v:point");
        // 点法线数组
        Surface_mesh::Vertex_property<Point>  vnormals = mesh()->vertex_property<Point>("v:normal");
        // 顶点颜色数组
        bool has_vertex_color = mesh()->has_vertex_property<Color>("v:color");
        Surface_mesh::Vertex_property<Color>  vcolor;
        if (has_vertex_color)
            vcolor = mesh()->get_vertex_property<Color>("v:color");

        /// Sort faces
        /// 将三角形面片按照由远到近排序
//        {
//            // Get current view transforms
//            GLdouble projMatrix[16], modelMatrix[16];
//            glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
//            glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);

//            // Temporary variables for projection
//            double depth,x,y;
//            GLint viewport[4] = {0, 100, 100, -100};

//            /// Apply model transform and fill in depth & indexes
//            Surface_mesh::Face_iterator fit;
//            Surface_mesh::Face_iterator fend = mesh()->faces_end();
//            Surface_mesh::Vertex_around_face_circulator fvit, fvend;
//            for (fit = mesh()->faces_begin(); fit != fend; ++fit){

//                // Compute face center
//                Eigen::Vector3d faceCenter = Eigen::Vector3d::Zero();
//                fvit = fvend = mesh()->vertices(fit);
//                int c = 0;
//                do{
//                    faceCenter += points[fvit];
//                    c++;
//                } while (++fvit != fvend);

//                faceCenter /= c;
//                // Project to get range 0 - 1.0
//                gluProject(faceCenter.x(),faceCenter.y(),faceCenter.z(),
//                            modelMatrix, projMatrix, viewport,
//                            &x, &y, &depth);

//                depthvec.push_back(DepthFace(depth, fit));
//            }

//            /// Sort depth
//            std::sort(depthvec.begin(), depthvec.end(),
//                [](DepthFace i, DepthFace j)->bool{
//                    return i.first < j.first;
//            });

//            triangles.clear();
//            for(DepthFace &depth : depthvec) {
//                triangles.push_back(static_cast<uint>((*(depth.second)).idx()));
//            }
//        }

        /// Render
        {
            // 平滑渲染
            glShadeModel(GL_SMOOTH);
            // 多边形模式设置
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            glEnable(GL_LIGHTING);
            glEnable(GL_NORMALIZE);
            glEnable(GL_BLEND);

            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            // 临时关闭深度测试写入功能
            glDepthMask(false);

            glPushMatrix();
            Matrix4d transformMatrix = mesh()->getTransformationMatrix();
            glMultMatrixd(transformMatrix.data());

            {
                if(!has_vertex_color){
                    QColor& c = mesh()->color;
                    Eigen::Vector4d colorv(c.redF(), c.greenF(), c.blueF(), c.alphaF());
                    /// @todo minimum 10% transparency
                    colorv[3] = qMin(0.5,colorv[3]);
                    glColor4dv(colorv.data());
                }

                gl::glVertexPointer(points.data());
                gl::glNormalPointer(vnormals.data());
                if(has_vertex_color)
                    gl::glColorPointer(vcolor.data());
                glEnableClientState(GL_VERTEX_ARRAY);
                glEnableClientState(GL_NORMAL_ARRAY);
                if(has_vertex_color)
                    glEnableClientState(GL_COLOR_ARRAY);
                if(!triangles.empty())
                    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(triangles.size()), GL_UNSIGNED_INT, &triangles[0]);
                glDisableClientState(GL_VERTEX_ARRAY);
                glDisableClientState(GL_NORMAL_ARRAY);

            }

            glPopMatrix();
            // 恢复深度写入功能
            glDepthMask(true);

            glDisable(GL_LIGHTING);
            glDisable(GL_NORMALIZE);
            glDisable(GL_BLEND);
        }
    }

    // Renderer interface
public slots:
    void init() override{
        mesh()->update_face_normals();
        mesh()->update_vertex_normals();

        /// Initialize triangle buffer
        triangles.clear();
        for(Face f : mesh()->faces())
            for(Vertex v : mesh()->vertices(f))
                triangles.push_back(static_cast<unsigned int>(v.idx()));
    }
};

/**
 * @brief plugin::instance
 * 返回 Renderer 实例
 * @return
 */
Renderer* plugin::instance(){ return new SortingTransparentRenderer(); }
