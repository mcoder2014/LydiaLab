#include "PluginManager.h"
#include <QObject>
#include <QRegExp>
#include <QPluginLoader>

#include "StarlabException.h"
#include "StarlabSettings.h"
#include "RenderPlugin.h"
#include "DecoratePlugin.h"
#include "FilterPlugin.h"
#include "GuiPlugin.h"
#include "ModePlugin.h"

#if defined(Q_OS_WIN)
    QString LIB_EXTENSION_FILTER("*.dll");
#elif defined(Q_OS_MAC)
    QString LIB_EXTENSION_FILTER("*.dylib");
#elif defined(Q_OS_LINUX)
    static QString LIB_EXTENSION_FILTER("*.so");
#endif

using namespace Starlab;
    
/**
 * @brief extractExtensions
 * 从插件名字中提取出插件支持的文件后缀名格式，使用 空格作为文件类型的间隔
 * @param a string like "[SurfaceMesh] Object File Format (*.off *.off2)"
 * @return a lowercase stringlist: {"off","off2"}
 */
QStringList extractExtensions(QString ioPluginName){
    QStringList retval;
    // qDebug() << "Parsing string: " << iopluginname;
    int openIndex  = ioPluginName.indexOf("(");
    int closeIndex = ioPluginName.indexOf(")");
    
    // Chunk of string between parenthesis, then split by comma
    ioPluginName = ioPluginName.mid(openIndex+1,closeIndex-openIndex-1);
    QStringList extensions = ioPluginName.split(QRegExp(" "));
    for(QString extension : extensions){
        int i_dot = extension.indexOf(".");
        extension = extension.mid(i_dot+1);
        // qDebug() << "extracted extension: " << extension;
        retval.append( extension.toLower() );
    }
    
    return retval;
}
    
static QString failurecauses_qtplugin(
"\nPOSSIBLE FAILURE REASONS:\n"
"  1) plugin needs a DLL which cannot be found in the executable folder\n"
"  2) Release / debug build mismatch\n"
"  3) Missing Q_INTERFACE(...) or Q_EXPORT_PLUGIN(...)\n"
"  *) any other reason?");

static QString failurecauses_starlabplugin(
"\nPOSSIBLE FAILURE REASONS:\n"
"  1) starlab PluginManager does not know how to load it\n"
"  2) Release / debug build mismatch.\n"
"  *) any other reason?");

/**
 * @brief PluginManager::PluginManager
 * 插件管理工具构造函数
 * 1. 读取程序配置文件
 * 2. 加载插件
 * @param settings
 */
PluginManager::PluginManager(Settings* settings) : 
    _settings(settings)
{
    //pluginsDir=QDir(getPluginDirPath()); 
    // without adding the correct library path in the mac the loading of jpg (done via qt plugins) fails
    //qApp->addLibraryPath(getPluginDirPath());
    //qApp->addLibraryPath(getBaseDirPath());
    
    try 
    {
        loadPlugins();
    } 
    STARLAB_CATCH_BLOCK
}

PluginManager::~PluginManager()
{
    foreach(QString pluginName, _plugins.keys())
        delete _plugins[pluginName];
}

FilterPlugin *PluginManager::getFilter(QString name){
    FilterPlugin* filter = _filterPlugins.value(name, nullptr);
    if(filter == nullptr)
        throw StarlabException("Cannot find filter '%s'", qPrintable(name));
    return filter;
}

/**
 * @brief PluginManager::loadPlugins
 * 程序启动时会执行，加载插件
 */
void PluginManager::loadPlugins() {

    qDebug() << "PluginManager::loadPlugins("
             << __FILE__ <<__PRETTY_FUNCTION__ << __LINE__ << ")";
    
    pluginsDir=QDir(getPluginDirPath());

    //only the files with appropriate extensions will be listed by function entryList()
    // 过滤出插件文件夹下拓展名是 *.dll *.so *.dylib 相关的文件，认为是插件
    pluginsDir.setNameFilters(QStringList(LIB_EXTENSION_FILTER));
    
    /// Load all plugins 对所有的插件文件，加载
    for (QString filepath : pluginsDir.entryList(QDir::Files)) {
        loadPlugin(filepath);
    }
}

/**
 * @brief PluginManager::loadPlugin
 * 加载指定插件
 * @param filepath
 */
