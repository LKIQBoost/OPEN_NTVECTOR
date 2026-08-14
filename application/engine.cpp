// engine.cpp
#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine.h"  // 锟斤拷锟斤拷头锟侥硷拷
#include "Logger.h"
#include "StringSplitUtils.h"
#include "ConnectInstance.h"
#include "PacketCommon.h"
#include "ClientInstance.h"
#include "ConfigLoader.h"
#include "Py3Compat.h"
#include "InventoryTransaction.h"
#include "ContainerOpen.h"
#include "InventoryContent.h"
#include "BlockActorData.h"
#include "BlockDataStore.h"
#include "SubChunkClient.h"
#include <json/json.h>
#include <map>

extern ClientInstance* g_client_instance;

#ifdef Linker_NtUniSdk
#include "NtUniSdkBase.h"
#include "MPayDelegate.h"

extern NtUniSDK::INtUniSdkGamerInterface* ctx;
std::string str;
#endif // !Linker_NtUniSdk
// 锟铰硷拷锟斤拷锟斤拷锟斤拷锟侥结构
typedef struct {
    PyObject* callback;
    char* event_name;
    long callback_id;  // 锟截碉拷锟斤拷锟斤拷锟斤拷唯一ID锟斤拷锟节达拷锟街凤拷锟?
    bool is_kernel_event; // 锟角凤拷为锟节猴拷锟铰硷拷锟斤拷锟斤拷锟解处锟斤拷锟斤拷锟斤拷止锟斤拷锟角ｏ拷
} EventHandler;

// 全锟斤拷锟铰硷拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
static EventHandler* event_handlers = NULL;
static int handler_count = 0;
static int handler_capacity = 0;

// 锟叫讹拷锟斤拷锟斤拷锟缴碉拷锟矫讹拷锟斤拷锟角凤拷锟斤拷锟?
static bool are_callables_equal(PyObject* callback1, PyObject* callback2) {
    // 1. 锟饺比斤拷指锟诫（锟斤拷虻サ锟斤拷锟斤拷锟斤拷
    if (callback1 == callback2) {
        return true;
    }

    // 2. 锟饺较猴拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
    if (PyFunction_Check(callback1) && PyFunction_Check(callback2)) {
        // 锟斤拷取锟斤拷锟斤拷锟斤拷
        PyObject* name1 = PyObject_GetAttrString(callback1, "__name__");
        PyObject* name2 = PyObject_GetAttrString(callback2, "__name__");

        // 锟斤拷取锟斤拷锟斤拷锟斤拷锟?
        PyObject* code1 = PyObject_GetAttrString(callback1, "__code__");
        PyObject* code2 = PyObject_GetAttrString(callback2, "__code__");

        bool equal = false;
        if (name1 && name2 && code1 && code2) {
            // 锟饺较猴拷锟斤拷锟斤拷
            if (PyObject_RichCompareBool(name1, name2, Py_EQ) == 1) {
                // 锟饺较达拷锟斤拷锟斤拷锟斤拷锟侥硷拷锟斤拷锟斤拷锟斤拷一锟叫号碉拷
                PyObject* filename1 = PyObject_GetAttrString(code1, "co_filename");
                PyObject* filename2 = PyObject_GetAttrString(code2, "co_filename");
                PyObject* firstlineno1 = PyObject_GetAttrString(code1, "co_firstlineno");
                PyObject* firstlineno2 = PyObject_GetAttrString(code2, "co_firstlineno");

                if (filename1 && filename2 && firstlineno1 && firstlineno2) {
                    equal = (PyObject_RichCompareBool(filename1, filename2, Py_EQ) == 1) &&
                        (PyObject_RichCompareBool(firstlineno1, firstlineno2, Py_EQ) == 1);
                }

                Py_XDECREF(filename1);
                Py_XDECREF(filename2);
                Py_XDECREF(firstlineno1);
                Py_XDECREF(firstlineno2);
            }
        }

        Py_XDECREF(name1);
        Py_XDECREF(name2);
        Py_XDECREF(code1);
        Py_XDECREF(code2);

        return equal;
    }

    return false;
}

// 锟斤拷取锟截碉拷锟斤拷锟斤拷锟斤拷唯一锟斤拷识锟斤拷使锟斤拷锟节达拷锟街凤拷锟?
static long get_callback_id(PyObject* callback) {
    return (long)callback;
}

// 锟斤拷楹拷锟斤拷欠锟斤拷锟斤拷锟斤拷锟斤拷一锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟?
static int check_function_accepts_args(PyObject* callback) {
    PyObject* func_code = PyObject_GetAttrString(callback, "__code__");
    if (!func_code) {
        return 0;
    }

    PyObject* co_argcount = PyObject_GetAttrString(func_code, "co_argcount");
    if (!co_argcount) {
        Py_DECREF(func_code);
        return 0;
    }

    int argcount = PyLong_AsLong(co_argcount);
    Py_DECREF(co_argcount);
    Py_DECREF(func_code);

    return argcount >= 1;
}

