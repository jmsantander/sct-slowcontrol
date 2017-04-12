/* slow_control.i */
%module slow_control
%{
/* Include the headers in the wrapper code */
#include "interface_control.h"
#include "tm_control.h"
%}

/* Parse the headers to generate wrappers */
%include <std_string.i>
%include "interface_control.h"
%include "tm_control.h"
