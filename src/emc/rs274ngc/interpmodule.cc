// Interpreter internals - Python bindings
// Michael Haberler 7/2011
//

#include <boost/python.hpp>
#include <boost/python/suite/indexing/map_indexing_suite.hpp>
#include <map>

namespace bp = boost::python;
extern int _task;  // zero in gcodemodule, 1 in milltask

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "rs274ngc.hh"
#include "interp_return.hh"
#include "interp_internal.hh"
#include "rs274ngc_interp.hh"
#include "units.h"
#include "array1.hh"

extern void export_Internals();
extern void export_Arrays();
extern void export_Block();
extern void export_EmcTypes();
#include "paramclass.hh"

namespace pp = pyplusplus::containers::static_sized;
#include "interp_array_types.hh"

#define IS_STRING(x) (PyObject_IsInstance(x.ptr(), (PyObject*)&PyString_Type))
#define IS_INT(x) (PyObject_IsInstance(x.ptr(), (PyObject*)&PyInt_Type))

static  active_g_codes_array active_g_codes_wrapper ( Interp & inst) {
    return active_g_codes_array(inst._setup.active_g_codes);
}
static  active_m_codes_array active_m_codes_wrapper ( Interp & inst) {
    return active_m_codes_array(inst._setup.active_m_codes);
}

static  active_settings_array active_settings_wrapper ( Interp & inst) {
    return active_settings_array(inst._setup.active_settings);
}

static  blocks_array blocks_wrapper ( Interp & inst) {
    return blocks_array(inst._setup.blocks);
}

static  parameters_array parameters_wrapper ( Interp & inst) {
    return parameters_array(inst._setup.parameters);
}

static  tool_table_array tool_table_wrapper ( Interp & inst) {
    return tool_table_array(inst._setup.tool_table);
}

static  sub_context_array sub_context_wrapper ( Interp & inst) {
    return sub_context_array(inst._setup.sub_context);
}


#pragma GCC diagnostic ignored "-Wformat-security"
static void setErrorMsg(Interp &interp, const char *s)
{
    setup *settings  = &interp._setup;

    if ((s == NULL) || (strlen(s) == 0))
	s = "###";
    interp.setError (s);
    settings->stack_index = 0;
    strncpy(settings->stack[settings->stack_index],
	    "Python", STACK_ENTRY_LEN);
    settings->stack[settings->stack_index][STACK_ENTRY_LEN-1] = 0;
    settings->stack_index++;
    settings->stack[settings->stack_index][0] = 0;
}

#pragma GCC diagnostic warning "-Wformat-security"

static bp::object errorStack(Interp &interp)
{
    setup *settings  = &interp._setup;
    bp::list msgs;

    for (int i = 0; i < settings->stack_index; i++)
	msgs.append(bp::object( (const char *) settings->stack[i]));
    return msgs;
}

static bp::object wrap_find_tool_pocket(Interp &interp, int toolno)
{
    int status, pocket;
    setup *settings  =  &interp._setup;

    status = interp.find_tool_pocket(settings, toolno, &pocket);
    return bp::make_tuple(status, pocket);
}


// FIXME not sure if this is really needed
static  ParamClass param_wrapper ( Interp & inst) {
    return ParamClass(inst);
}

static int get_task(Interp &i) { return _task; };
static const char *get_filename(Interp &i) { return i._setup.filename; };
static const char *get_linetext(Interp &i) { return i._setup.linetext; };

// those are exposed here because they look useful for regression testing
static bool __equal(double a, double b) { return fabs(a - b) < TOLERANCE_EQUAL; }
// see interp_convert.cc
static bool is_near_int(double value) {
    int i = (int)(value + .5);
    return fabs(i - value) < .0001;
}
static int nearest_int(double value) { return (int)(value + .5); }

int (Interp::*execute_1)(const char *command) = &Interp::execute;
int (Interp::*execute_2)(const char *command, int line_number) = &Interp::execute;