// 注锟斤拷锟铰硷拷锟侥猴拷锟斤拷锟斤拷支锟街讹拷锟斤拷锟斤拷锟阶拷岬酵伙拷录锟斤拷锟?
static PyObject* engine_register(PyObject* self, PyObject* args) {
    PyObject* callback;
    char* event_name;

    // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷录锟斤拷锟斤拷锟斤拷址锟斤拷锟?
    if (!PyArg_ParseTuple(args, "Os", &callback, &event_name)) {
        return NULL;
    }

    // 锟斤拷锟斤拷一锟斤拷锟斤拷锟斤拷锟角凤拷为锟缴碉拷锟矫讹拷锟斤拷
    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "The first argument must be a callable object");
        return NULL;
    }

    // 锟斤拷楹拷锟斤拷欠锟斤拷锟斤拷锟斤拷锟斤拷一锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟?
    if (!check_function_accepts_args(callback)) {
        PyErr_SetString(PyExc_TypeError, "The registered function must accept at least one argument.");
        return NULL;
    }

    // 锟斤拷锟斤拷欠锟斤拷丫锟阶拷锟斤拷锟斤拷同锟侥回碉拷锟斤拷锟斤拷锟斤拷同一锟铰硷拷
    for (int i = 0; i < handler_count; i++) {
        if (strcmp(event_handlers[i].event_name, event_name) == 0) {
            if (are_callables_equal(callback, event_handlers[i].callback)) {
                // 锟斤拷同锟侥回碉拷锟斤拷锟斤拷锟窖撅拷注锟结到同一锟铰硷拷锟斤拷锟饺斤拷注锟缴碉拷锟斤拷注锟斤拷锟铰的ｏ拷锟斤拷锟角ｏ拷
                Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] Info: The same callback function has already been registered to the event '" + event_name + "', perform overwrite operation");

                // 锟酵放旧碉拷锟斤拷源
                Py_DECREF(event_handlers[i].callback);
                free(event_handlers[i].event_name);

                // 锟斤拷锟斤拷锟铰回碉拷锟斤拷锟斤拷锟矫硷拷锟斤拷
                Py_INCREF(callback);

                // 锟斤拷锟斤拷锟铰硷拷锟斤拷锟斤拷锟斤拷
                event_handlers[i].callback = callback;
                event_handlers[i].event_name = strdup(event_name);
                event_handlers[i].callback_id = get_callback_id(callback);

                if (!event_handlers[i].event_name) {
                    Py_DECREF(callback);
                    PyErr_NoMemory();
                    return NULL;
                }

                PyObject* repr_str = PyObject_Repr(callback);
                std::string c_str = repr_str ? (PyUnicode_AsUTF8(repr_str) ? PyUnicode_AsUTF8(repr_str) : "") : "";
                // 锟斤拷锟斤拷 c_str 锟斤拷锟角ｏ拷
                // <common.system.eventHandler.EventHandler object at 0x12345678>
                Py_DECREF(repr_str);
                Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] RegisterEngineHandler: Minecraft:Engine:" + event_name +
                    " "+c_str+" (Coverage)");
                Logger::getInstance().log(LOG_WARN, std::string() + " *********************** register_event_id ************************ BusID: 0, EventID: " +
                    std::to_string(i + 1) + " (Overwrite update)");

                Py_RETURN_NONE;
            }
        }
    }

    // 锟斤拷锟斤拷锟斤拷锟矫硷拷锟斤拷锟斤拷锟斤拷止锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
    Py_INCREF(callback);

    // 锟斤拷展锟铰硷拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
    if (handler_count >= handler_capacity) {
        int new_capacity = handler_capacity == 0 ? 10 : handler_capacity * 2;
        EventHandler* new_handlers = (EventHandler*)realloc(event_handlers,
            new_capacity * sizeof(EventHandler));
        if (!new_handlers) {
            Py_DECREF(callback);
            PyErr_NoMemory();
            return NULL;
        }
        event_handlers = new_handlers;
        handler_capacity = new_capacity;
    }

    // 锟芥储锟铰硷拷锟斤拷锟斤拷锟斤拷
    event_handlers[handler_count].callback = callback;
    event_handlers[handler_count].event_name = strdup(event_name);
    event_handlers[handler_count].callback_id = get_callback_id(callback);

    if (!event_handlers[handler_count].event_name) {
        Py_DECREF(callback);
        PyErr_NoMemory();
        return NULL;
    }

    int new_handler_index = handler_count;
    handler_count++;

    // 统锟狡革拷锟铰硷拷锟斤拷注锟结函锟斤拷锟斤拷锟斤拷
    int event_handler_count = 0;
    for (int i = 0; i < handler_count; i++) {
        if (strcmp(event_handlers[i].event_name, event_name) == 0) {
            event_handler_count++;
        }
    }

    PyObject* repr_str = PyObject_Repr(callback);
    std::string c_str = repr_str ? (PyUnicode_AsUTF8(repr_str) ? PyUnicode_AsUTF8(repr_str) : "") : "";
    // 锟斤拷锟斤拷 c_str 锟斤拷锟角ｏ拷
    // <common.system.eventHandler.EventHandler object at 0x12345678>
    Py_DECREF(repr_str);
    Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] RegisterEngineHandler: Minecraft:Engine:" + event_name +
        " "+c_str);
    Logger::getInstance().log(LOG_WARN, std::string() + " *********************** register_event_id ************************ BusID: 0, EventID: " +
        std::to_string(new_handler_index + 1) + ", EventHandlers: " + std::to_string(event_handler_count));

    Py_RETURN_NONE;
}
// 注锟斤拷锟铰硷拷锟侥猴拷锟斤拷锟斤拷支锟街讹拷锟斤拷锟斤拷锟阶拷岬酵伙拷录锟斤拷锟?
static PyObject* register_kernel_event(PyObject* self, PyObject* args) {
    PyObject* callback;
    char* event_name;

    // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷录锟斤拷锟斤拷锟斤拷址锟斤拷锟?
    if (!PyArg_ParseTuple(args, "Os", &callback, &event_name)) {
        return NULL;
    }

    // 锟斤拷锟斤拷一锟斤拷锟斤拷锟斤拷锟角凤拷为锟缴碉拷锟矫讹拷锟斤拷
    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "The first argument must be a callable object");
        return NULL;
    }

    // 锟斤拷楹拷锟斤拷欠锟斤拷锟斤拷锟斤拷锟斤拷一锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟?
    if (!check_function_accepts_args(callback)) {
        PyErr_SetString(PyExc_TypeError, "The registered function must accept at least one argument.");
        return NULL;
    }

    // 锟斤拷锟斤拷欠锟斤拷丫锟阶拷锟斤拷锟斤拷同锟侥回碉拷锟斤拷锟斤拷锟斤拷同一锟铰硷拷
    for (int i = 0; i < handler_count; i++) {
        if (strcmp(event_handlers[i].event_name, event_name) == 0) {
            if (are_callables_equal(callback, event_handlers[i].callback)) {
                // 锟斤拷同锟侥回碉拷锟斤拷锟斤拷锟窖撅拷注锟结到同一锟铰硷拷锟斤拷锟饺斤拷注锟缴碉拷锟斤拷注锟斤拷锟铰的ｏ拷锟斤拷锟角ｏ拷
                Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] Info: The same callback function has already been registered to the event '" + event_name + "', perform overwrite operation");

                // 锟酵放旧碉拷锟斤拷源
                Py_DECREF(event_handlers[i].callback);
                free(event_handlers[i].event_name);

                // 锟斤拷锟斤拷锟铰回碉拷锟斤拷锟斤拷锟矫硷拷锟斤拷
                Py_INCREF(callback);

                // 锟斤拷锟斤拷锟铰硷拷锟斤拷锟斤拷锟斤拷
                event_handlers[i].callback = callback;
                event_handlers[i].event_name = strdup(event_name);
                event_handlers[i].callback_id = get_callback_id(callback);

                if (!event_handlers[i].event_name) {
                    Py_DECREF(callback);
                    PyErr_NoMemory();
                    return NULL;
                }

                PyObject* repr_str = PyObject_Repr(callback);
                std::string c_str = repr_str ? (PyUnicode_AsUTF8(repr_str) ? PyUnicode_AsUTF8(repr_str) : "") : "";
                // 锟斤拷锟斤拷 c_str 锟斤拷锟角ｏ拷
                // <common.system.eventHandler.EventHandler object at 0x12345678>
                Py_DECREF(repr_str);
                Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] RegisterEngineHandler: Minecraft:Engine:" + event_name +
                    " "+c_str+" (Coverage)");
                Logger::getInstance().log(LOG_WARN, std::string() + " *********************** register_event_id ************************ BusID: 0, EventID: " +
                    std::to_string(i + 1) + " (Overwrite update)");

                Py_RETURN_NONE;
            }
        }
    }

    // 锟斤拷锟斤拷锟斤拷锟矫硷拷锟斤拷锟斤拷锟斤拷止锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
    Py_INCREF(callback);

    // 锟斤拷展锟铰硷拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
    if (handler_count >= handler_capacity) {
        int new_capacity = handler_capacity == 0 ? 10 : handler_capacity * 2;
        EventHandler* new_handlers = (EventHandler*)realloc(event_handlers,
            new_capacity * sizeof(EventHandler));
        if (!new_handlers) {
            Py_DECREF(callback);
            PyErr_NoMemory();
            return NULL;
        }
        event_handlers = new_handlers;
        handler_capacity = new_capacity;
    }

    // 锟芥储锟铰硷拷锟斤拷锟斤拷锟斤拷
    event_handlers[handler_count].callback = callback;
    event_handlers[handler_count].event_name = strdup(event_name);
    event_handlers[handler_count].callback_id = get_callback_id(callback);
    event_handlers[handler_count].is_kernel_event = true; // 锟斤拷锟轿拷诤锟斤拷录锟?

    if (!event_handlers[handler_count].event_name) {
        Py_DECREF(callback);
        PyErr_NoMemory();
        return NULL;
    }

    int new_handler_index = handler_count;
    handler_count++;

    // 统锟狡革拷锟铰硷拷锟斤拷注锟结函锟斤拷锟斤拷锟斤拷
    int event_handler_count = 0;
    for (int i = 0; i < handler_count; i++) {
        if (strcmp(event_handlers[i].event_name, event_name) == 0) {
            event_handler_count++;
        }
    }

    PyObject* repr_str = PyObject_Repr(callback);
    std::string c_str = repr_str ? (PyUnicode_AsUTF8(repr_str) ? PyUnicode_AsUTF8(repr_str) : "") : "";
    // 锟斤拷锟斤拷 c_str 锟斤拷锟角ｏ拷
    // <common.system.eventHandler.EventHandler object at 0x12345678>
    Py_DECREF(repr_str);
    Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] RegisterEngineHandler: Minecraft:Engine:" + event_name +
        " "+ c_str);
    Logger::getInstance().log(LOG_WARN, std::string() + " *********************** register_event_id ************************ BusID: 0, EventID: " +
        std::to_string(new_handler_index + 1) + ", EventHandlers: " + std::to_string(event_handler_count));

    Py_RETURN_NONE;
}

// 锟斤拷注锟斤拷锟铰硷拷锟侥猴拷锟斤拷
static PyObject* engine_unregister(PyObject* self, PyObject* args) {
    PyObject* callback = NULL;
    char* event_name = NULL;

    // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟铰硷拷锟斤拷锟斤拷锟街凤拷锟斤拷锟酵匡拷选锟侥猴拷锟斤拷锟斤拷锟斤拷
    if (!PyArg_ParseTuple(args, "s|O", &event_name, &callback)) {
        return NULL;
    }

    // 锟斤拷锟矫伙拷锟斤拷峁╋拷氐锟斤拷锟斤拷锟斤拷锟斤拷锟缴撅拷锟斤拷锟斤拷录锟斤拷锟斤拷锟斤拷写锟斤拷锟斤拷锟?
    if (callback == NULL) {
        int removed_count = 0;

        for (int i = 0; i < handler_count; i++) {
            if (strcmp(event_handlers[i].event_name, event_name) == 0) {
                // 锟酵凤拷锟斤拷源
                Py_DECREF(event_handlers[i].callback);
                free(event_handlers[i].event_name);

                // 锟斤拷锟斤拷锟揭伙拷锟皆拷锟斤拷贫锟斤拷锟斤拷锟角拔伙拷锟?
                if (i < handler_count - 1) {
                    event_handlers[i] = event_handlers[handler_count - 1];
                }

                handler_count--;
                i--; // 锟斤拷锟铰硷拷榈鼻拔伙拷茫锟斤拷锟轿拷乇锟斤拷贫锟斤拷耍锟?
                removed_count++;
            }
        }

        if (removed_count > 0) {
            Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] Success from events '" + event_name +
                "' Remove from " + std::to_string(removed_count) + " processor");
            Py_RETURN_TRUE;
        }
        else {
            Logger::getInstance().log(LOG_WARN, std::string() + "[Engine] Warning: Event '" + event_name +
                "' No processor found");
            Py_RETURN_FALSE;
        }
    }

    // 锟斤拷锟斤拷峁╋拷嘶氐锟斤拷锟斤拷锟斤拷锟街簧撅拷锟狡ワ拷锟侥达拷锟斤拷锟斤拷
    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "The second argument must be a callable object");
        return NULL;
    }

    for (int i = 0; i < handler_count; i++) {
        if (strcmp(event_handlers[i].event_name, event_name) == 0 &&
            are_callables_equal(callback, event_handlers[i].callback)) {

            PyObject* repr_str = PyObject_Repr(callback);
            std::string c_str = repr_str ? (PyUnicode_AsUTF8(repr_str) ? PyUnicode_AsUTF8(repr_str) : "") : "";
            // 锟斤拷锟斤拷 c_str 锟斤拷锟角ｏ拷
            // <common.system.eventHandler.EventHandler object at 0x12345678>
            Py_DECREF(repr_str);
            // 锟斤拷录锟斤拷志
            Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] UnregisterEngineHandler: Minecraft:Engine:" + event_name +
                " "+ c_str);

            // 锟酵凤拷锟斤拷源
            Py_DECREF(event_handlers[i].callback);
            free(event_handlers[i].event_name);

            // 锟斤拷锟斤拷锟揭伙拷锟皆拷锟斤拷贫锟斤拷锟斤拷锟角拔伙拷锟?
            if (i < handler_count - 1) {
                event_handlers[i] = event_handlers[handler_count - 1];
            }

            handler_count--;

            Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] Success from events '" + event_name + "' Remove the specified processor");
            Py_RETURN_TRUE;
        }
    }

    Logger::getInstance().log(LOG_WARN, std::string() + "[Engine] Warning: Not in event '" + event_name +
        "' Find the specified processor");
    Py_RETURN_FALSE;
}
/*
// 取锟斤拷注锟斤拷锟铰硷拷锟侥猴拷锟斤拷
static PyObject* engine_unregister(PyObject* self, PyObject* args) {
    PyObject* callback;
    char* event_name;

    if (!PyArg_ParseTuple(args, "Os", &callback, &event_name)) {
        Py_RETURN_NONE;
    }

    int found_index = -1;
    for (int i = 0; i < handler_count; i++) {
        if (strcmp(event_handlers[i].event_name, event_name) == 0) {
            if (are_callables_equal(callback, event_handlers[i].callback)) {
                found_index = i;
                break;
            }
        }
    }

    if (found_index != -1) {
        // 锟揭碉拷匹锟斤拷锟斤拷录锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷瞥锟斤拷锟?
        Py_DECREF(event_handlers[found_index].callback);
        free(event_handlers[found_index].event_name);

        // 锟斤拷锟斤拷锟斤拷锟叫碉拷元锟斤拷锟斤拷前锟狡讹拷
        for (int i = found_index; i < handler_count - 1; i++) {
            event_handlers[i] = event_handlers[i + 1];
        }

        handler_count--;

        Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] Unregister Event: " + event_name +
            ", Number of remaining handlers: " + std::to_string(handler_count));
    }
    else {
        Logger::getInstance().log(LOG_WARN, std::string() + "[Engine] Warning: No event handler found to unregister '" + event_name + "'");
    }

    Py_RETURN_NONE;
}
*/
// 锟斤拷锟斤拷录锟斤拷欠锟斤拷锟阶拷锟?
static PyObject* engine_is_registered(PyObject* self, PyObject* args) {
    PyObject* callback;
    char* event_name;

    if (!PyArg_ParseTuple(args, "Os", &callback, &event_name)) {
        return NULL;
    }

    for (int i = 0; i < handler_count; i++) {
        if (strcmp(event_handlers[i].event_name, event_name) == 0) {
            if (are_callables_equal(callback, event_handlers[i].callback)) {
                Py_RETURN_TRUE;
            }
        }
    }

    Py_RETURN_FALSE;
}