void PluginManager::loadPlugin(QString filepath)
{
    /// Attempt to load a Qt plugin
    QPluginLoader loader(pluginsDir.absoluteFilePath(filepath));
    qDebug("Load Plugin %s %s\n", qPrintable(filepath), loader.load()?"true":"false");
    QObject* plugin = loader.instance();

    if(!plugin){
        // 如果插件加载失败，打印报错信息
        qDebug("Plugin '%s' is not a proper *Qt* plugin!! %s", qPrintable(filepath), qPrintable(failurecauses_qtplugin));
        return;
    }

    /// Attempt to load one of the starlab plugins
    bool loadok = false;

    // 1. 模型的输入输出插件
    loadok |= loadInputOutputPlugin(plugin);
    // 2. 工程的输入输出插件
    loadok |= loadProjectInputOutputPlugin(plugin);
    // 3. Filter 插件
    loadok |= loadFilterPlugin(plugin);
    // 4. 装饰器插件
    loadok |= loadDecoratePlugin(plugin);
    // 5. Gui 插件
    loadok |= loadGuiPlugin(plugin);
    // 6. Mode 插件
    loadok |= loadModePlugin(plugin);
    // 7. 渲染插件
    loadok |= loadRenderPlugin(plugin);

    if( !loadok )
        throw StarlabException("plugin " + filepath + " was not recognized as one of the declared Starlab plugin!!");

    // 所有插件都在 StarlabPlugin 中存储一份
    StarlabPlugin* splugin = dynamic_cast<StarlabPlugin*>(plugin);
    if(!splugin) {
        return;
    }

    /// Store pointers to all plugin
    _plugins.insert(splugin->name(),splugin);

    /// If we read here loading went ok
    // qDebug() << "Plugin: " << fileName << " loaded succesfully";
}

QString PluginManager::getBaseDirPath(){
    QDir baseDir(qApp->applicationDirPath());

#if defined(Q_OS_WIN)
    // Windows:
    // during development with visual studio binary could be in the debug/release subdir.
    // once deployed plugins dir is in the application directory, so
    if (baseDir.dirName() == "debug" || baseDir.dirName() == "release")        
        baseDir.cdUp();
#endif

#if defined(Q_OS_MAC)
    /// In MAC application is always deployed as "Starlab.app/Contents/MacOS/starlab"
    /// So going 2 folders up we will find the plugin folder
    baseDir.cdUp();
    baseDir.cdUp();
#endif

    return baseDir.absolutePath();
}

QString PluginManager::getPluginDirPath() {
    QDir pluginsDir(getBaseDirPath());
    if(!pluginsDir.exists("plugins"))
        throw StarlabException("Serious error. Unable to find the plugins directory at %s.", qPrintable(pluginsDir.absoluteFilePath("plugins")) );
    pluginsDir.cd("plugins");
    return pluginsDir.absolutePath();
}

bool PluginManager::loadProjectInputOutputPlugin(QObject *plugin){
    ProjectInputOutputPlugin* iIO = qobject_cast<ProjectInputOutputPlugin*>(plugin);
    if(!iIO) return false;
    projectIOPlugins.push_back(iIO);
    
    QStringList exts = extractExtensions( iIO->name() );
    for(QString ext : exts) {
        projectExtensionToPlugin.insert(ext,iIO);
    }

    return true;
}

/**
 * @brief PluginManager::loadInputOutputPlugin
 * 1. 按插件名称插入 map modelIOPlugins
 * 2. 提取插件支持的文件格式后缀名，插入 map modelExtensionToPlugin(Unique Map)
 *    所以最后加载的插件会覆盖之间插件的数据
 * @param plugin
 * @return
 */
bool PluginManager::loadInputOutputPlugin(QObject *plugin){
    InputOutputPlugin* ioPlugin = qobject_cast<InputOutputPlugin*>(plugin);
    if(!ioPlugin)
        return false;

    modelIOPlugins.insert(ioPlugin->name(), ioPlugin);

    /// Parse the extension filter into extensions
    QStringList exts = extractExtensions( ioPlugin->name() );
    for(QString ext : exts) {
        modelExtensionToPlugin[ext] = ioPlugin;
    }
    return true;
}

/**
 * @brief PluginManager::loadFilterPlugin
 * @param plugin
 * @return
 */
bool PluginManager::loadFilterPlugin(QObject *plugin){
    FilterPlugin* iFilter = qobject_cast<FilterPlugin*>(plugin);
    if(!iFilter) {
        return false;
    }

    _filterPlugins.insert(iFilter->name(), iFilter);
    return true;
}

bool PluginManager::loadDecoratePlugin(QObject *plugin){
    DecoratePlugin* iDecorate = qobject_cast<DecoratePlugin*>(plugin);
    if(!iDecorate) {
        return false;
    }
    _decoratePlugins.insert(iDecorate->name(), iDecorate);
    return true;
}

bool PluginManager::loadGuiPlugin(QObject* _plugin){
    GuiPlugin* plugin = qobject_cast<GuiPlugin*>(_plugin);
    if(!plugin) {
        return false;
    }
    _guiPlugins.insert(plugin->name(), plugin);
    return true;
}

/**
 * @brief PluginManager::loadModePlugin
 * 1. 加载 Mode Plugin
 * 2. 记录 Mode Plugin 的 Filter 支持的拓展名到
 *    modeExtensionToPlugin，因为是单例所以仅支持最后一个加载的插件
 * @param _plugin
 * @return
 */