// lifted from http://stackoverflow.com/questions/2261858/boostpython-export-custom-exception


class InterpreterException : public std::exception {
private:
    std::string error_message;
    int line_number;
    std::string line_text;
public:
    InterpreterException(std::string error_message, int line_number, std::string line_text)  {
	this->error_message = error_message;
	this->line_number = line_number;
	this->line_text = line_text;
    }
    const char *what() const throw() { return this->error_message.c_str();  }

    ~InterpreterException() throw()  {}
    std::string get_error_message()  { return this->error_message;  }
    int get_line_number()    { return this->line_number;  }
    std::string get_line_text()      { return this->line_text; }
};

static PyObject *InterpreterExceptionType = NULL;

void translateInterpreterException(InterpreterException const &e)
{
  assert(InterpreterExceptionType != NULL);
  bp::object pythonExceptionInstance(e);
  PyErr_SetObject(InterpreterExceptionType, pythonExceptionInstance.ptr());
}

static int throw_exceptions = 1;

static int wrap_interp_execute_1(Interp &interp, const char *command)
{    
    setup &_setup = interp._setup;
    block saved_block = _setup.blocks[0];

    // use the remap stack to save/restore the current block
    CHP(interp.enter_remap());
    int status = interp.execute(command);
    CHP(interp.leave_remap());
    _setup.blocks[0] = saved_block;

    // printf("ie1: tc=%d if=%d pf=%d\n", _setup.toolchange_flag,_setup.input_flag,_setup.probe_flag);

    if ((status > INTERP_MIN_ERROR) && throw_exceptions) {
	throw InterpreterException(interp.getSavedError(),
				   _setup.blocks[0].line_number, // not sure
				   _setup.linetext);
    }
    return status;
}

static int wrap_interp_execute_2(Interp &interp, const char *command, int lineno)
{
    setup &_setup = interp._setup;
    block saved_block = _setup.blocks[0];

    // use the remap stack to save/restore the current block
    CHP(interp.enter_remap());
    int status = interp.execute(command, lineno);
    CHP(interp.leave_remap());
    _setup.blocks[0] = saved_block;
    
    //printf("ie2: tc=%d if=%d pf=%d\n", _setup.toolchange_flag,_setup.input_flag,_setup.probe_flag);
    if ((status > INTERP_MIN_ERROR) && throw_exceptions) {
	throw InterpreterException(interp.getSavedError(),
				   lineno, // not sure
				   _setup.linetext);
    }
    return status;
}

// this might not be a good idea - it destroys the block which has a 'o<ngcbody> call' parsed in it
static int wrap_interp_read(Interp &interp, const char *command)
{
    setup &_setup = interp._setup;
    block saved_block = _setup.blocks[0];

    int status = interp.read(command);
    if ((status > INTERP_MIN_ERROR) && throw_exceptions) {
	throw InterpreterException(interp.getSavedError(),
				   _setup.blocks[0].line_number, // not sure
				   _setup.linetext);
    }
    return status;
}


