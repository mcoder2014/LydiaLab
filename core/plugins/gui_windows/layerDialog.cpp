#include <QtGui>
#include <QDebug>

#include "layerDialog.h"
#include "Model.h"
#include "ui_layerDialog.h"

using namespace std;
using namespace Starlab;

/// Basic widget item of the layers interface for models
class LayersWidgetModelItem : public QTreeWidgetItem{
public:
    /// @todo change to Model const*
    Model& model;
public:
    LayersWidgetModelItem(Model& _model) :model(_model){
        if( model.isVisible) setIcon(0,QIcon(":/images/layer_eye_open.png"));
        if(!model.isVisible) setIcon(0,QIcon(":/images/layer_eye_close.png"));
        QString modelname = model.name;
        if (model.isModified) modelname += " *";
        setText(1, modelname);
    }
};

LayerDialog::~LayerDialog(){ delete ui; }

LayerDialog::LayerDialog(MainWindow* mainWindow) : 
    QDockWidget(mainWindow), ui(new Ui::layerDialog), mainWindow(mainWindow)
{
    this->setWindowTitle("Layer");

    setWindowFlags( windowFlags() | Qt::WindowStaysOnTopHint | Qt::SubWindow );
    QWidget::setAttribute( Qt::WA_MacAlwaysShowToolWindow);
    setVisible(false);
    ui->setupUi(this);
    
    // When document changes -> update layer table
    connect(mainWindow->document(), SIGNAL(hasChanged()),
            this, SLOT(updateTable()));
    
    // 鼠标左键点击
    connect(ui->modelTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem *, int)),
            this, SLOT(modelItemClicked(QTreeWidgetItem * , int )));

    /// TODO： 实现鼠标右键交互
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->modelTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->modelTreeWidget, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(showContextMenu(const QPoint&)));

    /// Setup the layer table
    ui->modelTreeWidget->clear();
    ui->modelTreeWidget->setColumnCount(3);
    ui->modelTreeWidget->setColumnWidth(0,60);
    ui->modelTreeWidget->setColumnWidth(1,80);
}

/**
 * @brief LayerDialog::modelItemClicked
 * 选择模型
 * @param item
 * @param column_number
 */
void LayerDialog::modelItemClicked(QTreeWidgetItem* item , int columnNumber){
    auto layerItem = dynamic_cast<LayersWidgetModelItem*>(item);
    if(layerItem){
        if( columnNumber == 0 ) {
            // 触发可见不可见
            layerItem->model.isVisible = !layerItem->model.isVisible;
        }
        else if( columnNumber > 0  ) {
            // 触发选中状态
            mainWindow->document()->setSelectedModel( &( layerItem->model ) );
        }
    }

    updateTable();
}

void LayerDialog::showEvent (QShowEvent* /* event*/){
    updateTable();
}

/**
 * @brief LayerDialog::showContextMenu
 * TODO：实现鼠标右键菜单
 */
void LayerDialog::showContextMenu(const QPoint& /*pos*/){

}

/**
 * @brief LayerDialog::updateTable
 * TODO: 不需要每次都重新建立 UI
 */
void LayerDialog::updateTable(){
    // qDebug() << __FUNCTION__ << __LINE__ << __FILE__;
    
    //TODO:Check if the current viewer is a GLArea
    if(!isVisible()) return;
    Document* document = mainWindow->document();
    ui->modelTreeWidget->clear();

    // Delegate the particular Model the
    // task to specify a layer widget item
    for(Model* model : document->models()){
        // Ask model to generate an item
        QTreeWidgetItem* item = model->getLayersWidgetItem();
        if(item==nullptr) {
            item = new LayersWidgetModelItem(*model);
        }
        model->decorateLayersWidgedItem(item);

        // Change color if currently selected
        if(model == mainWindow->document()->selectedModel()){
            item->setBackground(1, QBrush(Qt::yellow));
            item->setForeground(1, QBrush(Qt::blue));
        }
        // Add it to the tree
        ui->modelTreeWidget->addTopLevelItem(item);
    }
}

/**
 * @brief LayerDialog::onMoveModelUpReleased
 * 提升模型 layer
 */
void LayerDialog::onMoveModelUpReleased(){
    document()->raise_layer( document()->selectedModel() );
}

/**
 * @brief LayerDialog::onMoveModelDownReleased
 * 降低模型 layer
 */
void LayerDialog::onMoveModelDownReleased(){
    document()->lower_layer( document()->selectedModel() );
}

/**
 * @brief LayerDialog::onDeleteModelReleased
 * 删除一个模型
 */
void LayerDialog::onDeleteModelReleased(){
    Document* document = mainWindow->document();
    Model* model = document->selectedModel();
    document->deleteModel(model);
    updateTable();
}
