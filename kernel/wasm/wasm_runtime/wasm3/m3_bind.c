//
//  m3_bind.c
//
//  Created by Steven Massey on 4/29/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#include "m3_env.h"
#include "m3_exception.h"
#include "m3_info.h"


/*@
  @ assigns \nothing;
  @*\/ */
u8  ConvertTypeCharToTypeId (char i_code)
{
    switch (i_code) {
    case 'v': return c_m3Type_none;
    case 'i': return c_m3Type_i32;
    case 'I': return c_m3Type_i64;
    case 'f': return c_m3Type_f32;
    case 'F': return c_m3Type_f64;
    case '*': return c_m3Type_i32;
    }
    return c_m3Type_unknown;
}


/*@
  @ requires o_functionType == \null || \valid(o_functionType);
  @ assigns \nothing;
  @*\/ */
M3Result  SignatureToFuncType  (IM3FuncType * o_functionType, ccstr_t i_signature)
{
    IM3FuncType funcType = NULL;

_try {
    print ("m3_bind: parsing signature '%s'\n", i_signature);
    if (not o_functionType)
        _throw ("null function type");

    if (not i_signature)
        _throw ("null function signature");

    cstr_t sig = i_signature;

    size_t maxNumTypes = strlen (i_signature);

    // assume min signature is "()"
    _throwif (m3Err_malformedFunctionSignature, maxNumTypes < 2);
    maxNumTypes -= 2;

    _throwif (m3Err_tooManyArgsRets, maxNumTypes > d_m3MaxSaneFunctionArgRetCount);

_   (AllocFuncType (& funcType, (u32) maxNumTypes));

    u8 * typelist = funcType->types;

    bool parsingRets = true;
    while (* sig)
    {
        char typeChar = * sig++;

        if (typeChar == '(')
        {
            parsingRets = false;
            continue;
        }
        else if ( typeChar == ' ')
            continue;
        else if (typeChar == ')')
            break;

        u8 type = ConvertTypeCharToTypeId (typeChar);

        _throwif ("unknown argument type char", c_m3Type_unknown == type);

        if (type == c_m3Type_none)
            continue;

        if (parsingRets)
        {
            _throwif ("malformed signature; return count overflow", funcType->numRets >= maxNumTypes);
            funcType->numRets++;
            *typelist++ = type;
        }
        else
        {
            _throwif ("malformed signature; arg count overflow", (u32)(funcType->numRets) + funcType->numArgs >= maxNumTypes);
            funcType->numArgs++;
            *typelist++ = type;
        }
    }

} _catch:

    if (result)
        m3_Free (funcType);

    * o_functionType = funcType;

    return result;
}


static
/*@
  @ assigns \nothing;
  @*\/ */
M3Result  ValidateSignature  (IM3Function i_function, ccstr_t i_linkingSignature)
{
    M3Result result = m3Err_none;

    IM3FuncType ftype = NULL;
_   (SignatureToFuncType (& ftype, i_linkingSignature));

    if (not AreFuncTypesEqual (ftype, i_function->funcType))
    {
        static const char *type_names[] = { "none", "i32", "i64", "f32", "f64" };
        print ("m3_bind: expected (");
          /*@ loop invariant 0 <= i <= ftype->numArgs;
    @ loop assigns i;
    @ loop variant ftype->numArgs - i;
    @*\/ */
  for (u32 i = 0; i < ftype->numArgs; ++i)
        {
            if (i != 0) print (", ");
            u8 t = d_FuncArgType (ftype, i);
            print ("%s", (t <= 4) ? type_names[t] : "?");
        }
        print (") -> ");
          /*@ loop invariant 0 <= i <= ftype->numRets;
    @ loop assigns i;
    @ loop variant ftype->numRets - i;
    @*\/ */
  for (u32 i = 0; i < ftype->numRets; ++i)
        {
            if (i != 0) print (", ");
            u8 t = d_FuncRetType (ftype, i);
            print ("%s", (t <= 4) ? type_names[t] : "?");
        }
        print ("\n");

        print ("m3_bind: found    (");
          /*@ loop invariant 0 <= i <= i_function->funcType->numArgs;
    @ loop assigns i;
    @ loop variant i_function->funcType->numArgs - i;
    @*\/ */
  for (u32 i = 0; i < i_function->funcType->numArgs; ++i)
        {
            if (i != 0) print (", ");
            u8 t = d_FuncArgType (i_function->funcType, i);
            print ("%s", (t <= 4) ? type_names[t] : "?");
        }
        print (") -> ");
          /*@ loop invariant 0 <= i <= i_function->funcType->numRets;
    @ loop assigns i;
    @ loop variant i_function->funcType->numRets - i;
    @*\/ */
  for (u32 i = 0; i < i_function->funcType->numRets; ++i)
        {
            if (i != 0) print (", ");
            u8 t = d_FuncRetType (i_function->funcType, i);
            print ("%s", (t <= 4) ? type_names[t] : "?");
        }
        print ("\n");

        _throw ("function signature mismatch");
    }

    _catch:

    m3_Free (ftype);

    return result;
}


M3Result  FindAndLinkFunction      (IM3Module       io_module,
                                    ccstr_t         i_moduleName,
                                    ccstr_t         i_functionName,
                                    ccstr_t         i_signature,
                                    voidptr_t       i_function,
                                    voidptr_t       i_userdata)
{
_try {
    _throwif(m3Err_moduleNotLinked, !io_module->runtime);

    const bool wildcardModule = (strcmp (i_moduleName, "*") == 0);

    result = m3Err_functionLookupFailed;

      /*@ loop invariant 0 <= i <= io_module->numFunctions;
    @ loop assigns i;
    @ loop variant io_module->numFunctions - i;
    @*\/ */
  for (u32 i = 0; i < io_module->numFunctions; ++i)
    {
        const IM3Function f = & io_module->functions [i];

        if (f->import.moduleUtf8 and f->import.fieldUtf8)
        {
            if (strcmp (f->import.fieldUtf8, i_functionName) == 0 and
               (wildcardModule or strcmp (f->import.moduleUtf8, i_moduleName) == 0))
            {
                if (i_signature) {
_                   (ValidateSignature (f, i_signature));
                }
_               (CompileRawFunction (io_module, f, i_function, i_userdata));
            }
        }
    }
} _catch:
    return result;
}

M3Result  m3_LinkRawFunctionEx  (IM3Module            io_module,
                                const char * const    i_moduleName,
                                const char * const    i_functionName,
                                const char * const    i_signature,
                                M3RawCall             i_function,
                                const void *          i_userdata)
{
    return FindAndLinkFunction (io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, i_userdata);
}

M3Result  m3_LinkRawFunction  (IM3Module            io_module,
                              const char * const    i_moduleName,
                              const char * const    i_functionName,
                              const char * const    i_signature,
                              M3RawCall             i_function)
{
    return FindAndLinkFunction (io_module, i_moduleName, i_functionName, i_signature, (voidptr_t)i_function, NULL);
}
