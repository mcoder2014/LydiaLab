#pragma once
#include "SurfaceMeshPlugins.h"

class plugin : public SurfaceMeshRenderPlugin{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "surfacemesh_render_wireframe.plugin.starlab")
    Q_INTERFACES(RenderPlugin)
   
public: 
    QString name() override { return "Wireframe"; }
    QIcon icon() override { return QIcon(":/icons/wireframe.png"); }
    Renderer* instance() override;
};