bool PluginManager::loadModePlugin(QObject* _plugin){
    ModePlugin* plugin = qobject_cast<ModePlugin*>(_plugin);
    if(!plugin) {
        return false;
    }
    _modePlugins.insert(plugin->name(), plugin);

    /// Parse the extension filter into extensions
    QString filter = plugin->filter();
    if (!filter.isEmpty()) {
        QStringList exts = extractExtensions(filter);
        for(QString ext : exts) {
            modeExtensionToPlugin[ext] = plugin;
        }
    }

    return true;
}
 
/**
 * @brief PluginManager::loadRenderPlugin
 * 加载渲染插件
 * @param _plugin
 * @return
 */
bool PluginManager::loadRenderPlugin(QObject *_plugin){
    RenderPlugin* plugin = qobject_cast<RenderPlugin*>(_plugin);
    if(!plugin) {
        return false;
    }
    _renderPlugins.insert(plugin->name(), plugin);
    return true;
} 

/**
 * @brief PluginManager::getFilterStrings
 * 获得 Filter 字符串
 * @return
 */
QString PluginManager::getFilterStrings(){
    QStringList filters;
    
    /// Fill in filters for Model files
    QStringList extensions= modelExtensionToPlugin.keys();
    for(QString extension : extensions) {
        filters.append("*."+extension);
    }
    
    /// Fill in filters for Document files
    filters.prepend("Starlab Project (*.starlab)");
    filters.prepend("All Files (*.*)");
    
    /// Merge in a ;; separated list
    return filters.join(";;");
}

/**
 * @brief PluginManager::getRenderPlugin
 * 获得 Renderer 插件
 * @param pluginName
 * @return
 */
RenderPlugin *PluginManager::getRenderPlugin(QString pluginName){
    if(_renderPlugins.find(pluginName) == _renderPlugins.end()) {
        throw StarlabException("Renderer %s could not be found", qPrintable(pluginName));
    }
    return _renderPlugins[pluginName];;
}

/// @todo rename getDecoratePlugin(QString name)
DecoratePlugin* PluginManager::newDecoratePlugin(QString /*pluginName*/, Model* /*model*/){
    throw StarlabException("Decoration needs refactoring (See Render)");
#if 0
    Q_ASSERT(model!=NULL);
    DecoratePlugin *plugin = _decoratePlugins.value(pluginName,NULL);
    if(plugin==NULL) 
        throw StarlabException("Could not find plugin '%s'.",qPrintable(pluginName));  
    DecoratePlugin* newplugin = plugin->factory();
    newplugin->_mainWindow = plugin->_mainWindow;
    newplugin->_application = plugin->_application;
    model->addDecoratePlugin(newplugin);
    newplugin->_action = plugin->action();
    return newplugin;
#endif
}

/**
 * @brief PluginManager::getPreferredRenderer
 * 查找推荐的渲染器，查询顺序：
 *
 * 1. 查找 Setting 中的推荐
 * 2. 在支持当前 model 的 renderer 中选一个默认 renderer
 * 3. 选择第一个支持当前 model 的 renderer
 * @param model
 * @return
 */
QString PluginManager::getPreferredRenderer(Model *model) {
    QString key = "DefaultRenderer/" + QString(model->metaObject()->className());
    QString rendererName;

    if(settings()->contains(key)) {
        rendererName = settings()->getString(key);
    }

    /// Preferred plugins could not be found
    /// 如果 Setting 里没有默认的渲染器设置，
    /// 则遍历寻找一个 标有 Default 属性的插件
    if(!_renderPlugins.contains(rendererName)){
        for(RenderPlugin* plugin : _renderPlugins){
            if(plugin->isApplicable(model) && plugin->isDefault()){
                rendererName = plugin->name();
                break;
            }
        }
    }
    
    /// Couldn't find one that was marked as isDefault()
    /// just take the first that is applicable. 
    /// BBOX renderer should be found here!
    /// 如果没有 Default 渲染插件，则用 BBOX 渲染器
    if(!_renderPlugins.contains(rendererName)){
        for(RenderPlugin* plugin : _renderPlugins){
            if(plugin->isApplicable(model)){
                rendererName = plugin->name();
                break;
            }
        }
    }

    /// Everything failed..  let's give up
    if(!_renderPlugins.contains(rendererName))
        throw StarlabException("No suitable render plugin found\nIs it possible you didn't compile the BBOX renderer?");

    qDebug() << "The Prefered renderer plugin: " << rendererName;

    return rendererName;
}

void PluginManager::setPreferredRenderer(Model *model, RenderPlugin* plugin){
    // qDebug("PluginManager::setPreferredRenderer(..,%s)",qPrintable(plugin->name()));
    Q_ASSERT(plugin!=NULL);
    QString key = "DefaultRenderer/"+QString(model->metaObject()->className());
    settings()->set(key,plugin->name());
    settings()->sync();
}

QList<RenderPlugin *> PluginManager::getApplicableRenderPlugins(Model* model){
    QList<RenderPlugin*> retval;
    Q_ASSERT(model!=NULL);
    foreach(RenderPlugin* plugin, _renderPlugins.values())
        if( plugin->isApplicable(model) )
            retval.append(plugin);
    return retval;
}
