#include "Document.h"
#include <QFileInfo>
#include <QDir>
#include <cassert>

using namespace Starlab;

Document::Document(){
    /// @todo does EIGEN offer a way of doing this?
    _selectedModel = nullptr;
    name = "Unsaved Project";
    _isBusy = 0;
}

Document::~Document()
{
    clear();
}

void Document::pushBusy(){
    /// @todo change cursors on need
    // qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
    // qApp->restoreOverrideCursor();
    _isBusy++;
}

/**
 * @brief Document::popBusy
 * 解除 busy 模型，可能发送 hasChanged 信号
 * popBusy 是一个重量级函数，大量 slots 绑定在 hasChanged 上
 */
void Document::popBusy(){
    if(_isBusy>=1)
        _isBusy--;
    if(!isBusy())
        emit_hasChanged();
}

Eigen::AlignedBox3d Document::bbox() {
    Eigen::AlignedBox3d box( Eigen::Vector3d(-1,-1,-1), Eigen::Vector3d(1,1,1) );
    if(_models.size())
        box = _models.first()->bbox();

    /// @todo rotate bbox of model by its custom transfrom like meshlab was doing in box.Add(m->transform,m->bbox)
    foreach(Model* m, _models)
        box.extend( m->bbox() );
    return box;
}

/**
 * @brief Document::addModel
 * 加入模型
 * 1. 一定会产生 hasChanged 信号
 * 2. 当加入第一个模型时，一定会产生 selectionChanged 信号
 * @param model
 */
void Document::addModel(Model *model){
    pushBusy();
    if(model==nullptr) {
        throw StarlabException("Attempted to add a NULL model to the document");
    }
    _models.append(model);
    popBusy();

    /// First model added is selection
    if(_models.size()==1)
        setSelectedModel(model);
}

Model *Document::getModel(QString name)
{
    Model * found = nullptr;

    pushBusy();
    foreach(Model* m, _models){
        if(m->name == name){
            found = m;
            break;
        }
    }
    popBusy();

    return found;
}


void Document::deleteModel(Model *model){
    int midx = _models.indexOf(model);
    if(midx < 0){
        qDebug() << "[removeModel] model not found!";
        return;
    }

    pushBusy();
    /// Delete object & remove from list
    _models[midx]->deleteLater();
    _models.removeAt(midx);

    /// Update selection (if necessary)
    if(_selectedModel == model)
        _selectedModel = (_models.size()>0)?_models[0] : nullptr;
    popBusy();
}

void Document::clear(){
    pushBusy();
    /// Delete models individually
    for(Model* model : _models){
        model->deleteLater();
    }
    /// Clear the list
    _models.clear();
    /// Clear the selection
    _selectedModel = nullptr;
    popBusy();
}

Model* Document::selectedModel(){
    return _selectedModel;
}

/**
 * @brief Document::setSelectedModel
 * 设置模型为选中模型
 * @param model
 */
void Document::setSelectedModel(Model* model){
    /// Avoid trivial updates
    if(_selectedModel == model)
        return;

    if(_models.size()==0)
        assert(model==nullptr);

    if(_models.size()>0){
        assert(model!=nullptr); // no null selection
        assert(_models.contains(model)); // no meaningless selection
    }

    /// Perform update
    _selectedModel = model;
    emit selectionChanged(_selectedModel);
}

void Document::raise_layer(Model *model){
    int idx = _models.indexOf(model);
    if(idx==-1 || _models.size()<=1 ) return;
    if(idx<=0) return; ///< cannot raise more
    pushBusy();
    _models[idx  ] = _models[idx-1];
    _models[idx-1] = model;
    popBusy();
}

void Document::lower_layer(Model *model){
    int idx = _models.indexOf(model);
    if(idx==-1 || _models.size()<=1 ) return;
    if(idx>=(_models.size()-1)) return; ///< cannot lower more
    pushBusy();
    _models[idx] = _models[idx+1];
    _models[idx+1] = model;
    popBusy();
}

void Document::emit_resetViewport(){ 
    emit resetViewport();
}
void Document::emit_hasChanged(){
    // qDebug() << Q_FUNC_INFO;
    // qDebug() << __FUNCTION__ << __LINE__ << __FILE__;

    if (_models.isEmpty()) return; // Skip any updates if there are no models

    if(!isBusy())
        emit hasChanged();
    else
        qWarning("WARNING: attempted to Document::emit_hasChanged() on busy document");
}
