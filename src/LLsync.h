#pragma once

#include <stdint.h>
#include <functional>
#include <string>
#include <list>
#include "core/ble_qiot_export.h"
#include "ThingModel.h"
#include "Log.h"

class LLsync
{
public:
    enum Event {
        CONNECT,
        DISCONNECT,
        OTA_START,
        OTA_SUCCESS,
        OTA_FAIL
    };
    typedef std::function<void(int)> EventHandler;

public:
    static LLsync *GetInstance(void) {
        static LLsync *p = NULL;
        if (!p)
            p = new LLsync();
        return p;
    }
    void Start();
    void Stop();
    void AddEventHandler(EventHandler *handler) {
        _handles.push_back(handler);
    }
    void RemoveEventHandler(EventHandler *handle) {
        _handles.remove(handle);
    }
    ThingModel &thingModel() {
        return _thingModel;
    }

    void set_product_id(const char *product_id) {
        _productID = product_id;
    }
    void set_device_name(const char *device_name) {
        _deviceName = device_name;
    }
    void set_device_secret(const char *device_secret) {
        _deviceSecret = device_secret;
    }
    void set_product_secret(const char *product_secret) {
        _productSecret = product_secret;
    }

// for internal
    std::string& get_product_id() {
        return _productID;
    }
    std::string& get_device_name() {
        return _deviceName;
    }
    std::string& get_device_secret() {
        return _deviceSecret;
    }
    std::string& get_product_secret() {
        return _productSecret;
    }

    void EventNotify(Event event) {
        for (auto h : _handles)
            (*h)(event);
    }

private:
    static void ota_start_cb();
    static void ota_stop_cb(uint8_t result);
    static ble_qiot_ret_status_t ota_valid_file_cb(uint32_t file_size, char *file_version);

private:
    std::string _productID;
    std::string _deviceName;
    std::string _deviceSecret;
    std::string _productSecret;
    ThingModel _thingModel;
    std::list<EventHandler*> _handles;
    bool _running;

private:
    LLsync()
        : _running(false)
    {}
    ~LLsync() {}
    LLsync(LLsync&) = delete;
    void operator=(LLsync&) = delete;
};
