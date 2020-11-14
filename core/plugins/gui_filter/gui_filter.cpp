#include "gui_filter.h"

#include <QToolTip>
#include <QToolBar>
#include <QMessageBox>

#include "OSQuery.h"
#include "FilterDockWidget.h"
#include "StarlabMainWindow.h"
#include "parameters/RichParameterSet.h"
#include "interfaces/FilterPlugin.h"
#include <QElapsedTimer>

/**
 * @brief gui_filter::load
 * 加载 filter 按钮
 */
void gui_filter::load(){

    filterActionGroup = new QActionGroup(mainWindow()->filterMenu);
    filterActionGroup->setExclusive(false);

    /// 加入插件到 Menu 和 图标
    /// 自动构建二级菜单
    for(FilterPlugin* plugin : pluginManager()->filterPlugins()){

        QAction* action = plugin->action();
        action->setText(getActionName(plugin->name()));
        filterPluginMap[action] = plugin;
        filterActionGroup->addAction(action);

        QMenu * parentMenu = getParentMenu(plugin->name());
        parentMenu->addAction(action);
        
        if(!action->icon().isNull())
            mainWindow()->filterToolbar->addAction(action);
    }

    // 二级菜单放在 Action 后方
    mainWindow()->filterMenu->addSeparator();
    for(std::pair<QString, QMenu*> pair : secondMenuMap) {
        mainWindow()->filterMenu->addMenu(pair.second);
    }

    connect(filterActionGroup, SIGNAL(triggered(QAction*)),
            this, SLOT(startFilter(QAction*)), Qt::UniqueConnection);

}

/**
 * @brief gui_filter::startFilter
 * 触发指定 Filter
 * @param action
 */
void gui_filter::startFilter(QAction *action)
{
    FilterPlugin* filterPlugin = filterPluginMap[action];
    if(!filterPlugin) {
        showMessage("Filter Plugin <%s> is not existed.",
                    action->text().toStdString().data());
        return;
    }

    if(!filterPlugin->isApplicable(document()->selectedModel())) {
        showMessage("Current selectedModel is not suitable for <%s>",
                    filterPlugin->name().toStdString().data());
        return;
    }

    try
    {
        /// 构建富文本参数 UI
        /// TODO: 阻止重复构建 UI
        RichParameterSet* parameters = new RichParameterSet();
        filterPlugin->initParameters(parameters);

        if(parameters->isEmpty()){
            delete parameters;
            parameters = nullptr;
            execute(filterPlugin, nullptr);
        }else {
            // 构建自动生成 UI
            FilterDockWidget* widget = new FilterDockWidget(filterPlugin,parameters,mainWindow());
            connect(widget, SIGNAL(execute(FilterPlugin*,RichParameterSet*)),
                    this, SLOT(execute(FilterPlugin*,RichParameterSet*)));
            mainWindow()->addDockWidget(Qt::RightDockWidgetArea, widget);
            widget->show();
        }
    }
    STARLAB_CATCH_BLOCK
}

/**
 * @brief gui_filter::execute
 * callback invoked when the params have been set up.
 * @param iFilter
 * @param parameters
 */
void gui_filter::execute(FilterPlugin* iFilter, RichParameterSet* parameters) {
    if(!iFilter->isApplicable(document()->selectedModel()))
        throw StarlabException("Filter is not applicable");

    /// @todo save the current filter and its parameters in the history
    // meshDoc()->filterHistory.actionList.append(qMakePair(iFilter->name(),params));
    
    /// @todo re-link the progress bar
    // qb->reset();
    
    qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
    document()->pushBusy();
    try {
        QElapsedTimer t;
        t.start();

        iFilter->applyFilter(parameters);

        mainWindow()->setStatusBarMessage("Filter '"+ iFilter->name() +"' Executed " + QString("(%1ms).").arg(t.elapsed()),5000);
    } STARLAB_CATCH_BLOCK
            document()->popBusy();
    qApp->restoreOverrideCursor();
    mainWindow()->closeProgressBar();
}

QMenu *gui_filter::getParentMenu(QString filterName)
{
    // 根据名称分类 名称规则 "category | filter name" 仅有一个 '|'
    if(!filterName.contains("|")){
        return mainWindow()->filterMenu;
    }

    // Split by delimiter, filter category then filter name
    QStringList pluginNames = filterName.split("|");
    QString catName = pluginNames.front().trimmed();

    // Try to locate exciting submenu
    if(secondMenuMap.find(catName) == secondMenuMap.end()) {
        QMenu *menu = new QMenu(catName);
        secondMenuMap[catName] = menu;
    }

    return secondMenuMap[catName];
}

QString gui_filter::getActionName(QString filterName)
{
    if(!filterName.contains("|"))
        return filterName.trimmed();

    QStringList pluginNames = filterName.split("|");
    return pluginNames.back().trimmed();
}