// 锟斤拷取指锟斤拷锟铰硷拷锟斤拷注锟结函锟斤拷锟斤拷锟斤拷
static PyObject* engine_get_event_handler_count(PyObject* self, PyObject* args) {
    char* event_name;

    if (!PyArg_ParseTuple(args, "s", &event_name)) {
        return NULL;
    }

    int count = 0;
    for (int i = 0; i < handler_count; i++) {
        if (strcmp(event_handlers[i].event_name, event_name) == 0) {
            count++;
        }
    }

    return PyLong_FromLong(count);
}

// C++锟姐触锟斤拷锟铰硷拷锟侥猴拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷C++使锟矫ｏ拷
void trigger_event(const char* event_name, PyObject* args_array) {
    Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] do cpp Engine Wrapper Event: " + event_name);
    //PyGILState_STATE oldstate = PyGILState_Ensure();
    if (!event_handlers || !args_array) return;

    // 确锟斤拷args_array锟斤拷锟叫憋拷锟斤拷元锟斤拷
    if (!PyList_Check(args_array) && !PyTuple_Check(args_array)) {
        Logger::getInstance().log(LOG_ERROR, "[Engine] The parameter must be a list or a tuple");
        return;
    }

    int triggered_count = 0;
    for (int i = 0; i < handler_count; i++) {
        if (strcmp(event_handlers[i].event_name, event_name) == 0) {
            // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷元锟介传锟捷革拷锟截碉拷锟斤拷锟斤拷
            PyObject* arg_tuple = PyTuple_New(1);
            Py_INCREF(args_array); // 锟斤拷锟斤拷锟斤拷锟矫硷拷锟斤拷
            PyTuple_SetItem(arg_tuple, 0, args_array);
            PyObject* result;
            try {
                result = PyObject_CallObject(event_handlers[i].callback, arg_tuple);
                Py_DECREF(arg_tuple);

                if (result == NULL) {
                    PyErr_Print(); // 锟斤拷印Python锟届常
                }
                else {
                    Py_DECREF(result);
                }
            }
            catch (...) {
                Logger::getInstance().log(LOG_ERROR, "python call object error.");
                PyErr_Print(); // 锟斤拷印Python锟届常
            }
            triggered_count++;
        }
    }

    if (triggered_count > 0) {
        Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] Event Triggered: " + event_name +
            ", Number of remaining handlers: " + std::to_string(triggered_count));
    }
    else {
        Logger::getInstance().log(LOG_WARN, std::string() + "[Engine] Warning: Event '" + event_name + "' has no registered handler");
    }

    //PyGILState_Release(oldstate);
}

// 锟斤拷锟斤拷锟铰硷拷锟斤拷锟斤拷锟捷讹拷锟斤拷锟斤拷锟斤拷锟斤拷远锟斤拷锟斤拷锟斤拷锟斤拷锟介）
void trigger_event_with_args(const char* event_name, int arg_count, ...) {
    if (!event_handlers) return;

    PyGILState_STATE gstate = PyGILState_Ensure();

    // 锟斤拷锟斤拷锟斤拷锟斤拷锟叫憋拷
    PyObject* args_list = PyList_New(arg_count);
    if (!args_list) {
        PyGILState_Release(gstate);
        return;
    }

    va_list vl;
    va_start(vl, arg_count);

    for (int i = 0; i < arg_count; i++) {
        PyObject* arg = va_arg(vl, PyObject*);
        Py_INCREF(arg); // 锟斤拷锟斤拷锟斤拷锟矫硷拷锟斤拷
        PyList_SetItem(args_list, i, arg);
    }

    va_end(vl);

    // 锟斤拷锟斤拷锟铰硷拷
    int triggered_count = 0;
    for (int i = 0; i < handler_count; i++) {
        if (strcmp(event_handlers[i].event_name, event_name) == 0) {
            PyObject* arg_tuple = PyTuple_New(1);
            if (arg_tuple) {
                Py_INCREF(args_list);
                PyTuple_SetItem(arg_tuple, 0, args_list);

                PyObject* result = PyObject_CallObject(event_handlers[i].callback, arg_tuple);
                Py_DECREF(arg_tuple);

                if (result == NULL) {
                    PyErr_Print();
                }
                else {
                    Py_DECREF(result);
                }

                triggered_count++;
            }
        }
    }

    Py_DECREF(args_list);
    PyGILState_Release(gstate);

    Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] Unregister Event: " + event_name +
        ", Number of remaining handlers: " + std::to_string(triggered_count));
}

// 锟斤拷锟斤拷锟矫的达拷锟斤拷锟斤拷锟斤拷锟斤拷Python锟缴碉拷锟矫ｏ拷
static PyObject* engine_trigger(PyObject* self, PyObject* args) {
    char* event_name;
    PyObject* args_array = NULL;

    if (!PyArg_ParseTuple(args, "s|O", &event_name, &args_array)) {
        return NULL;
    }

    if (args_array == NULL) {
        // 锟斤拷锟矫伙拷锟斤拷峁╋拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟?
        args_array = PyList_New(0);
    }
    else if (!PyList_Check(args_array) && !PyTuple_Check(args_array)) {
        // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷斜锟斤拷锟皆拷椋拷锟斤拷锟斤拷装锟斤拷锟叫憋拷
        PyObject* temp = args_array;
        args_array = PyList_New(1);
        if (args_array) {
            Py_INCREF(temp);
            PyList_SetItem(args_array, 0, temp);
        }
    }
    else {
        Py_INCREF(args_array);
    }

    if (args_array) {
        trigger_event(event_name, args_array);
        Py_DECREF(args_array);
    }

    Py_RETURN_NONE;
}

// 锟斤拷取锟斤拷注锟斤拷锟铰硷拷锟斤拷锟斤拷锟侥猴拷锟斤拷
static PyObject* engine_get_handler_count(PyObject* self, PyObject* args) {
    return PyLong_FromLong(handler_count);
}

