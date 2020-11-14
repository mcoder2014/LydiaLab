#pragma once

#include <map>

#include "interfaces/GuiPlugin.h"
#include "interfaces/ModePlugin.h"

class StateMachine;
using std::map;

/// At any point there can be a single editing plugin active.
/// When a plugin is active (i.e. not suspended) it intercept 
/// all the input (i.e. mouse/keyboard) actions.
class gui_mode : public GuiPlugin{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "gui_mode.plugin.starlab")
    Q_INTERFACES(GuiPlugin)

    /// The gui entry that suspends a plugin
public:
    // 默认 mode
    QAction* defaultModeAction;

    // Action Group 用作触发入口
    QActionGroup* modeActionGroup;

    // 构建二级菜单 map
    map<QString, QMenu*> secondMenuMap;

    // action -> modeplugin
    map<QAction*, ModePlugin*> modePluginMap;

    // plugin constructor
    void load();

private:

    void loadPlugins();
    QMenu *getParentMenu(QString pluginName);
    QString getActionName(QString pluginName);

public slots:
    void update();

    /// @{ State machine to manage suspension
private:
    // 状态机的状态
    enum STATE {
        DEFAULT = 0,
        SUSPENDED = 1,
        MODE = 2};

    // The current machinse state
    STATE state;

    // The action that was suspended
    QAction* lastActiveModeAction;

    // Called when entering a state
    void enterState(STATE state, QAction* action=nullptr);

public slots:
    // This causes changes of states
    void actionClicked(QAction *action);
    /// @}

public slots:

    /// Responds to a changes in document. If the plugin specifies its own way
    /// to respond to the event, this is used. This can be done by overloading
    /// ModePlugin::documenChanged(). If no custom behavior is provided, we simply
    /// call ModePlugin::destroy(), ModePlugin::create() in succession.
    void documentChanged();
};