BOOST_PYTHON_MODULE(interpreter) {
    using namespace boost::python;
    using namespace boost;

    scope().attr("__doc__") =
        "Interpreter introspection\n"
        ;
    scope().attr("throw_exceptions") = throw_exceptions;

    scope().attr("INTERP_OK") = INTERP_OK;
    scope().attr("INTERP_EXIT") = INTERP_EXIT;
    scope().attr("INTERP_EXECUTE_FINISH") = INTERP_EXECUTE_FINISH;
    scope().attr("INTERP_ENDFILE") = INTERP_ENDFILE;
    scope().attr("INTERP_FILE_NOT_OPEN") = INTERP_FILE_NOT_OPEN;
    scope().attr("INTERP_ERROR") = INTERP_ERROR;
    scope().attr("INTERP_MIN_ERROR") = INTERP_MIN_ERROR;
    scope().attr("TOLERANCE_EQUAL") = TOLERANCE_EQUAL;

    scope().attr("MODE_ABSOLUTE") = (int) MODE_ABSOLUTE;
    scope().attr("MODE_INCREMENTAL") = (int) MODE_INCREMENTAL;
    scope().attr("R_PLANE") =  (int) R_PLANE;
    scope().attr("OLD_Z") =  (int) OLD_Z;

    scope().attr("UNITS_PER_MINUTE") =  (int) UNITS_PER_MINUTE;
    scope().attr("INVERSE_TIME") =  (int) INVERSE_TIME;
    scope().attr("UNITS_PER_REVOLUTION") =  (int) UNITS_PER_REVOLUTION;

    scope().attr("RIGHT") = RIGHT;
    scope().attr("LEFT") = LEFT;
    scope().attr("CONSTANT_RPM") =  (int) CONSTANT_RPM;
    scope().attr("CONSTANT_SURFACE") =  (int) CONSTANT_SURFACE;

    def("equal", &__equal);  // EMC's perception of equality of doubles
    def("is_near_int", &is_near_int);  // EMC's perception of closeness to an int
    def("nearest_int", &nearest_int);

    export_EmcTypes();
    export_ParamClass();
    export_Internals();
    export_Block();
    class_<InterpreterException>InterpreterExceptionClass("InterpreterException",							bp::init<std::string, int, std::string>());
    InterpreterExceptionClass
	.add_property("error_message", &InterpreterException::get_error_message)
	.add_property("line_number", &InterpreterException::get_line_number)
	.add_property("line_text", &InterpreterException::get_line_text)
	;

    InterpreterExceptionType = InterpreterExceptionClass.ptr();

    bp::register_exception_translator<InterpreterException>
	(&translateInterpreterException);

    class_< Interp, noncopyable >("Interp",no_init) 

	.def("find_tool_pocket", &wrap_find_tool_pocket)
	.def("load_tool_table", &Interp::load_tool_table)
	.def("set_tool_parameters", &Interp::set_tool_parameters)


	.def("set_errormsg", &setErrorMsg)
	.def("get_errormsg", &Interp::getSavedError)
	.def("stack", &errorStack)
	.def_readwrite("stack_index", Interp::_setup.stack_index) // error stack pointer, beyond last entry

	.def("synch", &Interp::synch)

	// those will raise exceptions on return value < INTERP_MIN_ERROR  if throw_exceptions is set.
	.def("execute", &wrap_interp_execute_1)
	.def("execute",  &wrap_interp_execute_2)
	.def("read", &wrap_interp_read)

	.add_property("task", &get_task) // R/O
	.add_property("filename", &get_filename) // R/O
	.add_property("linetext", &get_linetext) // R/O

	.def_readwrite("a_axis_wrapped", &Interp::_setup.a_axis_wrapped)
	.def_readwrite("b_axis_wrapped", &Interp::_setup.b_axis_wrapped)
	.def_readwrite("c_axis_wrapped", &Interp::_setup.c_axis_wrapped)
	.def_readwrite("a_indexer", &Interp::_setup.a_indexer)
	.def_readwrite("b_indexer", &Interp::_setup.b_indexer)
	.def_readwrite("c_indexer", &Interp::_setup.c_indexer)

	.def_readwrite("AA_axis_offset", &Interp::_setup.AA_axis_offset)
	.def_readwrite("AA_current", &Interp::_setup.AA_current)
	.def_readwrite("AA_origin_offset", &Interp::_setup.AA_origin_offset)
	.def_readwrite("BB_axis_offset", &Interp::_setup.BB_axis_offset)
	.def_readwrite("BB_current", &Interp::_setup.BB_current)
	.def_readwrite("BB_origin_offset", &Interp::_setup.BB_origin_offset)
	.def_readwrite("CC_axis_offset", &Interp::_setup.CC_axis_offset)
	.def_readwrite("CC_current", &Interp::_setup.CC_current)
	.def_readwrite("CC_origin_offset", &Interp::_setup.CC_origin_offset)
	.def_readwrite("arc_not_allowed", &Interp::_setup.arc_not_allowed)
	.def_readwrite("axis_offset_x", &Interp::_setup.axis_offset_x)
	.def_readwrite("axis_offset_y", &Interp::_setup.axis_offset_y)
	.def_readwrite("axis_offset_z", &Interp::_setup.axis_offset_z)
	.def_readwrite("call_level", &Interp::_setup.call_level)
	.def_readwrite("current_pocket", &Interp::_setup.current_pocket)
	.def_readwrite("current_tool", &Interp::_setup.tool_table[0].toolno)
	.def_readwrite("current_x", &Interp::_setup.current_x)
	.def_readwrite("current_y", &Interp::_setup.current_y)
	.def_readwrite("current_z", &Interp::_setup.current_z)
	.def_readwrite("cutter_comp_firstmove", &Interp::_setup.cutter_comp_firstmove)
	.def_readwrite("cutter_comp_orientation", &Interp::_setup.cutter_comp_orientation)
	.def_readwrite("cutter_comp_radius", &Interp::_setup.cutter_comp_radius)
	.def_readwrite("cutter_comp_side", &Interp::_setup.cutter_comp_side)
	.def_readwrite("cycle_cc", &Interp::_setup.cycle_cc)
	.def_readwrite("cycle_i", &Interp::_setup.cycle_i)
	.def_readwrite("cycle_il", &Interp::_setup.cycle_il)
	.def_readwrite("cycle_il_flag", &Interp::_setup.cycle_il_flag)
	.def_readwrite("cycle_j", &Interp::_setup.cycle_j)
	.def_readwrite("cycle_k", &Interp::_setup.cycle_k)
	.def_readwrite("cycle_l", &Interp::_setup.cycle_l)
	.def_readwrite("cycle_p", &Interp::_setup.cycle_p)
	.def_readwrite("cycle_q", &Interp::_setup.cycle_q)
	.def_readwrite("cycle_r", &Interp::_setup.cycle_r)
	.def_readwrite("debugmask", &Interp::_setup.debugmask)
	.def_readwrite("distance_mode", &Interp::_setup.distance_mode)
	.def_readwrite("feed_mode", &Interp::_setup.feed_mode)
	.def_readwrite("feed_override", &Interp::_setup.feed_override)
	.def_readwrite("feed_rate", &Interp::_setup.feed_rate)
	.def_readwrite("ijk_distance_mode", &Interp::_setup.ijk_distance_mode)
	.def_readwrite("input_digital", &Interp::_setup.input_digital)
	.def_readwrite("input_flag", &Interp::_setup.input_flag)
	.def_readwrite("input_index", &Interp::_setup.input_index)
	.def_readwrite("length_units", &Interp::_setup.length_units)
	.def_readwrite("loggingLevel", &Interp::_setup.loggingLevel)
	.def_readwrite("mdi_interrupt", &Interp::_setup.mdi_interrupt)
	.def_readwrite("mist", &Interp::_setup.mist)
	.def_readwrite("motion_mode", &Interp::_setup.motion_mode)
	.def_readwrite("origin_index", &Interp::_setup.origin_index)
	.def_readwrite("origin_offset_x", &Interp::_setup.origin_offset_x)
	.def_readwrite("origin_offset_y", &Interp::_setup.origin_offset_y)
	.def_readwrite("origin_offset_z", &Interp::_setup.origin_offset_z)
	.def_readwrite("percent_flag", &Interp::_setup.percent_flag)
	.def_readwrite("plane", &Interp::_setup.plane)
	.def_readwrite("pockets_max", &Interp::_setup.pockets_max)
	.def_readwrite("probe_flag", &Interp::_setup.probe_flag)
	.def_readwrite("program_x", &Interp::_setup.program_x)
	.def_readwrite("program_y", &Interp::_setup.program_y)
	.def_readwrite("program_z", &Interp::_setup.program_z)
	.def_readwrite("random_toolchanger", &Interp::_setup.random_toolchanger)
	.def_readwrite("remap_level", &Interp::_setup.remap_level)
	.def_readwrite("retract_mode", &Interp::_setup.retract_mode)
	.def_readwrite("return_value", &Interp::_setup.return_value)
	.def_readwrite("value_returned", &Interp::_setup.value_returned)
	.def_readwrite("rotation_xy", &Interp::_setup.rotation_xy)
	.def_readwrite("selected_pocket", &Interp::_setup.selected_pocket)
	.def_readwrite("selected_tool", &Interp::_setup.selected_tool)
	.def_readwrite("sequence_number", &Interp::sequence_number)
	.def_readwrite("speed", &Interp::_setup.speed)
	.def_readwrite("speed_feed_mode", &Interp::_setup.speed_feed_mode)
	.def_readwrite("speed_override", &Interp::_setup.speed_override)
	.def_readwrite("spindle_mode", &Interp::_setup.spindle_mode)
	.def_readwrite("spindle_turning", &Interp::_setup.spindle_turning)
	.def_readwrite("tool_offset", &Interp::_setup.tool_offset)
	.def_readwrite("toolchange_flag", &Interp::_setup.toolchange_flag)
	.def_readwrite("traverse_rate", &Interp::_setup.traverse_rate)
	.def_readwrite("u_axis_offset", &Interp::_setup.u_axis_offset)
	.def_readwrite("u_current", &Interp::_setup.u_current)
	.def_readwrite("u_origin_offset", &Interp::_setup.u_origin_offset)
	.def_readwrite("v_axis_offset", &Interp::_setup.v_axis_offset)
	.def_readwrite("v_current", &Interp::_setup.v_current)
	.def_readwrite("v_origin_offset", &Interp::_setup.v_origin_offset)
	.def_readwrite("w_axis_offset", &Interp::_setup.w_axis_offset)
	.def_readwrite("w_current", &Interp::_setup.w_current)
	.def_readwrite("w_origin_offset", &Interp::_setup.w_origin_offset)
	.def_readwrite("remaps",  &Interp::_setup.remaps)

	.add_property( "params",
		       bp::make_function( &param_wrapper,
					  bp::with_custodian_and_ward_postcall< 0, 1 >()))


	// _setup arrays
	.add_property( "active_g_codes",
		       bp::make_function( active_g_codes_w(&active_g_codes_wrapper),
					  bp::with_custodian_and_ward_postcall< 0, 1 >()))
	.add_property( "active_m_codes",
		       bp::make_function( active_m_codes_w(&active_m_codes_wrapper),
					  bp::with_custodian_and_ward_postcall< 0, 1 >()))
	.add_property( "active_settings",
		       bp::make_function( active_settings_w(&active_settings_wrapper),
					  bp::with_custodian_and_ward_postcall< 0, 1 >()))
	.add_property( "blocks",
		       bp::make_function( blocks_w(&blocks_wrapper),
					  bp::with_custodian_and_ward_postcall< 0, 1 >()))
	.add_property( "parameters",
		       bp::make_function( parameters_w(&parameters_wrapper),
					  bp::with_custodian_and_ward_postcall< 0, 1 >()))
	.add_property( "tool_table",
		       bp::make_function( tool_table_w(&tool_table_wrapper),
					  bp::with_custodian_and_ward_postcall< 0, 1 >()))
	.add_property( "sub_context",
		       bp::make_function( sub_context_w(&sub_context_wrapper),
					  bp::with_custodian_and_ward_postcall< 0, 1 >()))
	;

    export_Arrays();
}