// 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷止锟节达拷泄漏锟斤拷
static PyObject* engine_cleanup(PyObject* self, PyObject* args) {
    for (int i = 0; i < handler_count; i++) {
        Py_DECREF(event_handlers[i].callback);
        free(event_handlers[i].event_name);
    }
    free(event_handlers);
    event_handlers = NULL;
    handler_count = 0;
    handler_capacity = 0;

    Logger::getInstance().log(LOG_INFO, "Clear all event handlers.");
    Py_RETURN_NONE;
}
// 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷止锟节达拷泄漏锟斤拷
static PyObject* engine_user_cleanup(PyObject* self, PyObject* args) {
    // 1. 锟斤拷锟斤拷锟介保锟斤拷
    if (event_handlers == NULL || handler_count == 0) {
        Logger::getInstance().log(LOG_INFO, "No user event handlers to clear.");
        Py_RETURN_NONE;
    }

    // 2. 锟斤拷锟斤拷锟斤拷锟介，锟斤拷锟斤拷锟节猴拷锟铰硷拷锟斤拷源 + 锟窖憋拷锟斤拷锟斤拷元锟斤拷前锟狡ｏ拷锟斤拷锟斤拷锟秸讹拷锟斤拷
    int new_count = 0; // 锟斤拷录锟斤拷锟斤拷锟斤拷元锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟节猴拷锟铰硷拷锟斤拷
    for (int i = 0; i < handler_count; i++) {
        if (!event_handlers[i].is_kernel_event) {
            // 锟斤拷锟斤拷锟节猴拷锟铰硷拷锟斤拷源
            if (event_handlers[i].callback != NULL) {
                Py_DECREF(event_handlers[i].callback);
                event_handlers[i].callback = NULL;
            }
            if (event_handlers[i].event_name != NULL) {
                free(event_handlers[i].event_name);
                event_handlers[i].event_name = NULL;
            }
        }
        else {
            // 锟斤拷锟节猴拷锟铰硷拷锟斤拷前锟狡碉拷锟斤拷位锟矫ｏ拷锟斤拷锟斤拷锟秸讹拷
            event_handlers[new_count] = event_handlers[i];
            new_count++;
        }
    }

    // 3. 锟斤拷锟斤拷锟斤拷效元锟斤拷锟斤拷锟斤拷锟斤拷锟秸讹拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
    handler_count = new_count;

    Logger::getInstance().logv(LOG_INFO, "Clear all user event handlers, total remaining handlers: %d", handler_count);
    Py_RETURN_NONE;
}
// 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷止锟节达拷泄漏 + 锟斤拷锟斤拷锟斤拷锟斤拷斩锟斤拷锟?
static PyObject* engine_kernel_cleanup(PyObject* self, PyObject* args) {
    // 1. 锟斤拷锟斤拷锟介保锟斤拷
    if (event_handlers == NULL || handler_count == 0) {
        Logger::getInstance().log(LOG_INFO, "No kernel event handlers to clear.");
        Py_RETURN_NONE;
    }

    // 2. 锟斤拷锟斤拷锟斤拷锟介，锟斤拷锟斤拷锟节猴拷锟铰硷拷锟斤拷源 + 锟窖憋拷锟斤拷锟斤拷元锟斤拷前锟狡ｏ拷锟斤拷锟斤拷锟秸讹拷锟斤拷
    int new_count = 0; // 锟斤拷录锟斤拷锟斤拷锟斤拷元锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟节猴拷锟铰硷拷锟斤拷
    for (int i = 0; i < handler_count; i++) {
        if (event_handlers[i].is_kernel_event) {
            // 锟斤拷锟斤拷锟节猴拷锟铰硷拷锟斤拷源
            if (event_handlers[i].callback != NULL) {
                Py_DECREF(event_handlers[i].callback);
                event_handlers[i].callback = NULL;
            }
            if (event_handlers[i].event_name != NULL) {
                free(event_handlers[i].event_name);
                event_handlers[i].event_name = NULL;
            }
        }
        else {
            // 锟斤拷锟节猴拷锟铰硷拷锟斤拷前锟狡碉拷锟斤拷位锟矫ｏ拷锟斤拷锟斤拷锟秸讹拷
            event_handlers[new_count] = event_handlers[i];
            new_count++;
        }
    }

    // 3. 锟斤拷锟斤拷锟斤拷效元锟斤拷锟斤拷锟斤拷锟斤拷锟秸讹拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
    handler_count = new_count;

    Logger::getInstance().logv(LOG_INFO, "Clear all kernel event handlers, total remaining handlers: %d", handler_count);
    Py_RETURN_NONE;
}

// 锟斤拷取锟斤拷锟斤拷注锟斤拷锟斤拷录锟斤拷锟较拷锟斤拷锟斤拷诘锟斤拷裕锟?
static PyObject* engine_get_events_info(PyObject* self, PyObject* args) {
    PyObject* result_dict = PyDict_New();

    for (int i = 0; i < handler_count; i++) {
        PyObject* event_name = PyTextFromUtf8(event_handlers[i].event_name);
        PyObject* callback_id = PyLong_FromLong(event_handlers[i].callback_id);

        // 锟斤拷锟截碉拷ID锟斤拷锟接碉拷锟斤拷应锟铰硷拷锟斤拷锟斤拷锟叫憋拷锟斤拷
        PyObject* handler_list = PyDict_GetItem(result_dict, event_name);
        if (!handler_list) {
            handler_list = PyList_New(0);
            PyDict_SetItem(result_dict, event_name, handler_list);
            Py_DECREF(handler_list);
        }

        PyList_Append(handler_list, callback_id);
        Py_DECREF(callback_id);
        Py_DECREF(event_name);
    }

    return result_dict;
}

static PyObject* rpc(PyObject* self, PyObject* args) {
    char* data;
    Py_ssize_t length;
    if (!PyArg_ParseTuple(args, "y#", &data, &length))
        return NULL;
    PyRpc rpc{};
    rpc.rpcdata = std::string(data, length);
    g_client_instance->getInstance()->WritePacket(rpc);
    Py_RETURN_NONE;
}
static PyObject* settingscommand(PyObject* self, PyObject* args) {
    char* data;
    Py_ssize_t length;
    // command text: Py3 's#' accepts str and yields UTF-8 (this fixes the GBK issue)
    if (!PyArg_ParseTuple(args, "s#", &data, &length))
        return NULL;
    SettingsCommand cmd{};
    cmd.command = std::string(data, length);
    g_client_instance->getInstance()->WritePacket(cmd);
    Py_RETURN_NONE;
}
static PyObject* get_mcp_load_config(PyObject* self, PyObject* args) {
    return PyBool_FromLong(ConfigLoader::use_mcp);
}

static PyObject* message(PyObject* self, PyObject* args) {
    char* name;
    char* msg;
    if (!PyArg_ParseTuple(args, "ss", &name, &msg))
        return NULL;
    Text text{};
    text.type = 1;
    text._data = std::string(name);
    text._data2 = std::string(msg);
    g_client_instance->getInstance()->WritePacket(text);
    Py_RETURN_NONE;
}
static PyObject* command(PyObject* self, PyObject* args) {
    char* name;
    if (!PyArg_ParseTuple(args, "s", &name))
        return NULL;
    std::string padding;
    padding = Easy::fillRandomBytes(padding, 16);
    CommandRequest cmd;
    cmd.RandomUUID = padding;
    cmd.command = name;
    g_client_instance->getInstance()->WritePacket(cmd);
    Py_RETURN_NONE;
}
static PyObject* command_guid(PyObject* self, PyObject* args) {
    char* name;
    char* guid;
    Py_ssize_t length;
    if (!PyArg_ParseTuple(args, "sy#", &name, &guid, &length))
        return NULL;
    std::string padding;
    CommandRequest cmd;
    cmd.RandomUUID = std::string(guid, length);
    cmd.command = name;
    g_client_instance->getInstance()->WritePacket(cmd);
    Py_RETURN_NONE;
}
static PyObject* send_binary(PyObject* self, PyObject* args) {
    char* data;
    Py_ssize_t length;
    if (!PyArg_ParseTuple(args, "y#", &data, &length))
        return NULL;
    std::string pack(data,length);
    g_client_instance->getInstance()->WritePacket(pack);
    Py_RETURN_NONE;
}

extern ConnectInstance* RakNet_connect;
static PyObject* do_login(PyObject* self, PyObject* args) {
    //NtUniSDK::SdkMgr

#ifdef Linker_NtUniSdk
    Logger::getInstance().log(LOG_INFO, "[UniSdk] do login.");
    NtUniSDK::INtUniSdkGamerInterface* ptr = *(NtUniSDK::INtUniSdkGamerInterface**)ctx;
    auto CallLoginFunction = reinterpret_cast<void(__stdcall*)(NtUniSDK::INtUniSdkGamerInterface*)>(ptr->func15);
    auto RunLoopFunc = reinterpret_cast<void(__stdcall*)(NtUniSDK::INtUniSdkGamerInterface*, float)>(ptr->func20);
    auto GetSauth = reinterpret_cast<char* (__stdcall*)(NtUniSDK::INtUniSdkGamerInterface*, const char*)>(ptr->func12);
    CallLoginFunction(ctx);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (RunLoopFunc) {
            RunLoopFunc(ctx, 0.05f);
        }
        std::string dstr = (char*)GetSauth(ctx, "SAUTH_JSON");
        if (dstr != str) {
            str = dstr;
            break;
        }
    }

    std::cout << str << std::endl;
#endif // !Linker_NtUniSdk

    Py_RETURN_NONE;
}
// 执锟斤拷cmd指锟斤拷
static PyObject* system_cmd(PyObject* self, PyObject* args) {
    char* message;

    // 锟斤拷锟斤拷锟斤拷锟斤拷
    if (!PyArg_ParseTuple(args, "s", &message)) {
        return NULL;
    }
    // 直锟接碉拷锟斤拷锟斤拷锟叫碉拷API锟斤拷锟斤拷
    system(message);

    Py_RETURN_NONE;
}
static PyObject* exit_process(PyObject* self, PyObject* args) {
    // 1. 锟斤拷锟斤拷锟斤拷Python锟斤拷锟斤拷锟斤拷锟斤拷源锟斤拷锟斤拷锟斤拷锟剿筹拷锟斤拷锟斤拷锟斤拷锟节达拷泄漏锟斤拷
    Py_Finalize();
    // 2. 锟斤拷锟斤拷锟剿筹拷锟斤拷锟教ｏ拷EXIT_SUCCESS = 0锟斤拷锟斤拷准锟剿筹拷锟诫）
    exit(EXIT_SUCCESS);
    // return只锟斤拷锟斤拷锟斤拷锟斤法锟斤拷实锟绞诧拷锟斤拷执锟叫ｏ拷exit锟窖撅拷锟斤拷止锟斤拷锟教ｏ拷
    Py_RETURN_NONE;
}
static PyObject* get_server_ip(PyObject* self, PyObject* args) {
    return PyTextFromUtf8(Params::ServerIP.c_str());
}
static PyObject* get_server_port(PyObject* self, PyObject* args) {
    return PyLong_FromLong(Params::ServerPort);
}
static PyObject* get_server_sid(PyObject* self, PyObject* args) {
    return PyTextFromUtf8(Params::NeteaseServerID.c_str());
}
#define INT_TO_BOOL(x)  ((x) != 0 ? 1 : 0)
static std::string HexEncode(const std::vector<uint8_t>& data) {
    static const char* hex = "0123456789abcdef";
    std::string out;
    out.reserve(data.size() * 2);
    for (uint8_t b : data) {
        out.push_back(hex[b >> 4]);
        out.push_back(hex[b & 0x0F]);
    }
    return out;
}

