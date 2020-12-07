#pragma once
#include <QDebug>
#include <QString>
#include <QObject>
#include <QColor>
#include <Eigen/Geometry>
#include <Eigen/StdVector>
#include "starlib_global.h"

/// @{ forward
    class QTreeWidgetItem;
    class DecoratePlugin;
    class RenderPlugin;
    class Renderer;    
/// @}

using Eigen::Vector3d;
using Eigen::Matrix4d;

namespace Starlab{
/// Interface of the basic characteristics that every model needs to have
class STARLIB_EXPORT Model : public QObject{
    Q_OBJECT

/// @{ Friends
    friend class Document; ///< to allow model deletion
/// @} 
    
/// @{ Constructors
public:
    Model():Model(""){}
    Model(QString path, QString name=QString());
    void assign(const Model& model);

protected:
    /// @note you should never delete a model explicitly, use document()->deleteModel(..)
    virtual ~Model();
/// @}        
    
/// @{ Overload these to customize appearance in the Layers dialog
public:
    /// A class that describes this class of models in the layer editor. The 
    /// default implementation doesn't specify anything, meaning the layerDialog
    /// will create something very basic. Rather than overriding this method you 
    /// should consider using its default implementation and override "decorateLayersWidgedItem"
    virtual QTreeWidgetItem* getLayersWidgetItem(){ return nullptr; }
    
    /// This takes the layer item of getLayersWidgetItem() and adds a bunch 
    /// of decorations on top of it. It is easier to customize this entry
    /// in your custom made model
    /// The default implementation just shows the full path as first entry
    virtual void decorateLayersWidgedItem(QTreeWidgetItem* parent);
/// @}
    
public:
    bool isVisible;       /// is the current model visible?
    bool isModified;      /// has the current model been modified (and not saved)? @todo make private and setter/getter to signal
    QString name;         /// name of the model (to be used in layer pane)
    QString path;         /// full path of the file associated to the model 
    QColor color;         /// Basic color (handy to see bboxes for example)
    /// @todo use the QGLViewer transformation type here
    // QMatrix4x4 transform; /// Per-model transformation (i.e. alignment matrix in a scan)

/// @{ Rendering 
public: 
    inline Renderer* renderer(){return _renderer;}
    void setRenderer
    (RenderPlugin* plugin);
private:
    Renderer* _renderer;
/// @}

/// @{ Decorators
/// 给模型附加 装饰器插件
/// 这个插件可以选在 DrawArea 中渲染
public:
    typedef QList<DecoratePlugin*> DecoratePlugins;
    const DecoratePlugins& decoratePlugins(){ return _decoratePlugins; }
    void removeDecoratePlugin(DecoratePlugin *plugin);
    void addDecoratePlugin(DecoratePlugin *plugin);
    bool hasDecoratePlugin(DecoratePlugin *plugin);
private:
    DecoratePlugins _decoratePlugins;
/// @}
    
/// @{ Basic geometry, needed for rendering
public:
    /// Retrieves a reference to the model bounding box
    const Eigen::AlignedBox3d& bbox(){ return _bbox; }
    
    /// @brief Overload to compute the bounding box for the current model: this->_bbox
    /// @note this necessary because we have to have a plausible BBOX for the scene, 
    ///       so that things like the view matrix can be determined
    virtual void updateBoundingBox() = 0;
    
    /// @brief Forces the bounding box to be set to a particular value
    void force_set_bbox(const Eigen::AlignedBox3d& newbbox){ _bbox = newbbox; }
    
protected:
    // 模型的包围盒
    Eigen::AlignedBox3d _bbox;
/// @}        

/// @{ Transformation
protected:

    // 模型 bbox center 的位置
    Vector3d position;

    // 单位是弧度值
    Vector3d rotation;

    // 缩放，初始时为 (1.0, 1.0, 1.0)
    Vector3d scale;

public:
    inline Eigen::Vector3d getPosition(){return position;}
    inline Eigen::Vector3d getRotation(){return rotation;}
    inline Eigen::Vector3d getScale(){return scale;}
    Matrix4d getTransformationMatrix();

    inline void setPosition(const Vector3d& position){this->position = position;}
    inline void setRotation(const Vector3d& rotation){this->rotation = rotation;}
    inline void setScale(const Vector3d& scale){this->scale = scale;}

    Vector3d toWorldCoordinate(const Vector3d& point);
    Vector3d toLocalCoordinate(const Vector3d& point);

private:
    Vector3d coordinateTrans(const Vector3d& point, const Matrix4d& transMat);

/// @}
};

} /// namespace 

typedef Starlab::Model StarlabModel;

Q_DECLARE_INTERFACE(Starlab::Model, "Starlab::Model/2.0")
