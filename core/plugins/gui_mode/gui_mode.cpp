#include "gui_mode.h"

#include <QKeyEvent>
#include <QApplication>
#include "interfaces/ModePlugin.h"
#include "StarlabMainWindow.h"
#include "StarlabDrawArea.h"

/**
 * @brief gui_mode::load
 * 初始化 mode gui
 * 1. 备份，减少 UI update 时耗时
 */
void gui_mode::load(){

    lastActiveModeAction = nullptr;
    
    modeActionGroup = new QActionGroup(mainWindow()->modeMenu);
    /// Managed manually
    modeActionGroup->setExclusive(false);
    
    /// Default mode (Trackball)
    defaultModeAction = new QAction (QIcon(":/images/no_edit.png"),"Default", this);
    defaultModeAction->setCheckable(true);
    defaultModeAction->setShortcut(Qt::Key_Escape);
    
    /// Reacts to changes in mode
    connect(modeActionGroup, SIGNAL(triggered(QAction*)),
            this, SLOT(actionClicked(QAction*)), Qt::UniqueConnection);
    
    /// Reacts to changes made on the selection
    connect(document(), SIGNAL(hasChanged()),
            this, SLOT(documentChanged()), Qt::UniqueConnection);

    /// When document changes, we make sure render menu/toolbars are up to date
    connect(document(), SIGNAL(hasChanged()),
            this, SLOT(update()), Qt::UniqueConnection);
    
    // 状态机进入 DEFAULT 模式
    enterState(DEFAULT, defaultModeAction);

    // 加载所有插件
    loadPlugins();
}

void gui_mode::loadPlugins()
{
    /// Clear the menus
    mainWindow()->modeToolbar->clear();
    mainWindow()->modeMenu->clear();

    /// Add the "default" mode action (modes disactivated)
    mainWindow()->modeToolbar->addAction(defaultModeAction);
    mainWindow()->modeMenu->addAction(defaultModeAction);
    modeActionGroup->addAction(defaultModeAction);

    /// Fill the menu with plugin names and make connections
    /// 自动构建二级菜单
    for(ModePlugin* plugin : pluginManager()->modePlugins()){

        QAction* action = plugin->action();
        action->setCheckable(true);
        action->setText(getActionName(plugin->name()));
        modePluginMap[action] = plugin;

        modeActionGroup->addAction(action);

        /// Add to menus and toolbars
        QMenu* parentMenu = getParentMenu(plugin->name());
        parentMenu->addAction(action);

        if(!action->icon().isNull()) {
            mainWindow()->modeToolbar->addAction(action);
        }
    }

    /// 二级菜单放置在 Action 后面
    mainWindow()->modeMenu->addSeparator();
    for(std::pair<QString, QMenu*> pair : secondMenuMap) {
        mainWindow()->modeMenu->addMenu(pair.second);
    }

    /// Remember trackball is always there, thus > 1
    bool showtoolbar = (mainWindow()->modeToolbar->children().size() > 1);
    mainWindow()->modeToolbar->setVisible(showtoolbar);
}

QMenu *gui_mode::getParentMenu(QString pluginName)
{
    // 根据名称分类 名称规则 "category | filter name" 仅有一个 '|'
    if(!pluginName.contains("|")){
        return mainWindow()->modeMenu;
    }

    // Split by delimiter, filter category then filter name
    QStringList pluginNames = pluginName.split("|");
    QString catName = pluginNames.front().trimmed();

    // Try to locate exciting submenu
    if(secondMenuMap.find(catName) == secondMenuMap.end()) {
        QMenu *menu = new QMenu(catName);
        secondMenuMap[catName] = menu;
    }

    return secondMenuMap[catName];
}

QString gui_mode::getActionName(QString pluginName)
{
    if(!pluginName.contains("|"))
        return pluginName.trimmed();

    QStringList pluginNames = pluginName.split("|");
    return pluginNames.back().trimmed();
}

/**
 * @brief gui_mode::update
 * 更新 mode gui
 * 1. 不显示当前不可用的 mode
 * 2. toolbar 只显示有 Icon 的 plugin
 * 3. 如果插件数量过少，不显示 插件
 */
void gui_mode::update(){
    for(QAction* action : modeActionGroup->actions()) {
        action->setEnabled(true);
        action->setCheckable(true);
        action->setChecked(false);
    }
}


/**
 * @brief gui_mode::enterState
 * 状态机转换
 * DEFAULT：
 *  1. ENABLE 其他 mode；
 *  2. 回退到 DEFAULT MODE；
 *  3. DEFAULT MODE UI 处理
 *
 * MODE:
 *  1. disable 其他 mode；
 *  2. 当前 MODE UI处理；
 *  3. DEFAULT MODE ACTION 处理;
 *
 * SUSPENDED:
 *  1. 暂停 当前 MODE；
 *  2. 回退到 DEFAULT MODE
 *  3. 修改 UI
 *
 * @param state DEFAULT, MODE, SUSPENDED
 * @param action
 */
