#pragma once

#include <string>
#include <vector>
#include <stdint.h>
#include <functional>
#include <list>
#include "CJsonObject.h"

class ThingModel;

class QiotData {

public:
    QiotData(const char *id, int type);
    QiotData(std::string &id, int type);

    bool SetValue(uint32_t val);
    bool SetValue(const char *str);
    bool SetValue(const char *dat, int len);
    bool SetValue(unsigned int index, uint32_t val);
    bool SetValue(unsigned int index, const char *val, int len);

    int GetValue(uint8_t index, char *buf, uint16_t buf_len);
    int GetValue(char *buf, uint16_t buf_len);
    QiotData *GetChildCtx(uint8_t index);

    uint8_t GetType();
    unsigned int ChildsCount() {
        return _childs.size();
    }
    const char *ID() {
        return _id.c_str();
    }

    // for debug
    void Dump(const char *preFormat = "");

private:
    std::string _id;
    int _type;
    uint32_t _val;
    std::vector<char> _strVal;
    std::vector<QiotData> _childs;

private:
    friend class ThingModel;
};

typedef std::function<void(QiotData&)> QiotDataHandler;

class ThingModel
{

public:
    ThingModel()
        : _valid(false),
          _properties(NULL),
          _events(NULL),
          _actions(NULL)
    {}
    ~ThingModel() {
        if (_properties)
            delete _properties;
        if (_events)
            delete _events;
        if (_actions)
            delete _actions;
    }

    bool Load(const char *jsonStr);
    bool Valid() {
        return _valid;
    }

    uint8_t GetPropertyType(uint8_t index);
    QiotData *GetPropertyCtx(uint8_t index);
    bool ReportProperty(const char *id, const char *val);
    void AddPropertyHandler(QiotDataHandler *handler) {
        _propertiesHandler.push_back(handler);
    }
    void RemovePropertyHandler(QiotDataHandler *handler) {
        _propertiesHandler.remove(handler);
    }
    unsigned int PropertiesSize() {
        if (_properties)
            return _properties->ChildsCount();
        return 0;
    }
    QiotData *GetEventCtx(uint8_t index);
    QiotData *GetActionInputCtx(uint8_t index);
    QiotData *GetActionOutputCtx(uint8_t index);

    // for debug
    void Dump();
    // for internal
    void PropertyNotify(QiotData &property) {
        for (auto hander : _propertiesHandler)
            (*hander)(property);
    }
    void ActionsNotify(uint8_t index, uint8_t output_flag[]) {
    }

private:
    bool PropertyValid(uint8_t index);

private:
    bool ParseProperties(neb::CJsonObject &properties);
    bool ParseEvents(neb::CJsonObject &events);
    bool ParseActions(neb::CJsonObject &actions);
    bool ParseStruct(neb::CJsonObject &Struct, QiotData &dat, const char *typeKey, bool noArray);
    bool ParseArray(neb::CJsonObject &array, QiotData &dat, const char *typeKey, bool noStruct);

private:
    bool _valid;
    QiotData *_properties;
    std::list<QiotDataHandler*> _propertiesHandler;
    QiotData *_events;
    QiotData *_actions;
    std::list<QiotDataHandler*> _actionsHandler;

private:
    ThingModel(ThingModel&) = delete;
    void operator=(ThingModel&) = delete;
};
