#pragma once
#include "SurfaceMeshPlugins.h"
using namespace Starlab;

class surfacemesh_render_wireframe : public SurfaceMeshRenderPlugin{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "surfacemesh_render_wireframe.plugin.starlab")
    Q_INTERFACES(RenderPlugin)

    QString name() override { return SHADING::FLAT; }
    QIcon icon() override { return QIcon(":/icons/flat_shading.png"); }
    Renderer* instance() override;
};