static void dict_set_item(PyObject* dict, const char* key, PyObject* value) {
    PyDict_SetItemString(dict, key, value);
    Py_DECREF(value);
}

// engine.openContainer(x, y, z)
// Sends an InventoryTransaction (use_item / click_block) to open the container
// at (x, y, z) and returns a dict:
//   { "x", "y", "z", "success",
//     "block_data": { "nbt": {...}, "nbt_len", "raw_nbt" } | None,
//     "container":  { "window_id", "window_type", "entity_unique_id",
//                     "slots": [ {network_id,count,metadata,block_runtime_id,has_stack_id,stack_id,extra_hex} ] } | None }
// Container contents arrive asynchronously (ContainerOpen 46 / InventoryContent 49)
// and are cached by BlockDataStore; register_protocol_event(46/49/56, cb, False)
// delivers the raw packet bytes to Python as they arrive.
static PyObject* open_container(PyObject* self, PyObject* args) {
    int x = 0, y = 0, z = 0;
    if (!PyArg_ParseTuple(args, "iii", &x, &y, &z)) {
        return NULL;
    }
    if (!g_client_instance || !g_client_instance->getInstance()) {
        PyErr_SetString(PyExc_RuntimeError, "client instance not ready");
        return NULL;
    }

    InventoryTransaction it;
    it.BlockPosition.x = x;
    it.BlockPosition.y = y;
    it.BlockPosition.z = z;
    it.BlockFace = 1;
    it.HotbarSlot = 0;
    LocalPlayer* lp = g_client_instance->getInstance()->getClientInstance()->getLocalPlayer();
    if (lp) {
        it.PlayerPosition = lp->position;
    }
    it.WorldPosition = Vec3{ (float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f };
    g_client_instance->getInstance()->WritePacket(it);

    PyObject* dict = PyDict_New();
    if (!dict) return NULL;
    dict_set_item(dict, "x", PyLong_FromLong(x));
    dict_set_item(dict, "y", PyLong_FromLong(y));
    dict_set_item(dict, "z", PyLong_FromLong(z));
    dict_set_item(dict, "success", PyBool_FromLong(1));

    // cached block entity data (sign text, command block command, ...)
    std::vector<uint8_t> rawNbt;
    std::map<std::string, std::string> fields;
    if (BlockDataStore::GetBlockActor(BlockPos{ x, y, z }, rawNbt, fields)) {
        PyObject* bd = PyDict_New();
        PyObject* nbt = PyDict_New();
        for (const auto& kv : fields) {
            dict_set_item(nbt, kv.first.c_str(), PyTextFromUtf8(kv.second.c_str()));
        }
        dict_set_item(bd, "nbt", nbt);
        dict_set_item(bd, "nbt_len", PyLong_FromLong((long)rawNbt.size()));
        dict_set_item(bd, "raw_nbt", PyTextFromUtf8(HexEncode(rawNbt).c_str()));
        dict_set_item(dict, "block_data", bd);
    } else {
        Py_INCREF(Py_None);
        dict_set_item(dict, "block_data", Py_None);
    }

    // cached container contents for this block (from a previous open)
    ContainerOpenInfo info;
    std::vector<SlotItem> slots;
    if (BlockDataStore::GetContainerAt(BlockPos{ x, y, z }, info, slots)) {
        PyObject* cont = PyDict_New();
        dict_set_item(cont, "window_id", PyLong_FromLong(info.WindowID));
        dict_set_item(cont, "window_type", PyLong_FromLong(info.WindowType));
        dict_set_item(cont, "entity_unique_id", PyLong_FromLongLong(info.EntityUniqueID));
        PyObject* slotList = PyList_New((Py_ssize_t)slots.size());
        for (size_t i = 0; i < slots.size(); ++i) {
            const SlotItem& s = slots[i];
            PyObject* sd = PyDict_New();
            dict_set_item(sd, "network_id", PyLong_FromLong(s.NetworkID));
            dict_set_item(sd, "count", PyLong_FromLong(s.Count));
            dict_set_item(sd, "metadata", PyLong_FromLong(s.Metadata));
            dict_set_item(sd, "block_runtime_id", PyLong_FromLong(s.BlockRuntimeID));
            dict_set_item(sd, "has_stack_id", PyBool_FromLong(s.HasStackID ? 1 : 0));
            dict_set_item(sd, "stack_id", PyLong_FromLong(s.StackID));
            dict_set_item(sd, "extra_hex", PyTextFromUtf8(HexEncode(s.Extra).c_str()));
            PyList_SetItem(slotList, (Py_ssize_t)i, sd);   // steals reference
        }
        dict_set_item(cont, "slots", slotList);
        dict_set_item(dict, "container", cont);
    } else {
        Py_INCREF(Py_None);
        dict_set_item(dict, "container", Py_None);
    }

    return dict;
}
static PyObject* command_update(PyObject* self, PyObject* args) {
    int x;
    int y;
    int z;
    int type;
    PyObject* keep_redstone;
    bool keep_redstone_;
    PyObject* co;
    bool co_;
    int tick;
    PyObject* sta_tick;
    bool sta_tick_;
    char* cmd;
    char* cName;
    if (!PyArg_ParseTuple(args, "iiiiOOiOss", &x, &y, &z, &type, &keep_redstone, &co, &tick, &sta_tick, &cmd, &cName)) {
        return NULL;
    }
    if (!PyBool_Check(keep_redstone)) {
        PyErr_SetString(PyExc_TypeError, "Expected a boolean argument (True/False)");
        return NULL; // 锟阶筹拷锟斤拷锟酵达拷锟襟，凤拷锟斤拷 NULL
    }
    keep_redstone_ = INT_TO_BOOL(PyObject_IsTrue(keep_redstone));
    if (!PyBool_Check(co)) {
        PyErr_SetString(PyExc_TypeError, "Expected a boolean argument (True/False)");
        return NULL; // 锟阶筹拷锟斤拷锟酵达拷锟襟，凤拷锟斤拷 NULL
    }
    co_ = INT_TO_BOOL(PyObject_IsTrue(co));
    if (!PyBool_Check(sta_tick)) {
        PyErr_SetString(PyExc_TypeError, "Expected a boolean argument (True/False)");
        return NULL; // 锟阶筹拷锟斤拷锟酵达拷锟襟，凤拷锟斤拷 NULL
    }
    sta_tick_ = INT_TO_BOOL(PyObject_IsTrue(sta_tick));
    CommandBlockUpdate cbu;
    BlockPos pos;
    pos.x = x;
    pos.y = y;
    pos.z = z;
    cbu.Block = true;
    cbu.Position = pos;
    cbu.Conditional = co_;
    cbu.ExecuteOnFirstTick = sta_tick_;
    cbu.Mode = type;
    cbu.Name = cName;
    cbu.Command = cmd;
    cbu.NeedsRedstone = keep_redstone_;
    cbu.ShouldTrackOutput = false;
    cbu.TickDelay = tick;

    // 使锟斤拷CommandBlockUpdate锟斤拷锟斤拷锟酵ｏ拷锟斤拷锟斤拷锟斤拷锟斤拷锟捷帮拷锟斤拷锟斤拷一锟斤拷
    g_client_instance->getInstance()->WritePacket(cbu);
    Py_RETURN_NONE;
}
/**
 * @brief 锟斤拷 Params::params (std::vector<std::string>) 转为 Python 锟叫憋拷锟斤拷锟斤拷锟斤拷
 * @param self Python 模锟斤拷/实锟斤拷指锟诫（锟斤拷展锟斤拷锟斤拷锟教讹拷锟斤拷锟斤拷锟斤拷
 * @param args Python 锟斤拷锟斤拷牟锟斤拷锟斤拷锟斤拷舜锟斤拷薏锟斤拷锟斤拷锟斤拷锟秸嘉伙拷锟?
 * @return PyObject* 指锟斤拷 Python 锟叫憋拷锟斤拷指锟诫，失锟杰凤拷锟斤拷 NULL锟斤拷锟睫憋拷准锟斤拷锟斤拷锟?
 */
static PyObject* getparams(PyObject* self, PyObject* args) {
    // 1. 锟斤拷取 C++ 锟斤拷锟?std::vector<std::string>
    const std::vector<std::string>& cpp_params = Params::params;

    // 2. 锟斤拷锟斤拷锟秸碉拷 Python 锟叫憋拷锟斤拷锟斤拷锟斤拷锟斤拷 vector 一锟斤拷
    PyObject* py_list = PyList_New(cpp_params.size());
    if (py_list == NULL) {
        // 锟斤拷锟斤拷锟斤拷 Python 锟届常锟斤拷锟斤拷使锟矫憋拷准锟斤拷锟?
        PyErr_SetString(PyExc_MemoryError, "Failed to create Python list for params");
        return NULL;
    }

    // 3. 锟斤拷锟斤拷 vector锟斤拷锟斤拷锟阶拷锟轿?Python 锟街凤拷锟斤拷锟斤拷锟斤拷锟接碉拷锟叫憋拷
    for (size_t i = 0; i < cpp_params.size(); ++i) {
        // 锟斤拷 C++ string 转为 Python 2.7 锟街凤拷锟斤拷锟斤拷锟斤拷PyString锟斤拷
        PyObject* py_str = PyTextFromGbk(cpp_params[i].c_str());
        if (py_str == NULL) {
            // 转锟斤拷失锟杰ｏ拷锟酵凤拷锟窖达拷锟斤拷锟斤拷锟叫憋拷锟斤拷锟斤拷锟斤拷锟届常锟襟返伙拷
            Py_DECREF(py_list);
            PyErr_SetString(PyExc_ValueError, "Failed to convert param string");
            return NULL;
        }

        // 锟斤拷锟街凤拷锟斤拷锟斤拷锟接碉拷锟叫憋拷指锟斤拷位锟斤拷
        int ret = PyList_SetItem(py_list, i, py_str);
        if (ret != 0) {
            // 锟斤拷锟斤拷失锟杰ｏ拷锟酵凤拷锟斤拷锟斤拷锟窖凤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷斐ｏ拷蠓祷锟?
            Py_DECREF(py_list);
            Py_DECREF(py_str);
            PyErr_SetString(PyExc_RuntimeError, "Failed to set param to list");
            return NULL;
        }
        // PyList_SetItem 锟斤拷庸锟?py_str 锟斤拷锟斤拷锟矫硷拷锟斤拷锟斤拷锟斤拷锟斤拷锟街讹拷 DECREF
    }

    // 4. 锟斤拷锟斤拷转锟斤拷锟斤拷锟?Python 锟叫憋拷锟斤拷args 锟斤拷式锟斤拷
    return py_list;
}

// 发送 MovePlayer 旋转包 (1.21.120, mode=3 rotation)，让服务器直接应用视角变化
static void sendHeadRotationPacket(ClientInstance* ci, LocalPlayer* lp) {
    MovePlayer mp;
    mp.EntityRuntimeID = lp->EntityRuntimeID;
    mp.Position = lp->position;
    mp.Pitch = lp->headRotation.x;   // 俯仰角
    mp.Yaw = lp->headRotation.y;     // 偏转角
    mp.HeadYaw = lp->headYaw;
    mp.Mode = 3;                     // rotation
    mp.OnGround = true;
    mp.RiddenRuntimeID = 0;
    mp.Tick = (__int64)ci->getTick();
    g_client_instance->getInstance()->WritePacket(mp);
}
static PyObject* add_head_x(PyObject* self, PyObject* args) {
    float input_num;
    if (!PyArg_ParseTuple(args, "f", &input_num)) {
        return NULL;
    }
    // add_head_x = 俯仰角 pitch（上下），对应 headRotation.x
    ClientInstance* ci = g_client_instance->getInstance()->getClientInstance();
    LocalPlayer* lp = ci->getLocalPlayer();
    lp->headRotation.x += input_num;
    sendHeadRotationPacket(ci, lp);
    ci->updateLocalPlayerStateOnServer();
    Py_RETURN_NONE;
}
static PyObject* add_head_y(PyObject* self, PyObject* args) {
    float input_num;
    if (!PyArg_ParseTuple(args, "f", &input_num)) {
        return NULL;
    }
    // add_head_y = 偏转角 yaw（左右），对应 headRotation.y；headYaw 跟随身体水平旋转
    ClientInstance* ci = g_client_instance->getInstance()->getClientInstance();
    LocalPlayer* lp = ci->getLocalPlayer();
    lp->headRotation.y += input_num;
    lp->headYaw += input_num;
    sendHeadRotationPacket(ci, lp);
    ci->updateLocalPlayerStateOnServer();
    Py_RETURN_NONE;
}
static PyObject* get_head_x(PyObject* self, PyObject* args) {
    // get_head_x = 俯仰角 pitch
    return PyFloat_FromDouble(g_client_instance->getInstance()->getClientInstance()->getLocalPlayer()->headRotation.x);
}
static PyObject* get_head_y(PyObject* self, PyObject* args) {
    // get_head_y = 偏转角 yaw
    return PyFloat_FromDouble(g_client_instance->getInstance()->getClientInstance()->getLocalPlayer()->headRotation.y);
}
static PyObject* add_pot_x(PyObject* self, PyObject* args) {
    float input_num;
    if (!PyArg_ParseTuple(args, "f", &input_num)) {
        return NULL;
    }
    g_client_instance->getInstance()->getClientInstance()->getLocalPlayer()->position.x += input_num;
    g_client_instance->getInstance()->getClientInstance()->updateLocalPlayerStateOnServer();
    Py_RETURN_NONE;
}
static PyObject* add_pot_y(PyObject* self, PyObject* args) {
    float input_num;
    if (!PyArg_ParseTuple(args, "f", &input_num)) {
        return NULL;
    }
    g_client_instance->getInstance()->getClientInstance()->getLocalPlayer()->position.y += input_num;
    g_client_instance->getInstance()->getClientInstance()->updateLocalPlayerStateOnServer();
    Py_RETURN_NONE;
}
static PyObject* add_pot_z(PyObject* self, PyObject* args) {
    float input_num;
    if (!PyArg_ParseTuple(args, "f", &input_num)) {
        return NULL;
    }
    g_client_instance->getInstance()->getClientInstance()->getLocalPlayer()->position.z += input_num;
    g_client_instance->getInstance()->getClientInstance()->updateLocalPlayerStateOnServer();
    Py_RETURN_NONE;
}
static PyObject* get_pot_x(PyObject* self, PyObject* args) {
    return PyFloat_FromDouble(g_client_instance->getInstance()->getClientInstance()->getLocalPlayer()->position.x);
}
static PyObject* get_pot_y(PyObject* self, PyObject* args) {
    return PyFloat_FromDouble(g_client_instance->getInstance()->getClientInstance()->getLocalPlayer()->position.y);
}
static PyObject* get_pot_z(PyObject* self, PyObject* args) {
    return PyFloat_FromDouble(g_client_instance->getInstance()->getClientInstance()->getLocalPlayer()->position.z);
}
static PyObject* get_entity_runtime_id(PyObject* self, PyObject* args) {
    return PyLong_FromLongLong(g_client_instance->getInstance()->getClientInstance()->getLocalPlayer()->EntityRuntimeID);
}
static PyObject* move(PyObject* self, PyObject* args) {
    float x;
    float y;
    float z;
    if (!PyArg_ParseTuple(args, "fff", &x, &y, &z)) {
        return NULL;
    }
    ClientInstance* ci = g_client_instance->getInstance()->getClientInstance();
    LocalPlayer* lp = ci->getLocalPlayer();
    // 1. 发送 MovePlayer 传送包 (1.21.120, mode=2 teleport)，不依赖 /tp 指令权限
    MovePlayer mp;
    mp.EntityRuntimeID = lp->EntityRuntimeID;
    mp.Position = Vec3{ x, y, z };
    mp.Pitch = lp->headRotation.x;   // 保持当前视角
    mp.Yaw = lp->headRotation.y;
    mp.HeadYaw = lp->headYaw;
    mp.Mode = 2;                     // teleport
    mp.OnGround = true;
    mp.RiddenRuntimeID = 0;
    mp.Cause = 3;                    // command
    mp.SourceEntityType = 0;
    mp.Tick = (__int64)ci->getTick();
    g_client_instance->getInstance()->WritePacket(mp);
    // 2. 同步本地坐标，让 get_pot 立即反映目标位置（服务器确认后经 onIDMovePlayer 再次校正）
    lp->position = Vec3{ x, y, z };
    ci->updateLocalPlayerStateOnServer();
    Py_RETURN_NONE;
}
static PyObject* JsonValueToPy(const Json::Value& v) {
    if (v.isString()) return PyUnicode_FromString(v.asCString());
    if (v.isBool()) return PyBool_FromLong(v.asBool() ? 1 : 0);
    if (v.isInt() || v.isUInt() || v.isInt64() || v.isUInt64()) return PyLong_FromLongLong(v.asInt64());
    if (v.isDouble()) return PyFloat_FromDouble(v.asDouble());
    if (v.isArray()) {
        PyObject* list = PyList_New((Py_ssize_t)v.size());
        for (Json::ArrayIndex i = 0; i < v.size(); i++) {
            PyList_SetItem(list, i, JsonValueToPy(v[i]));
        }
        return list;
    }
    if (v.isObject()) {
        PyObject* dict = PyDict_New();
        for (auto it = v.begin(); it != v.end(); ++it) {
            PyDict_SetItemString(dict, it.key().asCString(), JsonValueToPy(*it));
        }
        return dict;
    }
    Py_RETURN_NONE;
}

static PyObject* get_subchunk_blocks(PyObject* self, PyObject* args) {
    int ox, oy, oz;
    PyObject* offsets_list;
    if (!PyArg_ParseTuple(args, "iiiO", &ox, &oy, &oz, &offsets_list)) {
        return NULL;
    }
    if (!PyList_Check(offsets_list)) {
        PyErr_SetString(PyExc_TypeError, "offsets must be a list of (dx,dy,dz)");
        return NULL;
    }
    std::vector<std::array<int8_t, 3>> offsets;
    Py_ssize_t n = PyList_Size(offsets_list);
    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject* item = PyList_GetItem(offsets_list, i);
        if (!PySequence_Check(item) || PySequence_Size(item) < 3) {
            PyErr_SetString(PyExc_TypeError, "each offset must be a 3-tuple/list");
            return NULL;
        }
        std::array<int8_t, 3> o{};
        for (int j = 0; j < 3; j++) {
            PyObject* v = PySequence_GetItem(item, j);
            long iv = PyLong_AsLong(v);
            Py_DECREF(v);
            if (iv == -1 && PyErr_Occurred()) return NULL;
            o[j] = (int8_t)iv;
        }
        offsets.push_back(o);
    }

    std::vector<SubChunkClient::BlockData> blocks;
    bool req_ok;
    Py_BEGIN_ALLOW_THREADS   // 释放 GIL:等待响应时让接收线程能处理 174 包 + Python 回调
    req_ok = SubChunkClient::RequestBlocks(ox, oy, oz, offsets, blocks);
    Py_END_ALLOW_THREADS
    if (!req_ok) {
        PyErr_SetString(PyExc_RuntimeError, "SubChunk request failed or timed out");
        return NULL;
    }

    PyObject* result = PyList_New((Py_ssize_t)blocks.size());
    Json::Reader jreader;
    for (size_t i = 0; i < blocks.size(); i++) {
        const auto& b = blocks[i];
        PyObject* d = PyDict_New();
        PyDict_SetItemString(d, "x", PyLong_FromLong(b.x));
        PyDict_SetItemString(d, "y", PyLong_FromLong(b.y));
        PyDict_SetItemString(d, "z", PyLong_FromLong(b.z));
        PyDict_SetItemString(d, "name", PyUnicode_FromString(b.name.c_str()));
        Json::Value sj;
        if (jreader.parse(b.states_json, sj)) {
            PyDict_SetItemString(d, "states", JsonValueToPy(sj));
        } else {
            PyDict_SetItemString(d, "states", PyDict_New());
        }
        PyList_SetItem(result, (Py_ssize_t)i, d);
    }
    return result;
}

