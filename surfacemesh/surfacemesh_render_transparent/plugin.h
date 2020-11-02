#pragma once
#include "SurfaceMeshPlugins.h"

/**
 * @brief The plugin class
 * 渲染插件的唯一目的就是作为 Renderer 的工厂，
 * 提供 Renderer 实例。
 */
class plugin
        : public SurfaceMeshRenderPlugin{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "surfacemesh_render_transparent.plugin.starlab")
    Q_INTERFACES(RenderPlugin)
  
    QString name() { return "Transparent"; }
    QIcon icon(){ return QIcon(":/icons/transparent_mesh.png"); }
    Renderer* instance();
};
