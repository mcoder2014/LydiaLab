#include "gui_render.h"

#include "StarlabDrawArea.h"
#include <QDockWidget>
#include <QColorDialog>

using namespace Starlab;

/// Since we depend on the selected model, the load is minimal
void gui_render::load(){

    renderActionGroup = new QActionGroup(this);
    renderActionGroup->setExclusive(true);
    currentAsDefault = new QAction("Set current as default...",this);
    qColorDialog = nullptr;

    editRenderSettings = new QAction("Edit renderer settings...",this);
    editModelColor = new QAction("Change model color...",this);
    editBackgroundColor = new QAction("Change background color...",this);
    toggleBackgroundEffect = new QAction("Toggle background effect",this);
    clearRenderObjects = new QAction ("Clear render objects", this);
    clearRenderObjects->setToolTip("Removes all render objects from the scene. Render objects" \
                       "are typically used to visually debug the algorithms. "\
                       "This function allows you to clear them from the scene.");
    
    // 选择的模型发生变化 -> 调用 update
    connect(document(), SIGNAL(selectionChanged(Model*)),
            this, SLOT(update()));

    /// Connect click events to change in renderer system
    connect(renderActionGroup, SIGNAL(triggered(QAction*)),
            this, SLOT(triggerRendererAction(QAction*)), Qt::UniqueConnection);
    connect(currentAsDefault, SIGNAL(triggered()),
            this, SLOT(triggerSetDefaultRenderer()), Qt::UniqueConnection);
    connect(editRenderSettings, SIGNAL(triggered()),
            this, SLOT(trigger_editSettings()),Qt::UniqueConnection);
    connect(editModelColor, SIGNAL(triggered()),
            this, SLOT(trigger_editSelectedModelColor()), Qt::UniqueConnection);
    connect(editBackgroundColor, SIGNAL(triggered()),
            this, SLOT(trigger_editBackgroundColor()), Qt::UniqueConnection);
    connect(clearRenderObjects, SIGNAL(triggered()),
            drawArea(), SLOT(clear()), Qt::UniqueConnection);
    connect(toggleBackgroundEffect, &QAction::triggered,
            [&](){ drawArea()->toggleBackgroundEffect(); drawArea()->updateGL(); });

    /// 初始化 渲染器
    for(RenderPlugin* plugin : pluginManager()->renderPlugins()){

        QAction* action = plugin->action();
        renderPluginMap[action] = plugin;
        renderActionGroup->addAction(action);
        action->setCheckable(true);

        /// If it has icon.. add it to toolbar
        if(!action->icon().isNull())
            toolbar()->addAction(action);
    }

    /// @internal menu can be added only after it has been filled :(
    menu()->addAction(clearRenderObjects);
    menu()->addAction(editRenderSettings);
    menu()->addAction(currentAsDefault);
    menu()->addAction(editModelColor);
    menu()->addAction(editBackgroundColor);
    menu()->addAction(toggleBackgroundEffect);
    menu()->addSeparator();

    menu()->addActions(renderActionGroup->actions());

    toolbar()->setVisible(toolbar()->actions().size() > 0);
}

/**
 * @brief gui_render::update
 */
void gui_render::update(){

    if(!document()->selectedModel()) {
        return;
    }

    Renderer* currentRenderer = document()->selectedModel()->renderer();
    for(QAction* action : renderActionGroup->actions()) {
        action->setChecked(false);
    }
    currentRenderer->plugin()->action()->setChecked(true);

    /// Disable settings link when no parameters are given
    editRenderSettings->setDisabled(currentRenderer->parameters()->isEmpty());

}

void gui_render::triggerSetDefaultRenderer(){
    // qDebug() << "gui_render::triggerSetDefaultRenderer()";
    Model* model = document()->selectedModel();    
    RenderPlugin* renderer = model->renderer()->plugin();
    pluginManager()->setPreferredRenderer(model, renderer);
    QString message = QString("Preferred renderer for \"%1\" set to \"%2\"")
                              .arg(model->metaObject()->className())
                              .arg(renderer->name());
    showMessage(message.toStdString().data());
}

