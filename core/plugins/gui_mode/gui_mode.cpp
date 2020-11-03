#include "gui_mode.h"

#include <QKeyEvent>
#include <QApplication>
#include "interfaces/ModePlugin.h"
#include "StarlabMainWindow.h"
#include "StarlabDrawArea.h"

/**
 * @brief gui_mode::load
 * 初始化 mode gui
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
    connect(modeActionGroup,SIGNAL(triggered(QAction*)),this,SLOT(actionClicked(QAction*)));
    
    /// Reacts to changes made on the selection
    connect(document(),SIGNAL(hasChanged()),this,SLOT(documentChanged()));
    /// When document changes, we make sure render menu/toolbars are up to date    
    connect(document(), SIGNAL(hasChanged()), this, SLOT(update()));
    
    /// Start state machine in default mode
    enterState(DEFAULT,defaultModeAction);
}

/**
 * @brief gui_mode::update
 * 更新 mode gui
 * 1. 不显示当前不可用的 mode
 * 2. toolbar 只显示有 Icon 的 plugin
 * 3. 如果插件数量过少，不显示 插件
 */
void gui_mode::update(){
    /// Clear the menus
    mainWindow()->modeToolbar->clear();
    mainWindow()->modeMenu->clear();

    /// Add the "default" mode action (modes disactivated)
    mainWindow()->modeToolbar->addAction(defaultModeAction);
    mainWindow()->modeMenu->addAction(defaultModeAction);
    modeActionGroup->addAction(defaultModeAction);
    
    /// Re-fill the menu with plugin names and make connections
    for(ModePlugin* plugin : pluginManager()->modePlugins()){

        if(!plugin->isApplicable()) continue;

        QAction* action = plugin->action();
        action->setCheckable(true);

        /// Make GUI elements exclusive
        modeActionGroup->addAction(action);

        /// Add to menus and toolbars
        mainWindow()->modeMenu->addAction(action);

        if(!action->icon().isNull())
            mainWindow()->modeToolbar->addAction(action);
    }
    
    /// Remember trackball is always there, thus > 1
    bool showtoolbar = (mainWindow()->modeToolbar->children().size() > 1);
    mainWindow()->modeToolbar->setVisible(showtoolbar);
}

/**
 * @brief gui_mode::enterState
 * 状态机状态切换
 * @param state
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
        defaultModeAction->setChecked(true);
        defaultModeAction->setEnabled(false);
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
        Q_ASSERT(lastActiveModeAction!=NULL);
        for(QAction* action : modeActionGroup->actions())
            action->setEnabled(false);        
        defaultModeAction->setChecked(true);
        defaultModeAction->setEnabled(true);
        lastActiveModeAction->setChecked(true);
        lastActiveModeAction->setEnabled(false);
        break;
    }    
    
    /// Finally update state
    this->state = state;
}

/**
 * @brief gui_mode::actionClicked
 * 这个函数有点儿丑呀……
 * @param action
 */
void gui_mode::actionClicked(QAction *action){
    // qDebug() << QString("gui_mode::actionClicked(%1)").arg(action->text());
    
    /// @internal Only used by "CREATE" but cannot declare in a switch statement
    ModePlugin* plugin = nullptr;
    
    switch(state){
    case DEFAULT:
        /// ---------------- IGNORING --------------------
        if(action==defaultModeAction)
            break;
        /// ---------------- CREATING --------------------
        if(action!=defaultModeAction){
            plugin = qobject_cast<ModePlugin*>( action->parent() );
            /// We can only switch to a mode plugin
            if(plugin == nullptr)
                break;
            try{
                showMessage("Creating plugin: '%s'",qPrintable(action->text()));
                plugin->__internal_create();
                /// No exception? set it to GUI
                mainWindow()->setModePlugin(plugin);
                mainWindow()->resumeModePlugin();
                drawArea()->updateGL();
                lastActiveModeAction = action;
                enterState(MODE,action);
            } catch(...) {
                showMessage("Creating plugin: '%s' FAILED!",qPrintable(action->text()));
                action->setChecked(false);
                throw;
            } 
        }
        break;
    case MODE: 
        /// ---------------- TERMINATION --------------------
        /// 终止，进入状态 DEFAULT
        if(action==lastActiveModeAction){
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
        if(action==defaultModeAction){
            QAction* actionToSuspend = lastActiveModeAction;
            ModePlugin* pluginToSuspend = (ModePlugin*) lastActiveModeAction->parent();
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
        Q_ASSERT(action==defaultModeAction);
        mainWindow()->resumeModePlugin();
        ((ModePlugin*) lastActiveModeAction->parent())->resume();
        enterState(MODE,lastActiveModeAction);
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
    ModePlugin* iMode = mainWindow()->getModePlugin();

    switch(state){
    /// But there was no active plugin
    case DEFAULT:
        return;
    /// And there was an active plugin
    case MODE:
        /// Give the plugin a chance to react to the selection change
        /// If plugin didn't specify how to perform the update, simply 
        /// destroy it and re-create it from scratch.
        if(!iMode->documentChanged()){
            iMode->__internal_destroy();
            iMode->__internal_create();
        }
        return;
    /// There was a suspended plugin
    case SUSPENDED:
        /// On the other hand, when plugin is suspended, change in document just 
        /// results in the plugin termination
        if(!iMode->documentChanged())
            iMode->__internal_destroy();
        mainWindow()->removeModePlugin();
        lastActiveModeAction = nullptr;
        enterState(DEFAULT);
        return;    
    }
}