void gui_mode::enterState(STATE state, QAction* action /*=NULL*/){

    switch(state){

    case DEFAULT:
        // qDebug() << "[DEFAULT]";
        Q_ASSERT(lastActiveModeAction==nullptr);
        Q_ASSERT(!mainWindow()->hasModePlugin());
        for(QAction* action : modeActionGroup->actions())
            action->setEnabled(true);
        defaultModeAction->setEnabled(false);
        defaultModeAction->setChecked(true);
        break;

    case MODE:
        // qDebug() << "[MODE]";
        Q_ASSERT(mainWindow()->hasModePlugin());
        for(QAction* action : modeActionGroup->actions())
            action->setEnabled(false);
        defaultModeAction->setEnabled(true);
        defaultModeAction->setChecked(false);
        action->setEnabled(true);
        action->setChecked(true);
        break;

    case SUSPENDED:
        // qDebug() << "[SUSPENDED]";
        Q_ASSERT(mainWindow()->hasModePlugin());
        Q_ASSERT(lastActiveModeAction != nullptr);
        for(QAction* action : modeActionGroup->actions())
            action->setEnabled(false);
        defaultModeAction->setEnabled(true);
        defaultModeAction->setChecked(true);
        lastActiveModeAction->setChecked(true);
        lastActiveModeAction->setEnabled(false);
        break;
    }

    // 更新真实的状态
    this->state = state;
}

/**
 * @brief gui_mode::actionClicked
 * 如果 Action 触发，调用相应的 mode plugin
 * @param action
 */
void gui_mode::actionClicked(QAction *action){
    // qDebug() << QString("gui_mode::actionClicked(%1)").arg(action->text());
    
    /// @internal Only used by "CREATE" but cannot declare in a switch statement
    ModePlugin* plugin = nullptr;
    
    switch(state){
    case DEFAULT:
        /// ---------------- IGNORING --------------------
        /// 忽视 Default mode plugin
        if(action == defaultModeAction) {
            break;
        } else {
            /// ---------------- CREATING --------------------
            plugin = modePluginMap[action];
            /// We can only switch to a mode plugin
            if(plugin == nullptr) {
                showMessage("Not existing plugin <%s>", action->text().toStdString().data());
                action->setChecked(false);
                break;
            }

            /// check applicable
            if(!document()->selectedModel() && !plugin->isApplicable()) {
                showMessage("The mode plugin <%s> is not applicable for current model",
                            plugin->name().toStdString().data());

                // 取消 action 的选中状态
                action->setChecked(false);
                break;
            }

            try{
                showMessage("Creating plugin: '%s'",qPrintable(action->text()));
                plugin->__internal_create();

                mainWindow()->setModePlugin(plugin);
                mainWindow()->resumeModePlugin();
                drawArea()->updateGL();
                lastActiveModeAction = action;
                enterState(MODE,action);

            } catch(...) {
                showMessage("Creating plugin: '%s' FAILED!", qPrintable(action->text()));
                action->setChecked(false);
                throw;
            }
        }
        break;

    case MODE:
        /// ---------------- TERMINATION --------------------
        /// 终止，进入状态 DEFAULT
        if(action == lastActiveModeAction){

            Q_ASSERT(mainWindow()->getModePlugin()!=nullptr);
            mainWindow()->getModePlugin()->__internal_destroy();
            mainWindow()->removeModePlugin();
            lastActiveModeAction = nullptr;
            enterState(DEFAULT);
            showMessage("Terminated plugin: '%s'",qPrintable(action->text()));
            break;
        }

        /// ---------------- SUSPENSION --------------------
        /// 暂停，进入状态 SUSPENDED
        if(action == defaultModeAction){

            QAction* actionToSuspend = lastActiveModeAction;
            ModePlugin* pluginToSuspend = modePluginMap[lastActiveModeAction];
            mainWindow()->suspendModePlugin();
            pluginToSuspend->suspend();
            enterState(SUSPENDED,actionToSuspend);
            showMessage("Suspended plugin: '%s'",qPrintable(actionToSuspend->text()));
            break;
        }
        break;

    case SUSPENDED:
        /// ---------------- RESUMING --------------------
        /// 继续，进入状态 MODE
        Q_ASSERT(action == defaultModeAction);
        mainWindow()->resumeModePlugin();
        modePluginMap[lastActiveModeAction]->resume();
        enterState(MODE, lastActiveModeAction);
        showMessage("Resumed plugin: '%s'",qPrintable(lastActiveModeAction->text()));
        break;
    }
    
    /// Transition state finished. Update GUI
    drawArea()->updateGL();
}

/**
 * @brief gui_mode::documentChanged
 * 订阅 Document 的 hasChanged 事件，
 * 交给当前的 Mode Plugin 进行处理
 */
void gui_mode::documentChanged(){  
    // qDebug("gui_mode::documentChanged()");
    if(!mainWindow()->hasModePlugin()) return;
    ModePlugin* modePlugin = mainWindow()->getModePlugin();

    switch(state){

    /// But there was no active plugin
    case DEFAULT:
        return;

        /// And there was an active plugin
    case MODE:
        /// 更新 Mode 响应
        if(!modePlugin->documentChanged()){
            modePlugin->__internal_destroy();
            modePlugin->__internal_create();
        }
        return;

        /// There was a suspended plugin
    case SUSPENDED:
        /// 当 mode 是暂停状态时，直接停掉插件
        modePlugin->__internal_destroy();

        mainWindow()->removeModePlugin();
        lastActiveModeAction = nullptr;
        enterState(DEFAULT);
        return;
    }
}
