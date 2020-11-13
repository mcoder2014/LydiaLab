#include "gui_filemenu.h"

#include <QFileInfo>
#include "StarlabDrawArea.h"
#include "OSQuery.h"

const static QString ALL_FILTER = "All Files (*.*)";

using namespace Starlab;

void gui_filemenu::delete_selected_model(){
    if(selectedModel()==nullptr)
        return;
    document()->deleteModel( selectedModel() );
}

void gui_filemenu::open(){
    /// Restore browsing directory from the cache
    QDir lastUsedDirectory( settings()->getString("lastUsedDirectory") );
    
    /// Builds allowed extensions / filter list
    QString filters = ALL_FILTER;
    {
        QTextStream sout(&filters);
        for(InputOutputPlugin* plugin : pluginManager()->modelIOPlugins) {
            sout << ";;" << plugin->name();
        }
        for(ProjectInputOutputPlugin* plugin : pluginManager()->projectIOPlugins) {
            sout << ";;" << plugin->name();
        }
    }
    
    /// Prompt user for file to open
    QString selectedFilter;
    QString fileName = QFileDialog::getOpenFileName(
                mainWindow(), tr("Open Project/Model File"),
                lastUsedDirectory.path(),
                filters, &selectedFilter);
    
    /// "Cancel" button was pressed
    if(fileName.isNull()) {
        return;
    }
    
    /// Cache the opened directory for future use
    /// do it early so if operation fail it's still cached
    QFileInfo fileInfo(fileName);
    settings()->set( "lastUsedDirectory", fileInfo.absolutePath() );
    
    /// If user is trustring the "auto-loader"
    if(selectedFilter == ALL_FILTER) {
        application()->load(fileName);
    }else{

        // 根据 filter 查找到指定的  io plugin
        InputOutputPlugin* model_plugin = pluginManager()->modelIOPlugins.value(selectedFilter, nullptr);
        ProjectInputOutputPlugin* project_plugin = pluginManager()->projectExtensionToPlugin.value(selectedFilter, nullptr);
        Q_ASSERT(model_plugin==nullptr || project_plugin==nullptr);

        if(project_plugin != nullptr){
            application()->loadProject(fileName, project_plugin);
        }

        if(model_plugin != nullptr){
            application()->loadModel(fileName, model_plugin);
        }
    }
}

/// Case 1: only one model (didn't exist) and doc never existed
/// Case 2: only one model (did exist) and doc never existed
/// Case 3: multiple models but doc never existed
/// Case 4: multiple models and doc already existed
void gui_filemenu::save(){
    try
    {
        // qDebug() << "[[ENTERING]] gui_filemenu::save()";
        if(document()->models().size()==0)
            return;
        
        if(true){
            Starlab::Model* model = document()->selectedModel();
            bool pathAlreadySpecified = (model->path != "");
            bool success = false;
            
            /// Already know where to save
            if( pathAlreadySpecified )
                success = application()->saveModel(model, model->path);
            
            /// Query user where to save & save in model path
            else{
                QString lastDir = settings()->getString("lastUsedDirectory");
                QString fileName = QFileDialog::getSaveFileName(mainWindow(),"Save Selected Model",lastDir);
                if(fileName.isEmpty()) return;
                model->path = fileName;
                model->name = QFileInfo(fileName).baseName();
                success = application()->saveModel(model);
            }
            
            if(success){
                QFileInfo finfo(model->path);
                mainWindow()->setStatusBarMessage("Saved model at path: " + finfo.absoluteFilePath(),2.0f);
            } else {
                mainWindow()->setStatusBarMessage("Save operation failed...",2.0f);
            }
        } else {
            throw StarlabException("gui_file::save() modes 2...4 not implemented");
        }
    }
    STARLAB_CATCH_BLOCK
}


void gui_filemenu::reload_selection(){
    // qDebug() << "gui_filemenu::reload_selection()";
    mainWindow()->document()->pushBusy();
    Model* selection = mainWindow()->document()->selectedModel();
    if(selection==nullptr)
        return;

    QFileInfo fi(selection->path);

    if(!fi.exists())
        throw StarlabException("Cannot reload mode, file %s cannot be found.",selection->path.toStdString().c_str());
    
    /// Guess open plugin by extension
    QString extension = QFileInfo(selection->path).suffix().toLower();
    QList<InputOutputPlugin*> plugins = pluginManager()->modelExtensionToPlugin.values(extension);

    /// Check which of these have generated the model, then use it to re-open
    Model* newmodel = nullptr;
    foreach(InputOutputPlugin* plugin, plugins)
        if(plugin->isApplicable(selection))
            newmodel = plugin->open(selection->path);
    
    if(newmodel==nullptr)
        throw StarlabException("Impossible ot reload model");

    /// Compute its BBOX, otherwise rendering will not work
    newmodel->updateBoundingBox();

    /// Replace and set as selected model
    mainWindow()->document()->addModel(newmodel);
    mainWindow()->document()->deleteModel(selection);
    mainWindow()->document()->setSelectedModel(newmodel);
    mainWindow()->document()->popBusy();

    /// Inform the user
    mainWindow()->setStatusBarMessage("Model '"+ newmodel->name +"' reloaded from path: " + newmodel->path,5000);
}

void gui_filemenu::save_selection_as(){
    document()->selectedModel()->path = "";
    this->save();
}

void gui_filemenu::take_screenshot(){
    /// Screen Shot 2013-04-03 at 6.59.51 PM
    QString date = QDateTime::currentDateTime().toString("yyyy-MM-dd h.mm.ss AP");
    QString filename = QDir::homePath() + "/Desktop/" + QString("Screen Shot ") + date + ".png";
    //    QDir::setCurrent(QDir::homePath());

    // this uses qt stuff
    // drawArea()->setAttribute(Qt::WA_TranslucentBackground,true);
    QImage image = drawArea()->grabFrameBuffer(true);
    image.save(filename, "png",100);
    if(drawArea()->backgroundColor().alpha()<255 && drawArea()->format().samples()>1)
        qWarning() << "Antialiasing and transparent backgrounds do work well in snapshots"
                   << "Change background opacity to 100% in the render menu";

    showMessage("Screenshot saved at: %s",qPrintable(filename));
}
