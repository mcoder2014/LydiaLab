#include "Model.h"

#include <QTreeWidgetItem>  /// Plugins define items for layer tree menu
#include <QMenu>
#include <QToolBar>
#include <QAction>          /// Plugins define actions for toolbar buttons and such
#include <QFileInfo>        /// To automatically determine basename

#include <Eigen/Geometry>

#include "StarlabException.h"
#include "RenderPlugin.h"
#include "DecoratePlugin.h"

using namespace Starlab;

Model::Model(QString path, QString name){
    this->path = "";
    this->name = "";
    this->color = Qt::gray;
    this->isVisible = true;
    this->isModified = false;
    this->_renderer = nullptr;
    this->path = path;
    this->name = name.isEmpty() ? QFileInfo(path).baseName() : name;

    // 初始化坐标信息
    position = Vector3d::Zero();
    rotation = Vector3d::Zero();
    scale = Vector3d::Ones();
}

Model::~Model(){
    if(_renderer) 
        delete _renderer;
}

void Model::decorateLayersWidgedItem(QTreeWidgetItem* parent){
    QTreeWidgetItem *fileItem = new QTreeWidgetItem();
    fileItem->setText(1, QString("Path"));    
    fileItem->setText(2, this->path);
    parent->addChild(fileItem);
    // updateColumnNumber(fileItem);
}

/**
 * @brief Model::setRenderer
 * 设置一个渲染插件
 * @param plugin
 */
void Model::setRenderer(RenderPlugin* plugin){
    Q_ASSERT(plugin->isApplicable(this));
    if(_renderer != nullptr){
        // 如果有旧的渲染器，先停止并释放旧的渲染器
        _renderer->finalize();
        _renderer->deleteLater();
        _renderer = nullptr;
    }

    /// Get your own renderer instance
    _renderer = plugin->instance();   
    /// This deletes renderer upon model deletion
    _renderer->setParent(this);
    /// Record model and generating plugin inside renderer
    _renderer->_model = this;
    _renderer->_plugin = plugin;
    /// Now we are ready to initialize its parameters
    _renderer->initParameters();
    _renderer->init();
}

bool Model::hasDecoratePlugin(DecoratePlugin *plugin){
    return _decoratePlugins.contains(plugin);
}

/**
 * @brief Model::getTransformationMatrix
 * 快速获得转移矩阵
 * @return
 */
Eigen::Matrix4d Model::getTransformationMatrix()
{
    Eigen::Projective3d T = Eigen::Isometry3d::Identity();
    T.translate(position - bbox().center());
    T.rotate(Eigen::AngleAxisd(rotation.x(), Vector3d(1, 0, 0)));
    T.rotate(Eigen::AngleAxisd(rotation.y(), Vector3d(0, 1, 0)));
    T.rotate(Eigen::AngleAxisd(rotation.z(), Vector3d(0, 0, 1)));
    T.scale(scale);
    return T.matrix();
}


void Model::removeDecoratePlugin(DecoratePlugin *plugin){
    _decoratePlugins.removeAll(plugin);
}

void Model::addDecoratePlugin(DecoratePlugin* plugin){
    plugin->_model = this;
    plugin->setParent(this);
    _decoratePlugins.append(plugin);
}

