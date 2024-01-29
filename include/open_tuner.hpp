#ifndef OPEN_TUNER_HPP
#define OPEN_TUNER_HPP

#include <string>
#include <iostream>
#include <vector>
#include <cstring>
#include <Python.h>

#include "search_technique.hpp"
#include "helper.hpp"

namespace atf {

class open_tuner : public atf::search_technique {
public:
    open_tuner() {
        static bool initialized = false;
        if (!initialized) {
            std::string python_code = R"(
import opentuner
from opentuner.search.manipulator import ConfigurationManipulator
from opentuner.measurement.interface import DefaultMeasurementInterface
from opentuner.api import TuningRunManager
from opentuner.resultsdb.models import Result
from opentuner.search.manipulator import FloatParameter
import argparse

class search_technique:
    def __init__(self):
        parser = argparse.ArgumentParser(parents=opentuner.argparsers())
        self.args = parser.parse_args()
        self.args.no_dups = True
        self.api = None

    def database(self, path):
        self.args.database = path

    def initialize(self, dimensionality):
        self.manipulator = ConfigurationManipulator()
        for i in range(dimensionality):
            self.manipulator.add_parameter(FloatParameter('PARAM' + str(i), 0.0, 1.0))
        self.interface = DefaultMeasurementInterface(args=self.args,
                                                     manipulator=self.manipulator,
                                                     project_name='atf',
                                                     program_name='atf',
                                                     program_version='1.0')
        self.api = TuningRunManager(self.interface, self.args)

    def finalize(self):
        self.api.finish()

    def get_next_coordinates(self):
        self.desired_result = self.api.get_next_desired_result()
        while self.desired_result is None or self.desired_result.configuration is None or self.desired_result.configuration.data is None:
            self.desired_result = self.api.get_next_desired_result()
        return self.desired_result.configuration.data

    def report_costs(self, costs):
        self.api.report_result(self.desired_result, Result(time=costs))
)";
            char program_name[29];
            strcpy(program_name, "atf_opentuner_integration.py");
            program_name[28] = '\0';

            Py_Initialize(); check_python_error();
            std::vector<std::string> opentuner_cmd_line_arguments = {
                "atf_opentuner_integration.py"
            };
            std::vector<char*> opentuner_cmd_line_arguments_as_c_str;
            auto str_to_char = []( const std::string& str ){ char* c_str = new char[ str.size() + 1 ];
                std::strcpy(c_str, str.c_str());
                return c_str;
            };
            std::transform(opentuner_cmd_line_arguments.begin(),
                           opentuner_cmd_line_arguments.end(),
                           std::back_inserter(opentuner_cmd_line_arguments_as_c_str),
                           str_to_char);
            PySys_SetArgv(static_cast<int>(opentuner_cmd_line_arguments_as_c_str.size()),
                          opentuner_cmd_line_arguments_as_c_str.data()); check_python_error();
            auto error = PyRun_SimpleString( python_code.c_str() );
            if (error != 0)
                throw std::runtime_error("error: running python script failed with error " + std::to_string(error));
            _py_main_module = PyImport_ImportModule( "__main__" ); check_python_error();
            _py_search_technique_class = PyObject_GetAttrString(_py_main_module, "search_technique"); check_python_error();
            if (_py_search_technique_class == nullptr)
                throw std::runtime_error("failed to retrieve search_technique class from python");

            initialized = true;
        }
        create_py_search_technique_instance();
    }

    open_tuner(const open_tuner &other) : _dimensionality(other._dimensionality) {
        create_py_search_technique_instance();
        if (!other._database.empty())
            database(other._database);
    }

    open_tuner& operator=(const open_tuner &other) {
        _dimensionality = other._dimensionality;
        create_py_search_technique_instance();
        return *this;
    }

    ~open_tuner() {
        Py_XDECREF(_py_search_technique);
        Py_XDECREF(_py_database);
        Py_XDECREF(_py_initialize);
        Py_XDECREF(_py_finalize);
        Py_XDECREF(_py_get_next_coordinates);
        Py_XDECREF(_py_report_costs);

        static bool registered_py_finalize = false;
        if (!registered_py_finalize) {
            std::atexit( Py_Finalize );
            registered_py_finalize = true;
        }
    }

