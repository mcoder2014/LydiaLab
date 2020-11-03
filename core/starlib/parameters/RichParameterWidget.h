#pragma once
#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QLineEdit>
#include "starlib_global.h"
#include "RichParameter.h"

class STARLIB_EXPORT RichParameterWidget : public QWidget {
    Q_OBJECT
protected:
    /// Parameter we are creating a widget for
    RichParameter* richParameter;
    /// @todo why do we need to store this?
    QLabel* helpLabel;
    
public:
    ~RichParameterWidget(){ delete helpLabel; }
    RichParameterWidget(QWidget* parent, RichParameter* richParameter)
        : QWidget(parent){
        this->richParameter = richParameter;
        
        assert(parent != nullptr);
        if (richParameter!= nullptr) {
            helpLabel = new QLabel("<small>"+richParameter->tooltip +"</small>",parent);
            helpLabel->setTextFormat(Qt::RichText);
            helpLabel->setWordWrap(true);
            helpLabel->setVisible(false);
            helpLabel->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
            helpLabel->setMinimumWidth(250);
            helpLabel->setMaximumWidth(QWIDGETSIZE_MAX);
            gridLay = qobject_cast<QGridLayout*>(parent->layout());
            assert(gridLay != nullptr);
    
            row = gridLay->rowCount();
            //WARNING!!!!!!!!!!!!!!!!!! HORRIBLE PATCH FOR THE BOOL WIDGET PROBLEM
            if ((row == 1) && (richParameter->val->isBool())) {
                QLabel* lb = new QLabel("",parent);
                gridLay->addWidget(lb);
                gridLay->addWidget(helpLabel,row+1,3,1,1,Qt::AlignTop);
            }
            ///////////////////////////////////////////////////////////////////////
            else
                gridLay->addWidget(helpLabel,row,3,1,1,Qt::AlignTop);
        }
    }

    /// Restore gui values with defaults stored in RichParameter
    virtual void resetWidgetValue() = 0;
    /// Reads the value from the gui and stores it in "this->rp"
    virtual void collectWidgetValue() = 0;
    /// @todo What is this?
    virtual void setWidgetValue(const RichParameter::Value& nv) = 0;
    
signals:
    void parameterChanged();

protected:
    void InitRichParameter(RichParameter* rpar);
    int row;
    QGridLayout* gridLay;
};
