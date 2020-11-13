#pragma once

#include <map>

#include <QActionGroup>

#include "interfaces/GuiPlugin.h"
#include "StarlabMainWindow.h"

using std::map;

/// At any point there can be a single editing plugin active.
/// When a plugin is active (i.e. not suspended) it intercept 
/// all the input (i.e. mouse/keyboard) actions.
class gui_filter : public GuiPlugin{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "gui_filter.plugin.starlab")
    Q_INTERFACES(GuiPlugin)
    
public:    
    /// plugin constructor
    virtual void load();
    
    QActionGroup *filterActionGroup;
    map<QAction*, FilterPlugin*> filterPluginMap;

private slots:
    // 点击 Filter
    void startFilter(QAction* action);
    /// callback function that starts the filter, it is called by the 
    /// filter popup when the user presses "ok" or by a command line instruction
    void execute(FilterPlugin* iFilter, RichParameterSet *pars);

private:
    QMenu *getParentMenu(QString filterName);
    QString getActionName(QString filterName);
};
