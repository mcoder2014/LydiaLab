#include "plugin.h"
#include <QtOpenGL>
#include "surface_mesh/gl_wrappers.h"

using namespace SurfaceMesh;

class SmoothRenderer : public SurfaceMeshRenderer{
    ///< index array for buffered OpenGL rendering
    std::vector<unsigned int> triangles; 
    
    void init() override {
        // qDebug() << "surfacemesh_render_flat::init";
        mesh()->update_face_normals();
        mesh()->update_vertex_normals();
    
        /// Initialize triangle buffer
        triangles.clear();
        for(Face face : mesh()->faces()) {
            for(Vertex vertex : mesh()->vertices(face)) {
                triangles.push_back(vertex.idx());
            }
        }
    }

    /**
     * @brief render
     * 在 OpenGL 画布上渲染模型的代码
     */
    void render() override {

        if(mesh()->n_faces() < 1)
            return;

        Surface_mesh::Vertex_property<Point>  points = mesh()->vertex_property<Point>(VPOINT);
        Surface_mesh::Vertex_property<Point>  vnormals = mesh()->vertex_property<Point>(VNORMAL);
    
        // Deal with color
        bool has_vertex_color = mesh()->has_vertex_property<Color>(VCOLOR);
        Surface_mesh::Vertex_property<Color>  vcolor;
        if (has_vertex_color)
            vcolor = mesh()->get_vertex_property<Color>(VCOLOR);
    
        // setup vertex arrays    
        gl::glVertexPointer(points.data());
        gl::glNormalPointer(vnormals.data());
        if(has_vertex_color) 
            gl::glColorPointer(vcolor.data());

        // 入栈
        glEnable(GL_LIGHTING);
        glShadeModel(GL_SMOOTH);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glPushMatrix();

        // 加入一层坐标转换
        Matrix4d transformMatrix = mesh()->getTransformationMatrix();
        glMultMatrixd(transformMatrix.data());

        if(has_vertex_color) glEnableClientState(GL_COLOR_ARRAY);
        if(!triangles.empty())
            glDrawElements(GL_TRIANGLES, (GLsizei)triangles.size(), GL_UNSIGNED_INT, &triangles[0]);

        glPopMatrix();
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        glDisable(GL_LIGHTING);
    }
};

Renderer* plugin::instance(){ return new SmoothRenderer(); }