static PyObject* disabled_auth_input(PyObject* self, PyObject* args) {
    g_client_instance->getInstance()->getClientInstance()->SetSourceAuthInput(false);
    Py_RETURN_NONE;
}
static PyObject* enable_auth_input(PyObject* self, PyObject* args) {
    g_client_instance->getInstance()->getClientInstance()->SetSourceAuthInput(true);
    Py_RETURN_NONE;
}
static PyObject* get_auth_input(PyObject* self, PyObject* args) {
    return PyBool_FromLong(g_client_instance->getInstance()->getClientInstance()->GetSourceAuthInput());
}
static PyObject* respawn(PyObject* self, PyObject* args) {
    g_client_instance->getInstance()->getClientInstance()->respawn();
    Py_RETURN_NONE;
}
static PyObject* register_protocol_event(PyObject* self, PyObject* args) {
    PyObject* callback;
    uint32_t packet_id;
    PyObject* is_kernel;
    bool is_kernel_c;
    if (!PyArg_ParseTuple(args, "IOO", &packet_id, &callback, &is_kernel)) {
        return NULL;
    }
    if (!PyBool_Check(is_kernel)) {
        PyErr_SetString(PyExc_TypeError, "Expected a boolean argument (True/False)");
        return NULL; // 锟阶筹拷锟斤拷锟酵达拷锟襟，凤拷锟斤拷 NULL
    }
    is_kernel_c = INT_TO_BOOL(PyObject_IsTrue(is_kernel));

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "parameter must be callable");
        return NULL;
    }
    if (!check_function_accepts_args(callback)) {
        PyErr_SetString(PyExc_TypeError, "The registered function must accept at least one argument.");
        return NULL;
    }
    g_client_instance->getInstance()->RegisterReceiveCallBack(packet_id, callback, is_kernel_c);
    PyObject* repr_str = PyObject_Repr(callback);
    std::string c_str = repr_str ? (PyUnicode_AsUTF8(repr_str) ? PyUnicode_AsUTF8(repr_str) : "") : "";
    // 锟斤拷锟斤拷 c_str 锟斤拷锟角ｏ拷
    // <common.system.eventHandler.EventHandler object at 0x12345678>


    Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] RegisterEngineHandler: Minecraft:Engine:" + std::to_string(packet_id) +
        " "+ c_str);
    Logger::getInstance().log(LOG_WARN, std::string() + " *********************** register_event_id ************************ BusID: 0, EventID: " +
        std::to_string(packet_id + 1) + ", EventHandlers: " + std::to_string(packet_id));
    Py_DECREF(repr_str);

    Py_RETURN_NONE;
}
static PyObject* clear_all_protocol_event(PyObject* self, PyObject* args) {
    g_client_instance->getInstance()->ClearAllEvent();
    Py_RETURN_NONE;
}
static PyObject* clear_all_kernel_protocol_event(PyObject* self, PyObject* args) {
    g_client_instance->getInstance()->ClearKernelEvent();
    Py_RETURN_NONE;
}
static PyObject* clear_all_user_protocol_event(PyObject* self, PyObject* args) {
    g_client_instance->getInstance()->ClearUserEvent();
    Py_RETURN_NONE;
}


