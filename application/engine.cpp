// engine.cpp
#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine.h"  // 包含头文件
#include "Logger.h"
#include "StringSplitUtils.h"
#include "ConnectInstance.h"
#include "PacketCommon.h"
#include "ClientInstance.h"
#include "ConfigLoader.h"

extern ClientInstance* g_client_instance;

#ifdef Linker_NtUniSdk
#include "NtUniSdkBase.h"
#include "MPayDelegate.h"

extern NtUniSDK::INtUniSdkGamerInterface* ctx;
std::string str;
#endif // !Linker_NtUniSdk
// 事件处理器的结构
typedef struct {
    PyObject* callback;
    char* event_name;
    long callback_id;  // 回调函数的唯一ID（内存地址）
    bool is_kernel_event; // 是否为内核事件（特殊处理，禁止覆盖）
} EventHandler;

// 全局事件处理器数组
static EventHandler* event_handlers = NULL;
static int handler_count = 0;
static int handler_capacity = 0;

// 判断两个可调用对象是否相等
static bool are_callables_equal(PyObject* callback1, PyObject* callback2) {
    // 1. 先比较指针（最简单的情况）
    if (callback1 == callback2) {
        return true;
    }

    // 2. 比较函数名、代码对象等
    if (PyFunction_Check(callback1) && PyFunction_Check(callback2)) {
        // 获取函数名
        PyObject* name1 = PyObject_GetAttrString(callback1, "__name__");
        PyObject* name2 = PyObject_GetAttrString(callback2, "__name__");

        // 获取代码对象
        PyObject* code1 = PyObject_GetAttrString(callback1, "__code__");
        PyObject* code2 = PyObject_GetAttrString(callback2, "__code__");

        bool equal = false;
        if (name1 && name2 && code1 && code2) {
            // 比较函数名
            if (PyObject_RichCompareBool(name1, name2, Py_EQ) == 1) {
                // 比较代码对象的文件名、第一行号等
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

// 获取回调函数的唯一标识（使用内存地址）
static long get_callback_id(PyObject* callback) {
    return (long)callback;
}

// 检查函数是否接受至少一个参数（用于数组参数）
static int check_function_accepts_args(PyObject* callback) {
    PyObject* func_code = PyObject_GetAttrString(callback, "func_code");
    if (!func_code) {
        return 0;
    }

    PyObject* co_argcount = PyObject_GetAttrString(func_code, "co_argcount");
    if (!co_argcount) {
        Py_DECREF(func_code);
        return 0;
    }

    int argcount = PyInt_AsLong(co_argcount);
    Py_DECREF(co_argcount);
    Py_DECREF(func_code);

    return argcount >= 1;
}

// 注册事件的函数（支持多个函数注册到同一事件）
static PyObject* engine_register(PyObject* self, PyObject* args) {
    PyObject* callback;
    char* event_name;

    // 解析参数：函数对象和事件名称字符串
    if (!PyArg_ParseTuple(args, "Os", &callback, &event_name)) {
        return NULL;
    }

    // 检查第一个参数是否为可调用对象
    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "The first argument must be a callable object");
        return NULL;
    }

    // 检查函数是否接受至少一个参数（用于数组参数）
    if (!check_function_accepts_args(callback)) {
        PyErr_SetString(PyExc_TypeError, "The registered function must accept at least one argument.");
        return NULL;
    }

    // 检查是否已经注册过相同的回调函数到同一事件
    for (int i = 0; i < handler_count; i++) {
        if (strcmp(event_handlers[i].event_name, event_name) == 0) {
            if (are_callables_equal(callback, event_handlers[i].callback)) {
                // 相同的回调函数已经注册到同一事件，先解注旧的再注册新的（覆盖）
                Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] Info: The same callback function has already been registered to the event '" + event_name + "', perform overwrite operation");

                // 释放旧的资源
                Py_DECREF(event_handlers[i].callback);
                free(event_handlers[i].event_name);

                // 增加新回调的引用计数
                Py_INCREF(callback);

                // 更新事件处理器
                event_handlers[i].callback = callback;
                event_handlers[i].event_name = strdup(event_name);
                event_handlers[i].callback_id = get_callback_id(callback);

                if (!event_handlers[i].event_name) {
                    Py_DECREF(callback);
                    PyErr_NoMemory();
                    return NULL;
                }

                PyObject* repr_str = PyObject_Repr(callback);
                const char* c_str = PyString_AsString(repr_str);
                // 现在 c_str 就是：
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

    // 增加引用计数，防止被垃圾回收
    Py_INCREF(callback);

    // 扩展事件处理器数组
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

    // 存储事件处理器
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

    // 统计该事件的注册函数数量
    int event_handler_count = 0;
    for (int i = 0; i < handler_count; i++) {
        if (strcmp(event_handlers[i].event_name, event_name) == 0) {
            event_handler_count++;
        }
    }

    PyObject* repr_str = PyObject_Repr(callback);
    const char* c_str = PyString_AsString(repr_str);
    // 现在 c_str 就是：
    // <common.system.eventHandler.EventHandler object at 0x12345678>
    Py_DECREF(repr_str);
    Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] RegisterEngineHandler: Minecraft:Engine:" + event_name +
        " "+c_str);
    Logger::getInstance().log(LOG_WARN, std::string() + " *********************** register_event_id ************************ BusID: 0, EventID: " +
        std::to_string(new_handler_index + 1) + ", EventHandlers: " + std::to_string(event_handler_count));

    Py_RETURN_NONE;
}
// 注册事件的函数（支持多个函数注册到同一事件）
static PyObject* register_kernel_event(PyObject* self, PyObject* args) {
    PyObject* callback;
    char* event_name;

    // 解析参数：函数对象和事件名称字符串
    if (!PyArg_ParseTuple(args, "Os", &callback, &event_name)) {
        return NULL;
    }

    // 检查第一个参数是否为可调用对象
    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "The first argument must be a callable object");
        return NULL;
    }

    // 检查函数是否接受至少一个参数（用于数组参数）
    if (!check_function_accepts_args(callback)) {
        PyErr_SetString(PyExc_TypeError, "The registered function must accept at least one argument.");
        return NULL;
    }

    // 检查是否已经注册过相同的回调函数到同一事件
    for (int i = 0; i < handler_count; i++) {
        if (strcmp(event_handlers[i].event_name, event_name) == 0) {
            if (are_callables_equal(callback, event_handlers[i].callback)) {
                // 相同的回调函数已经注册到同一事件，先解注旧的再注册新的（覆盖）
                Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] Info: The same callback function has already been registered to the event '" + event_name + "', perform overwrite operation");

                // 释放旧的资源
                Py_DECREF(event_handlers[i].callback);
                free(event_handlers[i].event_name);

                // 增加新回调的引用计数
                Py_INCREF(callback);

                // 更新事件处理器
                event_handlers[i].callback = callback;
                event_handlers[i].event_name = strdup(event_name);
                event_handlers[i].callback_id = get_callback_id(callback);

                if (!event_handlers[i].event_name) {
                    Py_DECREF(callback);
                    PyErr_NoMemory();
                    return NULL;
                }

                PyObject* repr_str = PyObject_Repr(callback);
                const char* c_str = PyString_AsString(repr_str);
                // 现在 c_str 就是：
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

    // 增加引用计数，防止被垃圾回收
    Py_INCREF(callback);

    // 扩展事件处理器数组
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

    // 存储事件处理器
    event_handlers[handler_count].callback = callback;
    event_handlers[handler_count].event_name = strdup(event_name);
    event_handlers[handler_count].callback_id = get_callback_id(callback);
    event_handlers[handler_count].is_kernel_event = true; // 标记为内核事件

    if (!event_handlers[handler_count].event_name) {
        Py_DECREF(callback);
        PyErr_NoMemory();
        return NULL;
    }

    int new_handler_index = handler_count;
    handler_count++;

    // 统计该事件的注册函数数量
    int event_handler_count = 0;
    for (int i = 0; i < handler_count; i++) {
        if (strcmp(event_handlers[i].event_name, event_name) == 0) {
            event_handler_count++;
        }
    }

    PyObject* repr_str = PyObject_Repr(callback);
    const char* c_str = PyString_AsString(repr_str);
    // 现在 c_str 就是：
    // <common.system.eventHandler.EventHandler object at 0x12345678>
    Py_DECREF(repr_str);
    Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] RegisterEngineHandler: Minecraft:Engine:" + event_name +
        " "+ c_str);
    Logger::getInstance().log(LOG_WARN, std::string() + " *********************** register_event_id ************************ BusID: 0, EventID: " +
        std::to_string(new_handler_index + 1) + ", EventHandlers: " + std::to_string(event_handler_count));

    Py_RETURN_NONE;
}

