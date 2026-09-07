/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
    yaml-scalar-util.h

    Shared scalar parsing utilities for YAML module

    Qore Programming Language

    Copyright 2003 - 2026 Qore Technologies, s.r.o.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _QORE_YAML_SCALAR_UTIL_H
#define _QORE_YAML_SCALAR_UTIL_H

#include <qore/Qore.h>
#include <yaml.h>

// Forward declarations
DLLLOCAL extern const char* QORE_YAML_DURATION_TAG;
DLLLOCAL extern const char* QORE_YAML_NUMBER_TAG;
DLLLOCAL extern const char* QORE_YAML_SQLNULL_TAG;

//! Parse a scalar value with an explicit YAML tag
/** @param val the scalar value as a string
    @param len the length of the value
    @param tag the YAML tag (e.g., YAML_INT_TAG, QORE_YAML_DURATION_TAG)
    @param xsink exception sink for error reporting
    @return the parsed value, or nothing on error
*/
DLLLOCAL QoreValue yaml_parse_tagged_scalar(const char* val, size_t len, const char* tag, ExceptionSink* xsink);

//! Parse a scalar value with type inference (no explicit tag)
/** @param val the scalar value as a string
    @param len the length of the value
    @param style the YAML scalar style (plain, quoted, etc.)
    @param favor_string if true, return string for ambiguous values
    @param xsink exception sink for error reporting
    @return the inferred value
*/
DLLLOCAL QoreValue yaml_parse_implicit_scalar(const char* val, size_t len, yaml_scalar_style_t style,
                                               bool favor_string, ExceptionSink* xsink);

//! Check if a value looks like an ISO 8601 absolute date/time
/** @param len the length of the value
    @param val the value string
    @param quoted true if the value was quoted in the YAML source
    @return true if the value appears to be an absolute date/time
*/
DLLLOCAL bool yaml_check_absolute_date(size_t len, const char* val, bool quoted = false);

//! Check if a value looks like an ISO 8601 duration (P[n]Y[n]M[n]DT[n]H[n]M[n]S)
/** @param val the value string
    @return true if the value appears to be a duration
*/
DLLLOCAL bool yaml_check_duration(const char* val);

//! Parse an ISO 8601 absolute date/time value
/** @param val the date/time string
    @param len the length of the string
    @param xsink exception sink for error reporting
    @return the parsed DateTimeNode
*/
DLLLOCAL DateTimeNode* yaml_parse_absolute_date(const char* val, size_t len, ExceptionSink* xsink);

//! Parse an ISO 8601 duration value
/** @param val the duration string
    @return the parsed DateTimeNode (relative)
*/
DLLLOCAL DateTimeNode* yaml_parse_duration(const char* val);

//! Try to parse a value as a number (int, float, or arbitrary precision)
/** Uses locale-independent parsing.
    @param val the value string
    @param len the length of the string
    @param no_simple_numeric if true, don't return simple int/float, only number
    @return the parsed number, or nothing if not a valid number
*/
DLLLOCAL QoreValue yaml_try_parse_number(const char* val, size_t len, bool no_simple_numeric = false);

//! Parse a float value with locale-independent handling
/** Handles YAML .inf / .nan spellings and Qore @inf@ / @nan@ special values.
    Finite values must have a decimal mantissa and optional exponent with digits in each.
    @param val the value string
    @param len the length of the string
    @param xsink exception sink for invalid float values
    @return the parsed double value
*/
DLLLOCAL double yaml_parse_float(const char* val, size_t len, ExceptionSink* xsink);

//! Parse an arbitrary-precision number value
/** Handles precision suffix in format: value{precision}
    @param val the value string
    @param len the length of the string
    @return the parsed QoreNumberNode
*/
DLLLOCAL QoreNumberNode* yaml_parse_number(const char* val, size_t len);

#endif // _QORE_YAML_SCALAR_UTIL_H