// 模锟介方锟斤拷锟斤拷锟斤拷
static PyMethodDef EngineMethods[] = {
    {"get_mcp_load_config", get_mcp_load_config, METH_NOARGS, "get mcp config"},
    {"rpc", rpc, METH_VARARGS, "Send netease rpc."},
    {"settingscommand", settingscommand, METH_VARARGS, "Send command."},
    {"register", engine_register, METH_VARARGS, "Register an event handler function, which must accept an array parameter."},

    {"register_protocol_event", register_protocol_event, METH_VARARGS, "***"},
    {"clear_all_protocol_event", clear_all_protocol_event, METH_NOARGS, "***"},
    {"clear_all_kernel_protocol_event", clear_all_kernel_protocol_event, METH_NOARGS, "***"},
    {"clear_all_user_protocol_event", clear_all_user_protocol_event, METH_NOARGS, "***"},

    {"register_kernel_event", register_kernel_event, METH_VARARGS, "***"},
    {"unregister", engine_unregister, METH_VARARGS, "Unregister an event handler function."},
    {"is_registered", engine_is_registered, METH_VARARGS, "Check if an event handler is registered."},
    {"get_event_handler_count", engine_get_event_handler_count, METH_VARARGS, "Get handler count for specific event."},
    {"get_events_info", engine_get_events_info, METH_NOARGS, "Get all registered events information."},
    {"trigger", engine_trigger, METH_VARARGS, "Trigger event (for testing)"},
    {"get_handler_count", engine_get_handler_count, METH_NOARGS, "Get the number of event handlers."},
    {"cleanup_all", engine_cleanup, METH_NOARGS, "Clear all event handlers."},
    {"cleanup_user", engine_user_cleanup, METH_NOARGS, "Clear all event handlers."},
    {"cleanup_kernel", engine_kernel_cleanup, METH_NOARGS, "Clear all event handlers."},
    {"message", message, METH_VARARGS, "Send game text packet."},
    {"command", command, METH_VARARGS, "Send game command request packet."},
    {"command_guid", command_guid, METH_VARARGS, "Send game command request packet."},
    {"send", send_binary, METH_VARARGS, "send to server binary data."},
    {"do_login", do_login, METH_NOARGS, "ntunisdk login."},
    {"system", system_cmd, METH_VARARGS, "execute windows cmd."},
    {"exit", exit_process, METH_VARARGS, "exit process."},
    {"get_server_ip", get_server_ip, METH_VARARGS, "getip"},
    {"get_server_port", get_server_port, METH_VARARGS, "getport"},
    {"get_server_sid", get_server_sid, METH_VARARGS, "getsid"},
    {"command_update", command_update, METH_VARARGS, "set command block"},
    {"openContainer", open_container, METH_VARARGS, "Open container at (x, y, z) and return cached block data."},
    {"getparams", getparams, METH_VARARGS, "get program start param"},

    {"add_head_x", add_head_x, METH_VARARGS, ""},
    {"add_head_y", add_head_y, METH_VARARGS, ""},
    {"get_head_x", get_head_x, METH_NOARGS, ""},
    {"get_head_y", get_head_y, METH_NOARGS, ""},
    {"add_pot_x", add_pot_x, METH_VARARGS, ""},
    {"add_pot_y", add_pot_y, METH_VARARGS, ""},
    {"add_pot_z", add_pot_z, METH_VARARGS, ""},
    {"get_pot_x", get_pot_x, METH_NOARGS, ""},
    {"get_pot_y", get_pot_y, METH_NOARGS, ""},
    {"get_pot_z", get_pot_z, METH_NOARGS, ""},
    {"move", move, METH_VARARGS, ""},
    {"get_subchunk_blocks", get_subchunk_blocks, METH_VARARGS, "Request subchunks, return [{x,y,z,name,states}]"},
    {"get_entity_runtime_id", get_entity_runtime_id, METH_NOARGS, ""},
    {"get_auth_input", get_auth_input, METH_NOARGS, ""},
    {"disabled_auth_input", disabled_auth_input, METH_NOARGS, ""},
    {"enable_auth_input", enable_auth_input, METH_NOARGS, ""},
    {"key_down", getparams, METH_VARARGS, ""},
    {"key_up", getparams, METH_VARARGS, ""},
    {"respawn", respawn, METH_VARARGS, ""},


    {NULL, NULL, 0, NULL} // 锟斤拷锟斤拷锟斤拷锟?
};

// Module init (Python 3)
static struct PyModuleDef engine_module = { PyModuleDef_HEAD_INIT, "engine", NULL, -1, EngineMethods };
PyMODINIT_FUNC PyInit_engine(void) {
    return PyModule_Create(&engine_module);
}


