    open_tuner& database( const std::string& path ) {
        PyObject* arg = PyTuple_New(1);
        PyTuple_SetItem(arg, 0, PyString_FromString(path.c_str()));
        PyObject_CallObject(_py_database, arg); check_python_error();
        Py_DECREF(arg);
        _database = path;
        return *this;
    }


    void initialize(size_t dimensionality) override {
        _dimensionality = dimensionality;
        PyObject* arg = PyTuple_New(1);
        PyTuple_SetItem(arg, 0, PyInt_FromSize_t(dimensionality));
        PyObject_CallObject(_py_initialize, arg); check_python_error();
        Py_DECREF(arg);
    }

    void finalize() override {
        PyObject_CallObject(_py_finalize, nullptr); check_python_error();
    }

    std::set<atf::coordinates> get_next_coordinates() override {
        PyObject* p_config = PyObject_CallObject(_py_get_next_coordinates, nullptr); check_python_error();
        if (p_config == nullptr)
            throw std::runtime_error("failed to retrieve config from py_get_next_coordinates");
        atf::coordinates indices;
        for (size_t i = 0 ; i < _dimensionality; ++i) {
            PyObject* pValue = PyDict_GetItemString(p_config, (std::string("PARAM") + std::to_string(i)).c_str());
            indices.push_back(PyFloat_AsDouble(pValue));
        }
        Py_DECREF( p_config );
        if (!atf::valid_coordinates(indices))
            atf::clamp_coordinates_capped(indices);
        return { indices };
    }

    void report_costs(const std::map<atf::coordinates, atf::cost_t>& costs) override {
        PyObject* arg = PyTuple_New(1);
        PyTuple_SetItem(arg, 0, PyFloat_FromDouble(costs.begin()->second));
        PyObject_CallObject(_py_report_costs, arg); check_python_error();
        Py_DECREF(arg);
    }
private:
    static PyObject* _py_main_module;
    static PyObject* _py_search_technique_class;

    size_t    _dimensionality;
    PyObject* _py_search_technique;
    PyObject* _py_database;
    PyObject* _py_initialize;
    PyObject* _py_finalize;
    PyObject* _py_get_next_coordinates;
    PyObject* _py_report_costs;

    std::string _database;

    void check_python_error() {
        PyObject* error = PyErr_Occurred();
        if (error != nullptr) {
            PyErr_Print();
            PyErr_Clear();
            throw std::exception();
        }
    }

    void create_py_search_technique_instance() {
        _py_search_technique = PyObject_CallObject(_py_search_technique_class, nullptr); check_python_error();
        if (_py_search_technique == nullptr)
            throw std::runtime_error("failed to retrieve search_technique object from python");
        _py_database = PyObject_GetAttrString(_py_search_technique, "database"); check_python_error();
        if (_py_database == nullptr)
            throw std::runtime_error("failed to retrieve 'database' method from search_technique object");
        _py_initialize = PyObject_GetAttrString(_py_search_technique, "initialize"); check_python_error();
        if (_py_initialize == nullptr)
            throw std::runtime_error("failed to retrieve 'initialize' method from search_technique object");
        _py_finalize = PyObject_GetAttrString(_py_search_technique, "finalize"); check_python_error();
        if (_py_finalize == nullptr)
            throw std::runtime_error("failed to retrieve 'finalize' method from search_technique object");
        _py_get_next_coordinates = PyObject_GetAttrString(_py_search_technique, "get_next_coordinates"); check_python_error();
        if (_py_get_next_coordinates == nullptr)
            throw std::runtime_error("failed to retrieve 'get_next_coordinates' method from search_technique object");
        _py_report_costs = PyObject_GetAttrString(_py_search_technique, "report_costs"); check_python_error();
        if (_py_report_costs == nullptr)
            throw std::runtime_error("failed to retrieve 'report_costs' method from search_technique object");
    }
};

PyObject* open_tuner::_py_main_module = nullptr;
PyObject* open_tuner::_py_search_technique_class = nullptr;

}

#endif //OPEN_TUNER_HPP
