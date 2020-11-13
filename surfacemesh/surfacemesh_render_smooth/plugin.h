#pragma once
#include "SurfaceMeshPlugins.h"

class plugin : public SurfaceMeshRenderPlugin{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "surfacemesh_render_smooth.plugin.starlab")
    Q_INTERFACES(RenderPlugin)
  
    QString name() override { return "Smooth Shading"; }
    QIcon icon() override { return QIcon(":/icons/smooth_shading.png"); }
    Renderer* instance() override;
            
    bool isDefault() override { return true; }
};
