/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002-2007 Stefan Westerfeld, 2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __SFIDL_CBASE_H__
#define __SFIDL_CBASE_H__

#include <sfi/glib-extra.h>
#include <vector>
#include <map>
#include <iostream>
#include <algorithm>
#include "sfidl-utils.hh"
#include "sfidl-namespace.hh"
#include "sfidl-options.hh"
#include "sfidl-parser.hh"
#include "sfidl-generator.hh"

namespace Sfidl {
  
  /*
   * Base class for C and C++-like CodeGenerators
   */
  class CodeGeneratorCBase : public CodeGenerator {
  protected:
    bool generateBoxedTypes;

    enum TypeCodeModel {
      MODEL_FROM_VALUE, MODEL_TO_VALUE,
      MODEL_VCALL, MODEL_VCALL_ARG, 
      MODEL_VCALL_CARG, MODEL_VCALL_CONV, MODEL_VCALL_CFREE,
      MODEL_VCALL_RET, MODEL_VCALL_RCONV, MODEL_VCALL_RFREE
    };

    enum PrefixSymbolMode { generateOutput, generatePrefixSymbols };
    std::vector<String> prefix_symbols; /* symbols which should get a namespace prefix */

    const gchar *makeCStr (const String& str);

    String scatId (SfiSCategory c);

    /* record/sequence binding used by --host-c and --client-c binding */
    void printClientRecordPrototypes();
    void printClientSequencePrototypes();

    void printClientRecordDefinitions();
    void printClientSequenceDefinitions();

    void printClientRecordMethodPrototypes (PrefixSymbolMode mode);
    void printClientSequenceMethodPrototypes (PrefixSymbolMode mode);

    void printClientRecordMethodImpl();
    void printClientSequenceMethodImpl();

    void printClientChoiceDefinitions();
    void printClientChoiceConverterPrototypes (PrefixSymbolMode mode);

    void printProcedure (const Method& mdef, bool proto = false, const String& className = "");
    void printChoiceConverters ();

    virtual String makeProcName (const String& className, const String& procName);

    String makeGTypeName (const String& name);
    String makeParamSpec (const Param& pdef);
    String createTypeCode (const String& type, TypeCodeModel model);

    /*
     * data types: the following models deal with how to represent a certain
     * SFI type in the binding
     */

    // how "type" looks like when passed as argument to a function
    virtual String typeArg (const String& type);
    const gchar *cTypeArg (const String& type) { return makeCStr (typeArg (type)); }

    // how "type" looks like when stored as member in a struct or class
    virtual String typeField (const String& type);
    const gchar *cTypeField (const String& type) { return makeCStr (typeField (type)); }

    // how the return type of a function returning "type" looks like
    virtual String typeRet (const String& type);
    const gchar *cTypeRet (const String& type) { return makeCStr (typeRet (type)); }

    // how an array of "type"s looks like ( == MODEL_MEMBER + "*" ?)
    virtual String typeArray (const String& type);
    const gchar *cTypeArray (const String& type) { return makeCStr (typeArray (type)); }

    /*
     * function required to create a new "type" (blank return value allowed)
     * example: funcNew ("FBlock") => "sfi_fblock_new" (in C)
     */
    virtual String funcNew (const String& type);
    const gchar *cFuncNew (const String& type) { return makeCStr (funcNew (type)); }

    /*
     * function required to copy a "type" (blank return value allowed)
     * example: funcCopy ("FBlock") => "sfi_fblock_ref" (in C)
     */ 
    virtual String funcCopy (const String& type);
    const gchar *cFuncCopy (const String& type) { return makeCStr (funcNew (type)); }
    
    /*
     * function required to free a "type" (blank return value allowed)
     * example: funcFree ("FBlock") => "sfi_fblock_unref" (in C)
     */ 
    virtual String funcFree (const String& type);
    const gchar *cFuncFree (const String& type) { return makeCStr (funcNew (type)); }

    virtual String createTypeCode (const String& type, const String& name, 
				   TypeCodeModel model);

    CodeGeneratorCBase (const Parser& parser) : CodeGenerator (parser) {
      generateBoxedTypes = false;
    }
  };

};

#endif  /* __SFIDL_CBASE_H__ */

/* vim:set ts=8 sts=2 sw=2: */
