#include "StarlabApplication.h"
#include "FilterPlugin.h"
#include "parameters/RichParameterSet.h"

#include "ModePlugin.h"

using namespace Starlab;

Application::Application(){
    /// Instantiate resources
    _settings      = new Settings(this);
    _pluginManager = new PluginManager(_settings);
    _document      = new Document();
    
    /// Register all plugins access functionality
    for(StarlabPlugin* plugin : pluginManager()->plugins()){
        plugin->_application = this;
    }
    
    /// Register Starlab.h types
    qRegisterMetaType<BBox3>("BBox3");
}

Application::~Application(){
    delete _document;
    delete _pluginManager;
    delete _settings;
}

bool Application::saveModel(Model* model, QString path){
    if(path.isEmpty()) path=model->path;
    // qDebug() << "Saving model: " << model->name << " to file: " << path;
    QFileInfo fileInfo(path);
    
    /// Extract or set default extension
    QString extension = fileInfo.suffix().toLower();
    // qDebug() << "requested extension: " << extension;
    if( extension.isEmpty() ) extension = "off";
    
    /// Checks a suitable plugin exists
    InputOutputPlugin* iIO = pluginManager()->modelExtensionToPlugin[extension];
    if( !iIO ) throw StarlabException("Cannot find plugin suitable for the provided extension %s", qPrintable(extension));
    iIO->save(model,path);
    
    return true;
}

/**
 * @brief Application::loadModel
 * 调用 IO 插件来加载模型
 * @param path
 * @param plugin
 * @return
 */
bool Application::loadModel(QString path, InputOutputPlugin* plugin){
    // qDebug("StarlabApplication::loadModel(\"%s\")", qPrintable(path));
    QFileInfo fileInfo(path);
    QString extension = fileInfo.suffix().toLower();
    QString basename = fileInfo.completeBaseName();
    
    if(plugin == nullptr){
        // 如果输入的 plugin 无效，根据后缀名选一个合适的插件
        plugin = pluginManager()->modelExtensionToPlugin[extension];
    }
    if(plugin == nullptr){
        throw StarlabException("File '%s' has not been opened becase format '%s' not supported",
                                                 qPrintable(basename), qPrintable(extension));
    }
    // 加载
    loadModelUsingPlugin(path, plugin);
    return true;
}

/**
 * @brief Application::loadProject
 * 加载一个项目
 * @param path
 * @param plugin
 * @return
 */
bool Application::loadProject(QString path, ProjectInputOutputPlugin* plugin){   
    // qDebug("StarlabApplication::loadProject(\"%s\")", qPrintable(path));
    QFileInfo fileInfo(path);
    QString extension = fileInfo.suffix().toLower();
    QString basename = fileInfo.completeBaseName();

    if(plugin==nullptr){
        plugin = pluginManager()->projectExtensionToPlugin[extension];
    }
    
    /// Checks file existence
    if(plugin == nullptr){
        throw StarlabException("Project file '%s' has not been opened, format %s not supported", qPrintable(basename), qPrintable(extension));
    }
    if(!fileInfo.exists()){
        throw StarlabException("Project file '%s' does not exist", qPrintable(path));
    }
    if(!fileInfo.isReadable()){
        throw StarlabException("Project file '%s' is not readable", qPrintable(path));
    }
    
    /// Clear the doc and use plugin to fill in
    document()->pushBusy();
    document()->clear();
    plugin->open(path, *this);
    document()->popBusy();
    
    /// Refresh visualization
    document()->emit_resetViewport();
    
    /// @todo catch exceptions in iIO->open and return false on any caught exception
    return true;
}

/**
 * @brief Application::loadByDrop
 * 通过拖拽的方式加载模型
 * @param path
 * @param plugin
 * @return
 */
bool Application::loadByDrop(QString path, ModePlugin *plugin)
{
    QFileInfo fileInfo(path);
    QString extension = fileInfo.suffix().toLower();
    QString basename = fileInfo.completeBaseName();

    if (plugin == nullptr){
        plugin = pluginManager()->modeExtensionToPlugin[extension];
    }
    /// Checks file existence
    if(plugin == nullptr){
        throw StarlabException("File '%s' has not been opened, format %s not supported", qPrintable(basename), qPrintable(extension));
    }
    if(!fileInfo.exists()){
        throw StarlabException("File '%s' does not exist", qPrintable(path));
    }
    if(!fileInfo.isReadable()){
        throw StarlabException("File '%s' is not readable", qPrintable(path));
    }

    /// Clear the doc and use plugin to fill in
    document()->pushBusy();
    /// load by drop here
    bool isLoaded = plugin->loadByDrop(path);
    if (!isLoaded)
        throw StarlabException("Loading by drop fails, unsupported file format");
    document()->popBusy();
    return true;
}

QList<FilterPlugin*> Application::applicableFilters(){
    return pluginManager()->filterPlugins();
}

QList<FilterPlugin *> Application::applicableFilters(Model *model){
    QList<FilterPlugin*> retval;
    for(FilterPlugin* plugin:pluginManager()->filterPlugins())
        if(plugin->isApplicable(model))
            retval.append(plugin);
    return retval;
}

void Application::load(QString path){
    // qDebug("StarlabApplication::load(\"%s\")", qPrintable(path));
    bool retstatus = false;
    if(!retstatus) retstatus = loadModel(path,nullptr);
    if(!retstatus) retstatus = loadProject(path,nullptr);
    if(!retstatus) retstatus = loadByDrop(path, nullptr);
    
    /// Nothing was able to open
    if(!retstatus)
        throw StarlabException("Starlab does not know how to open file: " + path);
}

void Application::executeFilter(Model* model, QString filterName){
    // qDebug() << "StarlabApplication::executeFilter()";
    FilterPlugin* filter = pluginManager()->getFilter(filterName);
    if(!filter->isApplicable(model))
        throw StarlabException("Filter not applicable");
    
    /// Filter is applied on the *selected* model
    document()->setSelectedModel(model);
    RichParameterSet* pars = new RichParameterSet();
    filter->initParameters(pars);
    filter->applyFilter(pars);
    pars->deleteLater();
}

QDir Application::starlabDirectory(){
    QDir baseDir(QApplication::applicationDirPath());
    if( OSQuery::isMac() ){
        baseDir.cdUp();
        baseDir.cdUp();
        return baseDir.absolutePath();
    }
    if( OSQuery::isWin() )
        return QCoreApplication::applicationDirPath();
    if( OSQuery::isLinux() )
        throw StarlabException("TODO: FIX THE INI LOAD PATH FOR LINUX!!!");

    /// @todo is there better way of doing this without having to return dummy?
    Q_ASSERT(false);
    return QDir("");
}

QDir Application::executionDirectory(){
    return QFileInfo("./").absoluteDir();
}

/**
 * @brief Application::loadModelUsingPlugin
 * 使用插件加载模型
 * 1. 会发送 selectionChanged 信号
 * 2. 会发送 hasChanged 信号
 * @param path
 * @param plugin
 * @return
 */
bool Application::loadModelUsingPlugin(QString path, InputOutputPlugin *plugin)
{
    /// Checks file existence
    plugin->checkReadable(path);

    Model* newModel = plugin->open(path);
    if(newModel==nullptr) {
        return false;
    }
    /// Update the bounding box (for rendering reasons)
    newModel->updateBoundingBox();
    document()->addModel( newModel );

    // 选择新载入的模型，并渲染
    document()->setSelectedModel( newModel );
    document()->emit_resetViewport();

    return true;
}

