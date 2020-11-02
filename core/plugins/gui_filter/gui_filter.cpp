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
    /// Fill the menu with plugin names and make connections
    for(FilterPlugin* plugin: pluginManager()->filterPlugins()){

        QAction* action = plugin->action();
        QString pluginName = plugin->name();

        QMenu * assignedMenu = getParentMenu(pluginName);
        action->setText(getActionName(pluginName));
        assignedMenu->addAction(action);
        
        if(!action->icon().isNull())
            mainWindow()->filterToolbar->addAction(action);

        /// Connect after it has been added        
        connect(action, SIGNAL(triggered()), this, SLOT(startFilter()));
    }
}

/**
 * @brief gui_filter::startFilter
 * that setup the dialogs and asks for parameters
 * 当点击某插件时，更新其 UI
 */
void gui_filter::startFilter() {
    try
    {
        /// TODO: 暂停其他插件的工作状态

        // 找到触发信号的 plugin
        QAction* action = qobject_cast<QAction*>(sender());
        FilterPlugin* iFilter = qobject_cast<FilterPlugin*>(action->parent());
        if(!iFilter) return;
        
        /// Even though it's on the stack we associate it with the widget 
        /// in such a way that memory will get deleted when widget goes out 
        /// of scope
        /// 构建富文本参数 UI
        RichParameterSet* parameters = new RichParameterSet();
        iFilter->initParameters(parameters);
        int needUserInput = !parameters->isEmpty();
        
        /// I do not need the user input, just run it
        switch(needUserInput){
            case false:
                delete parameters;
                parameters = nullptr;
                execute(iFilter, nullptr);
                break;
            case true:
                FilterDockWidget* widget = new FilterDockWidget(iFilter,parameters,mainWindow());
                connect(widget,SIGNAL(execute(FilterPlugin*,RichParameterSet*)), this,SLOT(execute(FilterPlugin*,RichParameterSet*)));            
                mainWindow()->addDockWidget(Qt::RightDockWidgetArea, widget);
                widget->show();
                break;
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
    // 获得 该 plugin 应该插入的 Menu 位置
    QMenu * assignedMenu = mainWindow()->filterMenu;
    // 根据名称分类 名称规则 "category | filter name" 仅有一个 '|'
    if(filterName.contains("|"))
    {
        // Split by delimiter, filter category then filter name
        QStringList pluginNames = filterName.split("|");
        QString catName = pluginNames.front().trimmed();

        // Try to locate exciting submenu
        QMenu * m = nullptr;
        for(QMenu * child : assignedMenu->findChildren<QMenu*>()){
            QString menuName = child->title();
            if(menuName == catName){
                m = child;
                break;
            }
        }
        if(m == nullptr) {
            m = mainWindow()->filterMenu->addMenu(catName);
        }

        assignedMenu = m;
    }

    return assignedMenu;
}

QString gui_filter::getActionName(QString filterName)
{
    if(!filterName.contains("|"))
        return filterName.trimmed();

    QStringList pluginNames = filterName.split("|");
    return pluginNames.back().trimmed();
}

