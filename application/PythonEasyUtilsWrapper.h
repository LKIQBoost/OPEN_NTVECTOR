#pragma once
#include "httplib.h"
#include <string>
#include <map>
#include "EasyUtils.cpp"
class PythonEasyUtilsWrapper
{
public:
    std::string CompileDynamicToken(std::string Token, std::string Body, std::string Url) {
        return Easy::ComputeDynamicToken(Token, Body, Url);
    }
};

class PyHttpLibWrapper
{
public:
    static std::string Request(
        const std::string& url,
        const std::string& data,
        const std::map<std::string, std::string>& headers)
    {
        // 解析 url: https://host:port/path
        std::string scheme, host, path;
        int port = 0;

        size_t scheme_end = url.find("://");
        if (scheme_end != std::string::npos) {
            scheme = url.substr(0, scheme_end);
            size_t host_start = scheme_end + 3;
            size_t path_start = url.find('/', host_start);
            std::string host_port;
            if (path_start != std::string::npos) {
                host_port = url.substr(host_start, path_start - host_start);
                path = url.substr(path_start);
            }
            else {
                host_port = url.substr(host_start);
                path = "/";
            }
            size_t colon = host_port.find(':');
            if (colon != std::string::npos) {
                host = host_port.substr(0, colon);
                port = std::stoi(host_port.substr(colon + 1));
            }
            else {
                host = host_port;
                port = (scheme == "https") ? 443 : 80;
            }
        }

        std::string content_type = "application/octet-stream";
        httplib::Headers h;
        for (auto& kv : headers) {
            if (kv.first == "Content-Type") {
                content_type = kv.second;
            }
            h.emplace(kv.first, kv.second);
        }
        if (scheme == "https") {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
            httplib::SSLClient cli(host, port);
            cli.enable_server_certificate_verification(false);
            auto res = cli.Post(path.c_str(), h, data, content_type.c_str());
            if (res) return res->body;
            return "";
#else
            return "";
#endif
        }
        else {
            httplib::Client cli(host, port);
            auto res = cli.Post(path.c_str(), h, data, content_type.c_str());
            if (res) return res->body;
            return "";
        }
    }
};


#include <pybind11/pybind11.h>
#include "PythonEasyUtilsWrapper.h"

namespace py = pybind11;

PYBIND11_MODULE(easy_utils, m) {
    m.def("ComputeDynamicToken", [](std::string Token, std::string Body, std::string Url) {
        return Easy::ComputeDynamicToken(Token, Body, Url);
        });
    m.def("HttpEncrypt", [](std::string in) {
        std::string out;
        Easy::HttpEncrypt(&out, &in);
        return py::bytes(out);
        });
    m.def("HttpDecrypt", [](std::string in) {
        std::string out;
        Easy::HttpDecrypt(&out, &in);
        return py::bytes(out);
        });
    m.def("encrypt", [](std::string in, int offset, int rounds) {
        std::string out = Easy::encrypt(in, offset, rounds);
        return py::bytes(out);
        });
    m.def("Request", [](std::string url, std::string data, py::dict headers) {
        std::map<std::string, std::string> h;
        for (auto item : headers) {
            h[py::cast<std::string>(item.first)] = py::cast<std::string>(item.second);
        }
        return py::bytes(PyHttpLibWrapper::Request(url, data, h));
        });
}