/* Generated code, do not edit. */
/* Generated for source version 2.3.2 */

#include "dwarf.h"
#include "libdwarf.h"
int
dwarf_language_version_data(
    Dwarf_Unsigned dw_lname,
    int *dw_default_lower_bound,
    const char   **dw_version_scheme)
{
    switch(dw_lname) {
    case DW_LNAME_Assembly:
    case DW_LNAME_BLISS:
    case DW_LNAME_C_sharp:
    case DW_LNAME_Crystal:
    case DW_LNAME_D:
    case DW_LNAME_Dylan:
    case DW_LNAME_Go:
    case DW_LNAME_HIP:
    case DW_LNAME_Haskell:
    case DW_LNAME_Hylo:
    case DW_LNAME_Java:
    case DW_LNAME_Kotlin:
    case DW_LNAME_Mojo:
    case DW_LNAME_OCaml:
    case DW_LNAME_OpenCL_C:
    case DW_LNAME_Python:
    case DW_LNAME_RenderScript:
    case DW_LNAME_Rust:
    case DW_LNAME_UPC:
    case DW_LNAME_Zig:
        *dw_default_lower_bound = 0;
        *dw_version_scheme = 0;
        return DW_DLV_OK;
    case DW_LNAME_Julia:
    case DW_LNAME_Modula2:
    case DW_LNAME_Modula3:
    case DW_LNAME_PLI:
        *dw_default_lower_bound = 1;
        *dw_version_scheme = 0;
        return DW_DLV_OK;
    case DW_LNAME_CPP_for_OpenCL:
    case DW_LNAME_OpenCL_CPP:
    case DW_LNAME_Swift:
        *dw_default_lower_bound = 0;
        *dw_version_scheme = "VVMM";
        return DW_DLV_OK;
    case DW_LNAME_GLSL:
    case DW_LNAME_GLSL_ES:
    case DW_LNAME_Gleam:
    case DW_LNAME_Metal:
    case DW_LNAME_Nim:
    case DW_LNAME_P4:
    case DW_LNAME_Ruby:
    case DW_LNAME_V:
        *dw_default_lower_bound = 0;
        *dw_version_scheme = "VVMMPP";
        return DW_DLV_OK;
    case DW_LNAME_Elixir:
    case DW_LNAME_Erlang:
        *dw_default_lower_bound = 1;
        *dw_version_scheme = "VVMMPP";
        return DW_DLV_OK;
    case DW_LNAME_HLSL:
        *dw_default_lower_bound = 0;
        *dw_version_scheme = "YYYY";
        return DW_DLV_OK;
    case DW_LNAME_Ada:
    case DW_LNAME_Algol68:
    case DW_LNAME_Cobol:
    case DW_LNAME_Fortran:
    case DW_LNAME_Pascal:
        *dw_default_lower_bound = 1;
        *dw_version_scheme = "YYYY";
        return DW_DLV_OK;
    case DW_LNAME_C:
    case DW_LNAME_C_plus_plus:
    case DW_LNAME_Move:
    case DW_LNAME_ObjC:
    case DW_LNAME_ObjC_plus_plus:
    case DW_LNAME_Odin:
        *dw_default_lower_bound = 0;
        *dw_version_scheme = "YYYYMM";
        return DW_DLV_OK;
    case DW_LNAME_SYCL:
        *dw_default_lower_bound = 0;
        *dw_version_scheme = "YYYYRR";
        return DW_DLV_OK;
    default:
        break;
    }
    return DW_DLV_NO_ENTRY;
}
