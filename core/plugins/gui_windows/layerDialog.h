#pragma once

#include <QtGui>
#include <QDockWidget>
#include "Starlab.h"
#include "Model.h"
#include "RichParameterSet.h"
#include "StarlabMainWindow.h"

class GLArea;
class QTreeWidget;
class GLLogStream;

#include <QDialog>
namespace Ui{ class layerDialog; }

/// @todo consider transforming this into yet another "Edit" plugin
class LayerDialog : public QDockWidget{
    Q_OBJECT
private:
    Ui::layerDialog* ui;
    Starlab::MainWindow* mainWindow;
    StarlabDocument* document(){ return mainWindow->document(); }
public:
    ~LayerDialog() override;
    LayerDialog(Starlab::MainWindow* mainwindow = nullptr);
    LayerDialog(const LayerDialog& ) = delete ;
    void updateDecoratorParsView();
    
public slots:
    /// Fills/Updates the layer table with content
    void updateTable();

    // 选择模型
    void modelItemClicked(QTreeWidgetItem * , int columnNumber);
    void showEvent( QShowEvent * event ) override;
    void showContextMenu(const QPoint& pos);

/// @{ slots for buttons at bottom of layer dialog
private slots:
    void onMoveModelUpReleased();
    void onMoveModelDownReleased();
    void onDeleteModelReleased();
/// @}
};