void gui_render::trigger_editSettings(){
    Renderer* renderer = document()->selectedModel()->renderer();
    /// No renderer set (btw... weird) skip
    if(renderer == nullptr) return;

    /// No parameters.. avoid useless empty widget
    if(renderer->parameters()->isEmpty()) return;
    
    /// Create 
    ParametersWidget* widget = new ParametersWidget(renderer->parameters(), mainWindow());
    
    /// Add a simple title
    widget->setWindowTitle(
                QString("Settings for '%1'").arg(renderer->plugin()->name()));
    
    /// Behave as independent window & stay on top
    widget->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
    
    /// Delete frame when its associated renderer gets killed
    connect(renderer, SIGNAL(destroyed()), widget, SLOT(deleteLater()));
    
    /// On a change in parameters, re-init, render and update
    connect(widget, SIGNAL(parameterChanged()), renderer, SLOT(init()));
    connect(widget, SIGNAL(parameterChanged()), renderer, SLOT(render()));
    connect(widget, SIGNAL(parameterChanged()), drawArea(), SLOT(updateGL()));    
    
    /// Finally show
    widget->show(); 
}

void gui_render::instantiate_color_dialog(){
    /// @internal on mac the (pretty) native dialog is buggy... randomly the native one opens
    /// https://bugreports.qt-project.org/browse/QTBUG-11188  
    /// Disconnect object from previous connections
    if(qColorDialog != nullptr){
        qColorDialog->disconnect();
        return;
    }
    
    qColorDialog = new QColorDialog();
    qColorDialog->hide();
    qColorDialog->setOption(QColorDialog::ShowAlphaChannel, true);
    qColorDialog->setWindowFlags(Qt::X11BypassWindowManagerHint | Qt::WindowStaysOnTopHint);
    connect(mainWindow(), SIGNAL(destroyed()),
            qColorDialog, SLOT(deleteLater()));
}

void gui_render::trigger_editBackgroundColor(){
    instantiate_color_dialog();
    qColorDialog->setCurrentColor( drawArea()->backgroundColor() );
    connect(qColorDialog, SIGNAL(currentColorChanged(QColor)), this, SLOT(liveupdate_backgroundColor(QColor)));
    // Predefind background colors
    qColorDialog->setCustomColor(0, QColor(255,255,255).rgb());
    qColorDialog->setCustomColor(1, QColor(208,212,240).rgb());
    qColorDialog->setCustomColor(2, QColor(50,50,60).rgb());
    qColorDialog->setCustomColor(3, QColor(0, 0, 0).rgb());
    qColorDialog->setCustomColor(4, QColor(136, 157, 179).rgb());
    qColorDialog->show();
}

void gui_render::trigger_editSelectedModelColor(){
    instantiate_color_dialog();
    connect(qColorDialog, SIGNAL(currentColorChanged(QColor)), this, SLOT(liveupdate_selectedModelColor(QColor)));
    qColorDialog->setCurrentColor(document()->selectedModel()->color);
    qColorDialog->show();
}

void gui_render::liveupdate_backgroundColor(QColor color){
    /// Force-remove background color from snapshots
    drawArea()->setBackgroundColor(color);
    drawArea()->updateGL();   
    
    /// Save it in the settings
    QString key = "DefaultBackgroundColor";
    settings()->set( key, QVariant( color ) );
    settings()->sync();
}

void gui_render::liveupdate_selectedModelColor(QColor color){
    Model* model = document()->selectedModel();
    if(model==nullptr) return;
    model->color = color;
    drawArea()->updateGL();        
}

/**
 * @brief gui_render::triggerRendererAction
 * 切换渲染器
 * @param action
 */
void gui_render::triggerRendererAction(QAction* action){

    Model* model = document()->selectedModel();
    RenderPlugin* plugin = renderPluginMap[action];

    if(!model || !plugin->isApplicable(model)) {
        showMessage("Renderer <%s> is not suitable for current model",
                    plugin->name().toStdString().data());
        return;
    }

    model->setRenderer(plugin);
    drawArea()->updateGL();    
}