// 解注册事件的函数
static PyObject* engine_unregister(PyObject* self, PyObject* args) {
    PyObject* callback = NULL;
    char* event_name = NULL;

    // 解析参数：事件名称字符串和可选的函数对象
    if (!PyArg_ParseTuple(args, "s|O", &event_name, &callback)) {
        return NULL;
    }

    // 如果没有提供回调函数，则删除该事件的所有处理器
    if (callback == NULL) {
        int removed_count = 0;

        for (int i = 0; i < handler_count; i++) {
            if (strcmp(event_handlers[i].event_name, event_name) == 0) {
                // 释放资源
                Py_DECREF(event_handlers[i].callback);
                free(event_handlers[i].event_name);

                // 将最后一个元素移动到当前位置
                if (i < handler_count - 1) {
                    event_handlers[i] = event_handlers[handler_count - 1];
                }

                handler_count--;
                i--; // 重新检查当前位置（因为元素被移动了）
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

    // 如果提供了回调函数，只删除匹配的处理器
    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "The second argument must be a callable object");
        return NULL;
    }

    for (int i = 0; i < handler_count; i++) {
        if (strcmp(event_handlers[i].event_name, event_name) == 0 &&
            are_callables_equal(callback, event_handlers[i].callback)) {

            PyObject* repr_str = PyObject_Repr(callback);
            const char* c_str = PyString_AsString(repr_str);
            // 现在 c_str 就是：
            // <common.system.eventHandler.EventHandler object at 0x12345678>
            Py_DECREF(repr_str);
            // 记录日志
            Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] UnregisterEngineHandler: Minecraft:Engine:" + event_name +
                " "+ c_str);

            // 释放资源
            Py_DECREF(event_handlers[i].callback);
            free(event_handlers[i].event_name);

            // 将最后一个元素移动到当前位置
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
// 取消注册事件的函数
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
        // 找到匹配的事件处理器，移除它
        Py_DECREF(event_handlers[found_index].callback);
        free(event_handlers[found_index].event_name);

        // 将数组中的元素向前移动
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
// 检查事件是否已注册
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

// 获取指定事件的注册函数数量
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

    return PyInt_FromLong(count);
}

// C++层触发事件的函数（导出给C++使用）
void trigger_event(const char* event_name, PyObject* args_array) {
    Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] do cpp Engine Wrapper Event: " + event_name);
    //PyGILState_STATE oldstate = PyGILState_Ensure();
    if (!event_handlers || !args_array) return;

    // 确保args_array是列表或元组
    if (!PyList_Check(args_array) && !PyTuple_Check(args_array)) {
        Logger::getInstance().log(LOG_ERROR, "[Engine] The parameter must be a list or a tuple");
        return;
    }

    int triggered_count = 0;
    for (int i = 0; i < handler_count; i++) {
        if (strcmp(event_handlers[i].event_name, event_name) == 0) {
            // 将数组参数打包成元组传递给回调函数
            PyObject* arg_tuple = PyTuple_New(1);
            Py_INCREF(args_array); // 增加引用计数
            PyTuple_SetItem(arg_tuple, 0, args_array);
            PyObject* result;
            try {
                result = PyObject_CallObject(event_handlers[i].callback, arg_tuple);
                Py_DECREF(arg_tuple);

                if (result == NULL) {
                    PyErr_Print(); // 打印Python异常
                }
                else {
                    Py_DECREF(result);
                }
            }
            catch (...) {
                Logger::getInstance().log(LOG_ERROR, "python call object error.");
                PyErr_Print(); // 打印Python异常
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

// 触发事件并传递多个参数（自动打包成数组）
void trigger_event_with_args(const char* event_name, int arg_count, ...) {
    if (!event_handlers) return;

    PyGILState_STATE gstate = PyGILState_Ensure();

    // 创建参数列表
    PyObject* args_list = PyList_New(arg_count);
    if (!args_list) {
        PyGILState_Release(gstate);
        return;
    }

    va_list vl;
    va_start(vl, arg_count);

    for (int i = 0; i < arg_count; i++) {
        PyObject* arg = va_arg(vl, PyObject*);
        Py_INCREF(arg); // 增加引用计数
        PyList_SetItem(args_list, i, arg);
    }

    va_end(vl);

    // 触发事件
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

// 测试用的触发函数（Python可调用）
static PyObject* engine_trigger(PyObject* self, PyObject* args) {
    char* event_name;
    PyObject* args_array = NULL;

    if (!PyArg_ParseTuple(args, "s|O", &event_name, &args_array)) {
        return NULL;
    }

    if (args_array == NULL) {
        // 如果没有提供参数，创建空数组
        args_array = PyList_New(0);
    }
    else if (!PyList_Check(args_array) && !PyTuple_Check(args_array)) {
        // 如果参数不是列表或元组，将其包装成列表
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

// 获取已注册事件数量的函数
static PyObject* engine_get_handler_count(PyObject* self, PyObject* args) {
    return PyInt_FromLong(handler_count);
}

// 清理函数（防止内存泄漏）
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
// 清理函数（防止内存泄漏）
static PyObject* engine_user_cleanup(PyObject* self, PyObject* args) {
    // 1. 空数组保护
    if (event_handlers == NULL || handler_count == 0) {
        Logger::getInstance().log(LOG_INFO, "No user event handlers to clear.");
        Py_RETURN_NONE;
    }

    // 2. 遍历数组，清理内核事件资源 + 把保留的元素前移（消除空洞）
    int new_count = 0; // 记录保留的元素数量（非内核事件）
    for (int i = 0; i < handler_count; i++) {
        if (!event_handlers[i].is_kernel_event) {
            // 清理内核事件资源
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
            // 非内核事件：前移到新位置，消除空洞
            event_handlers[new_count] = event_handlers[i];
            new_count++;
        }
    }

    // 3. 更新有效元素数量（空洞被“挤掉”）
    handler_count = new_count;

    Logger::getInstance().logv(LOG_INFO, "Clear all user event handlers, total remaining handlers: %d", handler_count);
    Py_RETURN_NONE;
}
// 清理函数（防止内存泄漏 + 消除数组空洞）
static PyObject* engine_kernel_cleanup(PyObject* self, PyObject* args) {
    // 1. 空数组保护
    if (event_handlers == NULL || handler_count == 0) {
        Logger::getInstance().log(LOG_INFO, "No kernel event handlers to clear.");
        Py_RETURN_NONE;
    }

    // 2. 遍历数组，清理内核事件资源 + 把保留的元素前移（消除空洞）
    int new_count = 0; // 记录保留的元素数量（非内核事件）
    for (int i = 0; i < handler_count; i++) {
        if (event_handlers[i].is_kernel_event) {
            // 清理内核事件资源
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
            // 非内核事件：前移到新位置，消除空洞
            event_handlers[new_count] = event_handlers[i];
            new_count++;
        }
    }

    // 3. 更新有效元素数量（空洞被“挤掉”）
    handler_count = new_count;

    Logger::getInstance().logv(LOG_INFO, "Clear all kernel event handlers, total remaining handlers: %d", handler_count);
    Py_RETURN_NONE;
}

// 获取所有注册的事件信息（用于调试）
static PyObject* engine_get_events_info(PyObject* self, PyObject* args) {
    PyObject* result_dict = PyDict_New();

    for (int i = 0; i < handler_count; i++) {
        PyObject* event_name = PyString_FromString(event_handlers[i].event_name);
        PyObject* callback_id = PyInt_FromLong(event_handlers[i].callback_id);

        // 将回调ID添加到对应事件名的列表中
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
    int length;
    if (!PyArg_ParseTuple(args, "s#", &data, &length))
        return NULL;
    PyRpc rpc{};
    rpc.rpcdata = std::string(data, length);
    g_client_instance->getInstance()->WritePacket(rpc);
    Py_RETURN_NONE;
}
static PyObject* settingscommand(PyObject* self, PyObject* args) {
    char* data;
    int length;
    if (!PyArg_ParseTuple(args, "s#", &data, &length))
        return NULL;
    SettingsCommand cmd{};
    //cmd.command = WindowsEncodingConverter::gbkToUtf8(std::string(data, length));
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
    int length;
    if (!PyArg_ParseTuple(args, "ss#", &name, &guid, &length))
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
    int length;
    if (!PyArg_ParseTuple(args, "s#", &data, &length))
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
// 执行cmd指令
static PyObject* system_cmd(PyObject* self, PyObject* args) {
    char* message;

    // 解析参数
    if (!PyArg_ParseTuple(args, "s", &message)) {
        return NULL;
    }
    // 直接调用已有的API函数
    system(message);

    Py_RETURN_NONE;
}
static PyObject* exit_process(PyObject* self, PyObject* args) {
    // 1. 先清理Python解释器资源（优雅退出，避免内存泄漏）
    Py_Finalize();
    // 2. 正常退出进程，EXIT_SUCCESS = 0（标准退出码）
    exit(EXIT_SUCCESS);
    // return只是满足语法，实际不会执行（exit已经终止进程）
    Py_RETURN_NONE;
}
static PyObject* get_server_ip(PyObject* self, PyObject* args) {
    return PyString_FromString(Params::ServerIP.c_str());
}
static PyObject* get_server_port(PyObject* self, PyObject* args) {
    return PyInt_FromLong(Params::ServerPort);
}
static PyObject* get_server_sid(PyObject* self, PyObject* args) {
    return PyString_FromString(Params::NeteaseServerID.c_str());
}
#define INT_TO_BOOL(x)  ((x) != 0 ? 1 : 0)
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
        return NULL; // 抛出类型错误，返回 NULL
    }
    keep_redstone_ = INT_TO_BOOL(PyObject_IsTrue(keep_redstone));
    if (!PyBool_Check(co)) {
        PyErr_SetString(PyExc_TypeError, "Expected a boolean argument (True/False)");
        return NULL; // 抛出类型错误，返回 NULL
    }
    co_ = INT_TO_BOOL(PyObject_IsTrue(co));
    if (!PyBool_Check(sta_tick)) {
        PyErr_SetString(PyExc_TypeError, "Expected a boolean argument (True/False)");
        return NULL; // 抛出类型错误，返回 NULL
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
    std::vector<uint8_t> data = cbu.Serializ();
    //cout << Easy::StringToHex_s((char*)data.data(), data.size()) << '\n';
    g_client_instance->getInstance()->WritePacket(cbu);
    Py_RETURN_NONE;
}
/**
 * @brief 将 Params::params (std::vector<std::string>) 转为 Python 列表并返回
 * @param self Python 模块/实例指针（扩展函数固定参数）
 * @param args Python 传入的参数（此处无参数，仅占位）
 * @return PyObject* 指向 Python 列表的指针，失败返回 NULL（无标准输出）
 */
static PyObject* getparams(PyObject* self, PyObject* args) {
    // 1. 获取 C++ 侧的 std::vector<std::string>
    const std::vector<std::string>& cpp_params = Params::params;

    // 2. 创建空的 Python 列表，长度与 vector 一致
    PyObject* py_list = PyList_New(cpp_params.size());
    if (py_list == NULL) {
        // 仅设置 Python 异常，不使用标准输出
        PyErr_SetString(PyExc_MemoryError, "Failed to create Python list for params");
        return NULL;
    }

    // 3. 遍历 vector，逐个转换为 Python 字符串并添加到列表
    for (size_t i = 0; i < cpp_params.size(); ++i) {
        // 将 C++ string 转为 Python 2.7 字符串对象（PyString）
        PyObject* py_str = PyString_FromString(cpp_params[i].c_str());
        if (py_str == NULL) {
            // 转换失败：释放已创建的列表，设置异常后返回
            Py_DECREF(py_list);
            PyErr_SetString(PyExc_ValueError, "Failed to convert param string");
            return NULL;
        }

        // 将字符串添加到列表指定位置
        int ret = PyList_SetItem(py_list, i, py_str);
        if (ret != 0) {
            // 添加失败：释放所有已分配对象，设置异常后返回
            Py_DECREF(py_list);
            Py_DECREF(py_str);
            PyErr_SetString(PyExc_RuntimeError, "Failed to set param to list");
            return NULL;
        }
        // PyList_SetItem 会接管 py_str 的引用计数，无需手动 DECREF
    }

    // 4. 返回转换后的 Python 列表（args 格式）
    return py_list;
}

static PyObject* add_head_x(PyObject* self, PyObject* args) {
    float input_num;
    if (!PyArg_ParseTuple(args, "f", &input_num)) {
        return NULL;
    }
    g_client_instance->getInstance()->getClientInstance()->getLocalPlayer()->headYaw += input_num;
    g_client_instance->getInstance()->getClientInstance()->getLocalPlayer()->headRotation.y += input_num;
    g_client_instance->getInstance()->getClientInstance()->updateLocalPlayerStateOnServer();
    Py_RETURN_NONE;
}
static PyObject* add_head_y(PyObject* self, PyObject* args) {
    float input_num;
    if (!PyArg_ParseTuple(args, "f", &input_num)) {
        return NULL;
    }
    g_client_instance->getInstance()->getClientInstance()->getLocalPlayer()->headRotation.x += input_num;
    g_client_instance->getInstance()->getClientInstance()->updateLocalPlayerStateOnServer();
    Py_RETURN_NONE;
}
static PyObject* get_head_x(PyObject* self, PyObject* args) {
    return PyFloat_FromDouble(g_client_instance->getInstance()->getClientInstance()->getLocalPlayer()->headYaw);
}
static PyObject* get_head_y(PyObject* self, PyObject* args) {
    return PyFloat_FromDouble(g_client_instance->getInstance()->getClientInstance()->getLocalPlayer()->headRotation.x);
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
    g_client_instance->getInstance()->getClientInstance()->getLocalPlayer()->position.z = x;
    g_client_instance->getInstance()->getClientInstance()->getLocalPlayer()->position.y = y;
    g_client_instance->getInstance()->getClientInstance()->getLocalPlayer()->position.z = z;
    g_client_instance->getInstance()->getClientInstance()->updateLocalPlayerStateOnServer();
    Py_RETURN_NONE;
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
        return NULL; // 抛出类型错误，返回 NULL
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
    const char* c_str = PyString_AsString(repr_str);
    // 现在 c_str 就是：
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


// 模块方法定义
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
    {"get_entity_runtime_id", get_entity_runtime_id, METH_NOARGS, ""},
    {"get_auth_input", get_auth_input, METH_NOARGS, ""},
    {"disabled_auth_input", disabled_auth_input, METH_NOARGS, ""},
    {"enable_auth_input", enable_auth_input, METH_NOARGS, ""},
    {"key_down", getparams, METH_VARARGS, ""},
    {"key_up", getparams, METH_VARARGS, ""},
    {"respawn", respawn, METH_VARARGS, ""},


    {NULL, NULL, 0, NULL} // 结束标记
};

// 模块初始化函数
void initengine(void) {
    (void)Py_InitModule("engine", EngineMethods);
}


































// 2. 创建实例：raknet.get_raknet()
static PyObject*
raknet_get_client(PyObject* self, PyObject* args)
{
    // 分配C结构体内存
    ClientInstance* inst = new ClientInstance();
    if (!inst) {
        PyErr_SetString(PyExc_MemoryError, "Failed to allocate RakNet instance");
        return NULL;
    }

    // 将C指针包装成PyCObject返回（Python层持有这个包装对象）
    return PyCObject_FromVoidPtr(inst, NULL);
}
/*
// 3. 获取属性：raknet.get(self) → 这里以get_port_ip为例
static PyObject*
raknet_get_port_ip(PyObject* self, PyObject* args)
{
    // 解析Python层传入的PyCObject（即RakNet实例包装对象）
    PyObject* py_inst;
    if (!PyArg_ParseTuple(args, "O", &py_inst)) {
        return NULL;
    }

    // 从PyCObject中提取C层指针
    RakNetInstance* inst = (RakNetInstance*)PyCObject_AsVoidPtr(py_inst);
    if (!inst) {
        PyErr_SetString(PyExc_ValueError, "Invalid RakNet instance");
        return NULL;
    }

    // 返回属性元组（端口、IP）
    return Py_BuildValue("is", inst->port, inst->server_ip);
}
*/
/*
// 4. 设置属性：raknet.set**(self, **) → 这里以set_port_ip为例
static PyObject*
raknet_set_port_ip(PyObject* self, PyObject* args)
{
    PyObject* py_inst;
    int port;
    char* server_ip;

    // 解析参数：实例、端口、IP
    if (!PyArg_ParseTuple(args, "Ois", &py_inst, &port, &server_ip)) {
        return NULL;
    }

    // 提取C层指针
    RakNetInstance* inst = (RakNetInstance*)PyCObject_AsVoidPtr(py_inst);
    if (!inst) {
        PyErr_SetString(PyExc_ValueError, "Invalid RakNet instance");
        return NULL;
    }

    // 设置属性（注意字符串内存管理）
    inst->port = port;
    free(inst->server_ip);          // 释放旧IP
    inst->server_ip = strdup(server_ip);  // 拷贝新IP

    Py_RETURN_NONE;
}
*/
/*
// 5. 销毁实例：raknet.delete(self)
static PyObject*
raknet_delete(PyObject* self, PyObject* args)
{
    PyObject* py_inst;
    if (!PyArg_ParseTuple(args, "O", &py_inst)) {
        return NULL;
    }

    // 提取C层指针
    RakNetInstance* inst = (RakNetInstance*)PyCObject_AsVoidPtr(py_inst);
    if (!inst) {
        PyErr_SetString(PyExc_ValueError, "Invalid RakNet instance");
        return NULL;
    }

    // 释放C结构体内部资源
    free(inst->server_ip);
    // 释放结构体本身
    free(inst);

    // 清空PyCObject的指针（避免重复释放）
    PyCObject_SetVoidPtr(py_inst, NULL);

    Py_RETURN_NONE;
}
*/

static PyObject*
disconnect(PyObject* self, PyObject* args)
{
    PyObject* py_inst;

    // 解析参数：实例、端口、IP
    if (!PyArg_ParseTuple(args, "O", &py_inst)) {
        return NULL;
    }

    // 提取C层指针
    ClientInstance* inst = (ClientInstance*)PyCObject_AsVoidPtr(py_inst);
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
    int length;
    char* DisplayName;
    char* UserID;
    char* EngineVersion;
    char* PatchVersion;
    char* AuthServerUrl;
    char* NeteaseServerID;
    char* ServerIP;
    int port;
    if (!PyArg_ParseTuple(args, "Os#sssssssi", &py_inst, &MD5Token, &length,
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

    ClientInstance* inst = (ClientInstance*)PyCObject_AsVoidPtr(py_inst);
    if (!inst) {
        PyErr_SetString(PyExc_ValueError, "Invalid Client instance");
        return NULL;
    }

    // ★ 新流程:先 Login 再 startUp
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

    // 提取C层指针
    ClientInstance* inst = (ClientInstance*)PyCObject_AsVoidPtr(py_inst);
    if (!inst) {
        PyErr_SetString(PyExc_ValueError, "Invalid RakNet instance");
        return NULL;
    }
    inst->disconnect();
    delete inst;

    // 清空PyCObject的指针（避免重复释放）
    PyCObject_SetVoidPtr(py_inst, NULL);

    Py_RETURN_NONE;
}

// 6. 方法列表：映射Python调用名到C函数
static PyMethodDef RakNetMethods[] = {
    {"get_client",  raknet_get_client, METH_NOARGS, "Create a new Client instance"},
    {"startUp", startUp, METH_VARARGS, ""},
    {"disconnect", disconnect, METH_VARARGS, ""},
    {"delete",      client_delete,      METH_VARARGS, "Destroy Client instance"},
    {NULL, NULL, 0, NULL}  // 结束标记
};

// 7. 模块初始化（Python 2.7 固定格式：init+模块名）
void initclient(void)
{
    (void)Py_InitModule("_client", RakNetMethods);
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

        // ★ 新 API:一步登录拿 LoginSession
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