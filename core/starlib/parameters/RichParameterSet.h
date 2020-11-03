#pragma once
#include <QList> // Used to stores the set of parameters
#include <QObject>

#include "starlib_global.h"
#include "RichParameter.h"
#include "RichFloat.h"
#include "RichBool.h"
#include "RichInt.h"
#include "RichString.h"
#include "RichColor.h"
#include "RichStringSet.h"

class STARLIB_EXPORT RichParameterSet : public QObject, public RichParameter{
    Q_OBJECT
    
private:
    Q_DISABLE_COPY(RichParameterSet)
    
    /// @{ constructors/destructors
public:
    RichParameterSet(QObject* parent = nullptr) : QObject(parent){}
    RichParameterSet(QString _name, QObject* parent = nullptr) : QObject(parent), RichParameter(_name){}
    ~RichParameterSet();
    /// @}

public:
    // 参数列表
    QList<RichParameter*> paramList;

    bool isEmpty() const;
    RichParameter* findParameter(QString name) const;
    bool hasParameter(QString name) const;

    /// Generates a string representation of the instance
    QString toString(QString prefix=QString());
    
    bool operator==(const RichParameterSet& rps);

    RichParameterSet& addParam(RichParameter* pd);

    //remove a parameter from the set by name
    RichParameterSet& removeParameter(QString name);

    void clear();

    /// General setter (require you to instantiate a Value type)
    void setValue(const QString name,const Value& val);
    
    bool getBool(QString name) const;
    void setValue(QString name, bool newval);

    int    getInt(QString name) const;
    void setValue(QString name, int newval);

    float getFloat(QString name) const;
    void setValue(QString name, float newval);

    QString getString(QString name) const;
    void setValue(QString name, const QString& newval);

    QColor getColor(QString name) const;
    void setValue(QString name, const QColor& newval);
};