// 2. 锟斤拷锟斤拷实锟斤拷锟斤拷raknet.get_raknet()
static PyObject*
raknet_get_client(PyObject* self, PyObject* args)
{
    // 锟斤拷锟斤拷C锟结构锟斤拷锟节达拷
    ClientInstance* inst = new ClientInstance();
    if (!inst) {
        PyErr_SetString(PyExc_MemoryError, "Failed to allocate RakNet instance");
        return NULL;
    }

    // 锟斤拷C指锟斤拷锟阶帮拷锟絇yCObject锟斤拷锟截ｏ拷Python锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷装锟斤拷锟斤拷
    return PyCapsule_New(inst, "client", NULL);
}
/*
// 3. 锟斤拷取锟斤拷锟皆ｏ拷raknet.get(self) 锟斤拷 锟斤拷锟斤拷锟斤拷get_port_ip为锟斤拷
static PyObject*
raknet_get_port_ip(PyObject* self, PyObject* args)
{
    // 锟斤拷锟斤拷Python锟姐传锟斤拷锟絇yCObject锟斤拷锟斤拷RakNet实锟斤拷锟斤拷装锟斤拷锟斤拷
    PyObject* py_inst;
    if (!PyArg_ParseTuple(args, "O", &py_inst)) {
        return NULL;
    }

    // 锟斤拷PyCObject锟斤拷锟斤拷取C锟斤拷指锟斤拷
    RakNetInstance* inst = (RakNetInstance*)PyCapsule_GetPointer(py_inst, "client");
    if (!inst) {
        PyErr_SetString(PyExc_ValueError, "Invalid RakNet instance");
        return NULL;
    }

    // 锟斤拷锟斤拷锟斤拷锟斤拷元锟介（锟剿口★拷IP锟斤拷
    return Py_BuildValue("is", inst->port, inst->server_ip);
}
*/
/*
// 4. 锟斤拷锟斤拷锟斤拷锟皆ｏ拷raknet.set**(self, **) 锟斤拷 锟斤拷锟斤拷锟斤拷set_port_ip为锟斤拷
static PyObject*
raknet_set_port_ip(PyObject* self, PyObject* args)
{
    PyObject* py_inst;
    int port;
    char* server_ip;

    // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷实锟斤拷锟斤拷锟剿口★拷IP
    if (!PyArg_ParseTuple(args, "Ois", &py_inst, &port, &server_ip)) {
        return NULL;
    }

    // 锟斤拷取C锟斤拷指锟斤拷
    RakNetInstance* inst = (RakNetInstance*)PyCapsule_GetPointer(py_inst, "client");
    if (!inst) {
        PyErr_SetString(PyExc_ValueError, "Invalid RakNet instance");
        return NULL;
    }

    // 锟斤拷锟斤拷锟斤拷锟皆ｏ拷注锟斤拷锟街凤拷锟斤拷锟节达拷锟斤拷锟斤拷锟?
    inst->port = port;
    free(inst->server_ip);          // 锟酵放撅拷IP
    inst->server_ip = strdup(server_ip);  // 锟斤拷锟斤拷锟斤拷IP

    Py_RETURN_NONE;
}
*/
/*
// 5. 锟斤拷锟斤拷实锟斤拷锟斤拷raknet.delete(self)
static PyObject*
raknet_delete(PyObject* self, PyObject* args)
{
    PyObject* py_inst;
    if (!PyArg_ParseTuple(args, "O", &py_inst)) {
        return NULL;
    }

    // 锟斤拷取C锟斤拷指锟斤拷
    RakNetInstance* inst = (RakNetInstance*)PyCapsule_GetPointer(py_inst, "client");
    if (!inst) {
        PyErr_SetString(PyExc_ValueError, "Invalid RakNet instance");
        return NULL;
    }

    // 锟酵凤拷C锟结构锟斤拷锟节诧拷锟斤拷源
    free(inst->server_ip);
    // 锟酵放结构锟藉本锟斤拷
    free(inst);

    // 锟斤拷锟絇yCObject锟斤拷指锟诫（锟斤拷锟斤拷锟截革拷锟酵放ｏ拷
    PyCapsule_SetPointer(py_inst, NULL);

    Py_RETURN_NONE;
}
*/

static PyObject*
disconnect(PyObject* self, PyObject* args)
{
    PyObject* py_inst;

    // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷实锟斤拷锟斤拷锟剿口★拷IP
    if (!PyArg_ParseTuple(args, "O", &py_inst)) {
        return NULL;
    }

    // 锟斤拷取C锟斤拷指锟斤拷
    ClientInstance* inst = (ClientInstance*)PyCapsule_GetPointer(py_inst, "client");
    if (!inst) {
        PyErr_SetString(PyExc_ValueError, "Invalid RakNet instance");
        return NULL;
    }
    inst->disconnect();

    Py_RETURN_NONE;
}
static PyObject*
startUp(PyObject* self, PyObject* args)
{
    PyObject* py_inst;
    char* MD5Token;
    Py_ssize_t length;
    char* DisplayName;
    char* UserID;
    char* EngineVersion;
    char* PatchVersion;
    char* AuthServerUrl;
    char* NeteaseServerID;
    char* ServerIP;
    int port;
    if (!PyArg_ParseTuple(args, "Oy#sssssssi", &py_inst, &MD5Token, &length,
        &DisplayName, &UserID, &EngineVersion, &PatchVersion,
        &AuthServerUrl, &ServerIP, &NeteaseServerID, &port)) {
        return NULL;
    }

    ChainPair Pair;
    Pair.AuthServerUrl = AuthServerUrl;
    Pair.DisplayName = DisplayName;
    Pair.EngineVersion = EngineVersion;
    Pair.PatchVersion = PatchVersion;
    Pair.MD5Token = std::string(MD5Token, 16);
    Pair.UserID = UserID;
    Pair.NeteaseServerID = NeteaseServerID;

    ClientInstance* inst = (ClientInstance*)PyCapsule_GetPointer(py_inst, "client");
    if (!inst) {
        PyErr_SetString(PyExc_ValueError, "Invalid Client instance");
        return NULL;
    }

    // 锟斤拷 锟斤拷锟斤拷锟斤拷:锟斤拷 Login 锟斤拷 startUp
    LoginSession session = LoginAuth::Login(Pair, ConfigLoader::SkinData);
    if (!session.valid()) {
        PyErr_SetString(PyExc_RuntimeError, "Login failed");
        return NULL;
    }
    inst->startUp(std::move(session), ServerIP, port);

    Py_RETURN_NONE;
}
static PyObject*
client_delete(PyObject* self, PyObject* args)
{
    PyObject* py_inst;
    if (!PyArg_ParseTuple(args, "O", &py_inst)) {
        return NULL;
    }

    // 锟斤拷取C锟斤拷指锟斤拷
    ClientInstance* inst = (ClientInstance*)PyCapsule_GetPointer(py_inst, "client");
    if (!inst) {
        PyErr_SetString(PyExc_ValueError, "Invalid RakNet instance");
        return NULL;
    }
    inst->disconnect();
    delete inst;

    // 锟斤拷锟絇yCObject锟斤拷指锟诫（锟斤拷锟斤拷锟截革拷锟酵放ｏ拷
    PyCapsule_SetPointer(py_inst, NULL);

    Py_RETURN_NONE;
}

// 6. 锟斤拷锟斤拷锟叫憋拷锟斤拷映锟斤拷Python锟斤拷锟斤拷锟斤拷锟斤拷C锟斤拷锟斤拷
static PyMethodDef RakNetMethods[] = {
    {"get_client",  raknet_get_client, METH_NOARGS, "Create a new Client instance"},
    {"startUp", startUp, METH_VARARGS, ""},
    {"disconnect", disconnect, METH_VARARGS, ""},
    {"delete",      client_delete,      METH_VARARGS, "Destroy Client instance"},
    {NULL, NULL, 0, NULL}  // 锟斤拷锟斤拷锟斤拷锟?
};

// Module init (Python 3)
static struct PyModuleDef _client_module = { PyModuleDef_HEAD_INIT, "_client", NULL, -1, RakNetMethods };
PyMODINIT_FUNC PyInit__client(void)
{
    return PyModule_Create(&_client_module);
}








class ClientInstancePythonWrapper
{
public:
    static void startUp(std::string MD5Token,
        std::string DisplayName,
        std::string UserID,
        std::string EngineVersion,
        std::string PatchVersion,
        std::string AuthServerUrl,
        std::string NeteaseServerID,
        std::string ServerIP,
        int port)
    {
        ChainPair Pair;
        Pair.AuthServerUrl = AuthServerUrl;
        Pair.DisplayName = DisplayName;
        Pair.EngineVersion = EngineVersion;
        Pair.PatchVersion = PatchVersion;
        Pair.MD5Token = MD5Token;
        Pair.UserID = UserID;
        Pair.NeteaseServerID = NeteaseServerID;

        // 锟斤拷 锟斤拷 API:一锟斤拷锟斤拷录锟斤拷 LoginSession
        LoginSession session = LoginAuth::Login(Pair, ConfigLoader::SkinData);
        if (!session.valid()) {
            LOG(LOG_ERROR, "[ClientInstancePythonWrapper] Login failed");
            return;
        }

        if (g_client_instance) {
            g_client_instance->disconnect();
            delete g_client_instance;
        }
        g_client_instance = new ClientInstance();
        g_client_instance->startUp(std::move(session), ServerIP, port);
    }

    static void disconnect()
    {
        if (g_client_instance) {
            g_client_instance->disconnect();
            delete g_client_instance;
            g_client_instance = nullptr;
        }
    }
};


#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(client_instance, m) {
    m.def("startUp", &ClientInstancePythonWrapper::startUp);
    m.def("disconnect", &ClientInstancePythonWrapper::disconnect);
}