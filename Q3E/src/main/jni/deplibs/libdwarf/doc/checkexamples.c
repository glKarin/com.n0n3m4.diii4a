/*! @file checkexamples.c
    @page checkexamples
    checkexamples.c contains what user code should be.
    Hence the code typed in checkexamples.c is PUBLIC DOMAIN
    and may be copied, used, and altered without any
    restrictions.

    This specific source file need not be compiled routinely
    nor should it ever be executed.

    To verify syntatic correctness compile in the
    libdwarf-code/doc directory with:

@code
cc -c -Wall -O0 -Wpointer-arith  \
-Wdeclaration-after-statement \
-Wextra -Wcomment -Wformat -Wpedantic -Wuninitialized \
-Wno-long-long -Wshadow -Wbad-function-cast \
-Wmissing-parameter-type -Wnested-externs \
-I../src/lib/libdwarf checkexamples.c
@endcode
*/

#include <stdio.h> /* for printf */
#include <stdlib.h> /* for free() */
#include <string.h> /* for memcmp() */
#include "dwarf.h"
#include "libdwarf.h"
/*  PRX is to match with the Dwarf_Unsigned datatype, an
    unsigned long long */
#define PRX  "0x%08llx"
#define TRUE  1
#define FALSE 0

/*! @defgroup exampleinit Using dwarf_init_path()

    @brief Example of a libdwarf initialization call.

    An example calling  dwarf_init_path() and dwarf_finish()
    @param path
    Path to an object we wish to open.
    @param groupnumber
    Desired groupnumber. Use DW_DW_GROUPNUMBER_ANY
    unless you have reason to do otherwise.
    @return
    Returns the applicable result. DW_DLV_OK etc.

    @code
*/
int exampleinit(const char *path, unsigned groupnumber)
{
    static char true_pathbuf[FILENAME_MAX];
    unsigned tpathlen = FILENAME_MAX;
    Dwarf_Handler errhand = 0;
    Dwarf_Ptr errarg = 0;
    Dwarf_Error error = 0;
    Dwarf_Debug dbg = 0;
    int res = 0;

    res = dwarf_init_path(path,true_pathbuf,
        tpathlen,groupnumber,errhand,
        errarg,&dbg, &error);
    if (res == DW_DLV_ERROR) {
        /*  Necessary call even though dbg is null!
            This avoids a memory leak.  */
        dwarf_dealloc_error(dbg,error);
        return res;
    }
    if (res == DW_DLV_NO_ENTRY) {
        /*  Nothing we can do */
        return res;
    }
    printf("The file we actually opened is %s\n",
        true_pathbuf);
    /* Call libdwarf functions here */
    dwarf_finish(dbg);
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup exampleinit_dl Using dwarf_init_path_dl()

    @brief Example focused on GNU debuglink data

    In case GNU debuglink data is followed the true_pathbuf
    content will not match path.
    The path actually used is copied to true_path_out.

    In the case of MacOS dSYM the true_path_out
    may not match path.

    If debuglink data is missing from the Elf executable
    or shared-object (ie, it is a normal
    object!) or unusable by libdwarf or
    true_path_buffer len is zero or true_path_out_buffer
    is zero libdwarf accepts the path given as the object
    to report on, no debuglink or dSYM processing will be used.

    @see https://sourceware.org/gdb/onlinedocs/gdb/Separate-Debug-Files.html

    An example calling  dwarf_init_path_dl() and dwarf_finish()

    @param path
    Path to an object we wish to open.
    @param groupnumber
    Desired groupnumber. Use DW_DW_GROUPNUMBER_ANY
    unless you have reason to do otherwise.
    @param error
    A pointer we can use to record error details.
    @return
    Returns the applicable result. DW_DLV_OK etc.

    @code
*/
int exampleinit_dl(const char *path, unsigned groupnumber,
    Dwarf_Error *error)
{
    static char true_pathbuf[FILENAME_MAX];
    static const char *glpath[3] = {
        "/usr/local/debug",
        "/usr/local/private/debug",
        "/usr/local/libdwarf/debug"
    };
    unsigned      tpathlen = FILENAME_MAX;
    Dwarf_Handler errhand = 0;
    Dwarf_Ptr     errarg = 0;
    Dwarf_Debug   dbg = 0;
    int           res = 0;
    unsigned char path_source = 0;

    res = dwarf_init_path_dl(path,true_pathbuf,
        tpathlen,groupnumber,errhand,
        errarg,&dbg,
        (char **)glpath,
        3,
        &path_source,
        error);
    if (res == DW_DLV_ERROR) {
        /*  We are not returning dbg, so we must do:
            dwarf_dealloc_error(dbg,*error);
            here to free the error details. */
        dwarf_dealloc_error(dbg,*error);
        *error = 0;
        return res;
    }
    if (res == DW_DLV_NO_ENTRY) {
        return res;
    }
    printf("The file we actually opened is %s\n",
        true_pathbuf);
    /* Call libdwarf functions here */
    dwarf_finish(dbg);
    return res;
}
/*! @endcode */

/*! @defgroup example1 Using dwarf_attrlist()

    @brief Example showing dwarf_attrlist()

    @param somedie
    Pass in any valid relevant DIE pointer.
    @param error
    An error pointer we can use.
    @return
    Return DW_DLV_OK (etc).

    @code
*/
int example1(Dwarf_Die somedie,Dwarf_Error *error)
{
    Dwarf_Debug dbg = 0;
    Dwarf_Signed atcount;
    Dwarf_Attribute *atlist;
    Dwarf_Signed i = 0;
    int errv;

    errv = dwarf_attrlist(somedie, &atlist,&atcount, error);
    if (errv != DW_DLV_OK) {
        return errv;
    }
    for (i = 0; i < atcount; ++i) {
        Dwarf_Half attrnum = 0;
        const char *attrname = 0;

        /*  use atlist[i], likely calling
            libdwarf functions and likely
            returning DW_DLV_ERROR if
            what you call gets DW_DLV_ERROR */
        errv = dwarf_whatattr(atlist[i],&attrnum,error);
        if (errv != DW_DLV_OK) {
            /* Something really bad happened. */
            return errv;
        }
        dwarf_get_AT_name(attrnum,&attrname);
        printf("Attribute[%ld], value %u name %s\n",
            (long int)i,attrnum,attrname);
        dwarf_dealloc_attribute(atlist[i]);
        atlist[i] = 0;
    }
    dwarf_dealloc(dbg, atlist, DW_DLA_LIST);
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup example2 Attaching a tied dbg

    @brief Example attaching base dbg to a split-DWARF object.

    See DWARF5 Appendix F on Split-DWARF.

    By libdwarf convention, open the split Dwarf_Debug using
    a dwarf_init call.  Then open
    the executable as the tied object.
    Then call dwarf_set_tied_dbg()
    so the library can look for relevant data
    in the tied-dbg (the executable).

    With split dwarf your libdwarf calls after
    the the initial open
    are done against the split Dwarf_Dbg and
    libdwarf automatically looks in the tied dbg
    when and as appropriate.
    the tied_dbg can be detached too, see
    example3 link, though you must call
    dwarf_finish() on the detached dw_tied_dbg,
    the library will not do that for you.

    @param split_dbg
    @param tied_dbg
    @param error
    @return
    Returns DW_DLV_OK  or DW_DLV_ERROR or
    DW_DLV_NO_ENTRY to the caller.

    @code
*/
int example2(Dwarf_Debug split_dbg, Dwarf_Debug tied_dbg,
    Dwarf_Error *error)
{
    int res = 0;

    /*  The caller should have opened dbg
        on the split-dwarf object/dwp,
        an object with DWARF, but no executable
        code.
        And it should have opened tieddbg on the
        runnable shared object or executable. */
    res = dwarf_set_tied_dbg(split_dbg,tied_dbg,error);
    /*  Let the caller (who initialized the dbg
        values) deal with doing dwarf_finish()
    */
    return res;
}
/*! @endcode */

/*! @defgroup example3 Detaching a tied dbg

    @brief Example detaching a tied (executable) dbg

    See DWARF5 Appendix F on Split-DWARF.

    With split dwarf your libdwarf calls after
    than the initial open
    are done against the split Dwarf_Dbg and
    libdwarf automatically looks in the open tied dbg
    when and as appropriate.
    the tied-dbg can be detached too, see
    example3 link, though you must call
    dwarf_finish() on the detached dw_tied_dbg,
    the library will not do that for you..

    @code
*/
int example3(Dwarf_Debug split_dbg,Dwarf_Error *error)
{
    int res = 0;
    res = dwarf_set_tied_dbg(split_dbg,NULL,error);
    if (res != DW_DLV_OK) {
        /* Something went wrong*/
        return res;
    }
    return res;
}
/*! @endcode */

/*! @defgroup examplesecgroup Examining Section Group data
    @brief Example accessing Section Group data

    With split dwarf your libdwarf calls after
    than the initial open
    are done against the base Dwarf_Dbg and
    libdwarf automatically looks in the open tied dbg
    when and as appropriate.
    the tied-dbg can be detached too, see
    example3 link, though you must call
    dwarf_finish() on the detached dw_tied_dbg,
    the library will not do that for you..

    Section groups apply to Elf COMDAT groups too.

    @code
*/
void examplesecgroup(Dwarf_Debug dbg)
{
    int res = 0;
    Dwarf_Unsigned  section_count = 0;
    Dwarf_Unsigned  group_count;
    Dwarf_Unsigned  selected_group = 0;
    Dwarf_Unsigned  group_map_entry_count = 0;
    Dwarf_Unsigned *sec_nums = 0;
    Dwarf_Unsigned *group_nums = 0;
    const char **   sec_names = 0;
    Dwarf_Error     error = 0;
    Dwarf_Unsigned  i = 0;

    res = dwarf_sec_group_sizes(dbg,&section_count,
        &group_count,&selected_group, &group_map_entry_count,
        &error);
    if (res != DW_DLV_OK) {
        /* Something is badly wrong*/
        return;
    }
    /*  In an object without split-dwarf sections
        or COMDAT sections we now have
        selected_group == 1. */
    sec_nums = calloc(group_map_entry_count,sizeof(Dwarf_Unsigned));
    if (!sec_nums) {
        /* FAIL. out of memory */
        return;
    }
    group_nums = calloc(group_map_entry_count,sizeof(Dwarf_Unsigned));
    if (!group_nums) {
        free(group_nums);
        /* FAIL. out of memory */
        return;
    }
    sec_names = calloc(group_map_entry_count,sizeof(char*));
    if (!sec_names) {
        free(group_nums);
        free(sec_nums);
        /* FAIL. out of memory */
        return;
    }

    res = dwarf_sec_group_map(dbg,group_map_entry_count,
        group_nums,sec_nums,sec_names,&error);
    if (res != DW_DLV_OK) {
        /* FAIL. Something badly wrong. */
        free(sec_names);
        free(group_nums);
        free(sec_nums);
    }
    for ( i = 0; i < group_map_entry_count; ++i) {
        /*  Now do something with
            group_nums[i],sec_nums[i],sec_names[i] */
    }
    /*  The strings are in Elf data.
        Do not free() the strings themselves.*/
    free(sec_names);
    free(group_nums);
    free(sec_nums);
}
/*! @endcode */

/*! @defgroup example4c Using dwarf_siblingof_c()
    @brief Example accessing a DIE sibling.

    Access to each DIE on a sibling list.
    This is the preferred form as it is
    slightly more efficient than
    dwarf_siblingof_b().

    @code
*/
int example4c(Dwarf_Die in_die,
    Dwarf_Error *error)
{
    Dwarf_Die return_sib = 0;
    int res = 0;

    /* in_die must be a valid Dwarf_Die */
    res = dwarf_siblingof_c(in_die,&return_sib, error);
    if (res == DW_DLV_OK) {
        /* Use return_sib here. */
        dwarf_dealloc_die(return_sib);
        /*  return_sib is no longer usable for anything, we
            ensure we do not use it accidentally with: */
        return_sib = 0;
        return res;
    }
    return res;
}
/*! @endcode */

/*! @defgroup example4b Using dwarf_siblingof_b()
    @brief Example accessing a DIE sibling.

    Access to each DIE on a sibling list
    This is the older form, required after
    dwarf_next_cu_header_d().

    Better to use dwarf_next_cu_header_e() and
    dwarf_siblingof_c().

    @code
*/
int example4b(Dwarf_Debug dbg,Dwarf_Die in_die,
    Dwarf_Bool is_info,
    Dwarf_Error *error)
{
    Dwarf_Die return_sib = 0;
    int res = 0;

    /*  in_die might be NULL following a call
        to dwarf_next_cu_header_d()
        or a valid Dwarf_Die */
    res = dwarf_siblingof_b(dbg,in_die,is_info,&return_sib, error);
    if (res == DW_DLV_OK) {
        /* Use return_sib here. */
        dwarf_dealloc_die(return_sib);
        /*  return_sib is no longer usable for anything, we
            ensure we do not use it accidentally with: */
        return_sib = 0;
        return res;
    }
    return res;
}
/*! @endcode */

/*! @defgroup example5 Using dwarf_child()
    @brief Example accessing a DIE child

    If the DIE has children (for example
    inner scopes in a function or members of
    a struct) this retrieves the DIE which
    appears first.  The child itself
    may have its own sibling chain.

    @code
*/
void example5(Dwarf_Die in_die)
{
    Dwarf_Die return_kid = 0;
    Dwarf_Error error = 0;
    int res = 0;

    res = dwarf_child(in_die,&return_kid, &error);
    if (res == DW_DLV_OK) {
        /* Use return_kid here. */
        dwarf_dealloc_die(return_kid);
        /*  The original form of dealloc still works
            dwarf_dealloc(dbg, return_kid, DW_DLA_DIE);
            */
        /*  return_kid is no longer usable for anything, we
            ensure we do not use it accidentally with: */
        return_kid = 0;
    }
}
/*! @endcode */

/*! @defgroup example_sibvalid using dwarf_validate_die_sibling
    @brief Example of a DIE tree validation.

    Here we show how one uses
    dwarf_validate_die_sibling().
    Dwarfdump uses this function as a part of its
    validation of DIE trees.

    It is not something you need to use.
    But one must use it in a specific pattern
    for it to work properly.

    dwarf_validate_die_sibling() depends on data set
    by dwarf_child() preceeding dwarf_siblingof_b() .
    dwarf_child() records a little bit
    of information invisibly in the Dwarf_Debug data.

    @code
*/

int example_sibvalid(Dwarf_Debug dbg,
    Dwarf_Die in_die,
    Dwarf_Error*error)
{
    int        cres = DW_DLV_OK;
    int        sibres = DW_DLV_OK;
    Dwarf_Die  die = 0;
    Dwarf_Die  sibdie = 0;
    Dwarf_Die  child = 0;
    Dwarf_Bool is_info = dwarf_get_die_infotypes_flag(die);

    die = in_die;
    for ( ; die ; die = sibdie) {
        int vres = 0;
        Dwarf_Unsigned offset = 0;

        /* Maybe print something you extract from the DIE */
        cres = dwarf_child(die,&child,error);
        if (cres == DW_DLV_ERROR) {
            if (die != in_die) {
                dwarf_dealloc_die(die);
            }
            printf("dwarf_child ERROR\n");
            return DW_DLV_ERROR;
        }
        if (cres == DW_DLV_OK) {
            int lres = 0;

            child = 0;
            lres = example_sibvalid(dbg,child,error);
            if (lres == DW_DLV_ERROR) {
                if (die != in_die) {
                    dwarf_dealloc_die(die);
                }
                dwarf_dealloc_die(child);
                printf("example_sibvalid ERROR\n");
                return lres;
            }
        }
        sibdie = 0;
        sibres = dwarf_siblingof_b(dbg,die,is_info,
            &sibdie,error);
        if (sibres == DW_DLV_ERROR) {
            if (die != in_die) {
                dwarf_dealloc_die(die);
            }
            if (child) {
                dwarf_dealloc_die(child);
            }
            printf("dwarf_siblingof_b ERROR\n");
            return DW_DLV_ERROR;
        }
        if (sibres == DW_DLV_NO_ENTRY) {
            if (die != in_die) {
                dwarf_dealloc_die(die);
            }
            if (child) {
                dwarf_dealloc_die(child);
            }
            return DW_DLV_OK;
        }
        vres = dwarf_validate_die_sibling(sibdie,&offset);
        if (vres == DW_DLV_ERROR) {
            if (die != in_die) {
                dwarf_dealloc_die(die);
            }
            if (child) {
                dwarf_dealloc_die(child);
            }
            dwarf_dealloc_die(sibdie);
            printf("Invalid sibling DIE\n");
            return DW_DLV_ERROR;
        }
        /*  loop again */
        if (die != in_die) {
            dwarf_dealloc_die(die);
        }
        die = 0;
    }
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup examplecuhdre Example walking CUs(e)
    @brief Example examining CUs looking for specific items(e).

    Loops through as many CUs as needed, stops
    and returns once a CU provides the desired data.

    Assumes certain functions you write to remember
    the aspect of CUs that matter to you so once found
    in a cu my_needed_data_exists() or some other
    function of yours can identify the correct record.

    Depending on your goals in examining the DIE tree
    it may be helpful to maintain a DIE stack
    of active DIEs, pushing and popping as you
    make your way throught the DIE levels.

    We assume that on a serious error we will give up
    (for simplicity here).

    We assume the caller  to examplecuhdre() will
    know what to retrieve (when we return DW_DLV_OK
    from examplecuhdree() and that myrecords points
    to a record with all the data needed by
    my_needed_data_exists() and
    recorded by myrecord_data_for_die().

    @code
*/

struct myrecords_struct *myrecords;
void myrecord_data_for_die(struct myrecords_struct *myrecords_data,
    Dwarf_Die d)
{
    /* do somthing */
    /*  avoid compiler warnings */
    (void)myrecords_data;
    (void)d;
}
int  my_needed_data_exists(struct myrecords_struct *myrecords_data)
{
    /* do something */
    /*  avoid compiler warnings */
    (void)myrecords_data;
    return DW_DLV_OK;
}

/*  Loop on DIE tree.  */
static void
record_die_and_siblings_e(Dwarf_Debug dbg, Dwarf_Die in_die,
    int is_info, int in_level,
    struct myrecords_struct *myrec,
    Dwarf_Error *error)
{
    int       res = DW_DLV_OK;
    Dwarf_Die cur_die=in_die;
    Dwarf_Die child = 0;

    myrecord_data_for_die(myrec,in_die);

    /*   Loop on a list of siblings */
    for (;;) {
        Dwarf_Die sib_die = 0;

        /*  Depending on your goals, the in_level,
            and the DW_TAG of cur_die, you may want
            to skip the dwarf_child call. We descend
            the DWARF-standard way of depth-first. */
        res = dwarf_child(cur_die,&child,error);
        if (res == DW_DLV_ERROR) {
            printf("Error in dwarf_child , level %d \n",in_level);
            exit(EXIT_FAILURE);
        }
        if (res == DW_DLV_OK) {
            record_die_and_siblings_e(dbg,child,is_info,
                in_level+1,myrec,error);
            /* No longer need 'child' die. */
            dwarf_dealloc(dbg,child,DW_DLA_DIE);
            child = 0;
        }
        /* res == DW_DLV_NO_ENTRY or DW_DLV_OK */
        res = dwarf_siblingof_c(cur_die,&sib_die,error);
        if (res == DW_DLV_ERROR) {
            exit(EXIT_FAILURE);
        }
        if (res == DW_DLV_NO_ENTRY) {
            /* Done at this level. */
            break;
        }
        /* res == DW_DLV_OK */
        if (cur_die != in_die) {
            dwarf_dealloc(dbg,cur_die,DW_DLA_DIE);
            cur_die = 0;
        }
        cur_die = sib_die;
        myrecord_data_for_die(myrec,sib_die);
    }
    return;
}

/*  Assuming records properly initialized for your use. */
int examplecuhdre(Dwarf_Debug dbg,
    struct myrecords_struct *myrec,
    Dwarf_Error *error)
{
    Dwarf_Unsigned abbrev_offset = 0;
    Dwarf_Half     address_size = 0;
    Dwarf_Half     version_stamp = 0;
    Dwarf_Half     offset_size = 0;
    Dwarf_Half     extension_size = 0;
    Dwarf_Sig8     signature;
    Dwarf_Unsigned typeoffset = 0;
    Dwarf_Unsigned next_cu_header = 0;
    Dwarf_Half     header_cu_type = 0;
    Dwarf_Bool     is_info = TRUE;
    int            res = 0;

    while(!my_needed_data_exists(myrec)) {
        Dwarf_Die cu_die = 0;
        Dwarf_Unsigned cu_header_length = 0;

        memset(&signature,0, sizeof(signature));
        res = dwarf_next_cu_header_e(dbg,is_info,
            &cu_die,
            &cu_header_length,
            &version_stamp, &abbrev_offset,
            &address_size, &offset_size,
            &extension_size,&signature,
            &typeoffset, &next_cu_header,
            &header_cu_type,error);
        if (res == DW_DLV_ERROR) {
            return res;
        }
        if (res == DW_DLV_NO_ENTRY) {
            if (is_info == TRUE) {
                /*  Done with .debug_info, now check for
                    .debug_types. */
                is_info = FALSE;
                continue;
            }
            /*  No more CUs to read! Never found
                what we were looking for in either
                .debug_info or .debug_types. */
            return res;
        }
        /*  We have the cu_die .
            New in v0.9.0 because the connection of
            the CU_DIE to the CU header is clear
            in the argument list.
            */
        record_die_and_siblings_e(dbg,cu_die,is_info,
            0, myrec,error);
        dwarf_dealloc_die(cu_die);
    }
    /*  Found what we looked for */
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup examplecuhdrd Example walking CUs(d)

    @brief Example accessing all CUs looking for specific items(d).

    Loops through as many CUs as needed, stops
    and returns once a CU provides the desired data.

    Assumes certain functions you write to remember
    the aspect of CUs that matter to you so once found
    in a cu my_needed_data_exists() or some other
    function of yours can identify the correct record.
    (Possibly a DIE global offset. Remember to note
    if each DIE has is_info TRUE or FALSE so libdwarf
    can find the DIE properly.)

    Depending on your goals in examining the DIE tree
    it may be helpful to maintain a DIE stack
    of active DIEs, pushing and popping as you
    make your way throught the DIE levels.

    We assume that on a serious error we will give up
    (for simplicity here).

    We assume the caller  to examplecuhdrd() will
    know what to retrieve (when we return DW_DLV_OK
    from examplecuhdrd() and that myrecords points
    to a record with all the data needed by
    my_needed_data_exists() and
    recorded by myrecord_data_for_die().

    @code
*/
struct myrecords_struct *myrecords;
void myrecord_data_for_die(struct myrecords_struct *myrecords,
    Dwarf_Die d);
int  my_needed_data_exists(struct myrecords_struct *myrecords);

/*  Loop on DIE tree. */
static void
record_die_and_siblingsd(Dwarf_Debug dbg, Dwarf_Die in_die,
    int is_info, int in_level,
    struct myrecords_struct *myrec,
    Dwarf_Error *error)
{
    int       res = DW_DLV_OK;
    Dwarf_Die cur_die=in_die;
    Dwarf_Die child = 0;

    myrecord_data_for_die(myrec,in_die);

    /*   Loop on a list of siblings */
    for (;;) {
        Dwarf_Die sib_die = 0;

        /*  Depending on your goals, the in_level,
            and the DW_TAG of cur_die, you may want
            to skip the dwarf_child call. */
        res = dwarf_child(cur_die,&child,error);
        if (res == DW_DLV_ERROR) {
            printf("Error in dwarf_child , level %d \n",in_level);
            exit(EXIT_FAILURE);
        }
        if (res == DW_DLV_OK) {
            record_die_and_siblingsd(dbg,child,is_info,
                in_level+1,myrec,error);
            /* No longer need 'child' die. */
            dwarf_dealloc(dbg,child,DW_DLA_DIE);
            child = 0;
        }
        /* res == DW_DLV_NO_ENTRY or DW_DLV_OK */
        res = dwarf_siblingof_b(dbg,cur_die,is_info,&sib_die,error);
        if (res == DW_DLV_ERROR) {
            exit(EXIT_FAILURE);
        }
        if (res == DW_DLV_NO_ENTRY) {
            /* Done at this level. */
            break;
        }
        /* res == DW_DLV_OK */
        if (cur_die != in_die) {
            dwarf_dealloc(dbg,cur_die,DW_DLA_DIE);
            cur_die = 0;
        }
        cur_die = sib_die;
        myrecord_data_for_die(myrec,sib_die);
    }
    return;
}

/*  Assuming records properly initialized for your use. */
int examplecuhdrd(Dwarf_Debug dbg,
    struct myrecords_struct *myrec,
    Dwarf_Error *error)
{
    Dwarf_Unsigned abbrev_offset = 0;
    Dwarf_Half     address_size = 0;
    Dwarf_Half     version_stamp = 0;
    Dwarf_Half     offset_size = 0;
    Dwarf_Half     extension_size = 0;
    Dwarf_Sig8     signature;
    Dwarf_Unsigned typeoffset = 0;
    Dwarf_Unsigned next_cu_header = 0;
    Dwarf_Half     header_cu_type = 0;
    Dwarf_Bool     is_info = TRUE;
    int            res = 0;

    while(!my_needed_data_exists(myrec)) {
        Dwarf_Die no_die = 0;
        Dwarf_Die cu_die = 0;
        Dwarf_Unsigned cu_header_length = 0;

        memset(&signature,0, sizeof(signature));
        res = dwarf_next_cu_header_d(dbg,is_info,&cu_header_length,
            &version_stamp, &abbrev_offset,
            &address_size, &offset_size,
            &extension_size,&signature,
            &typeoffset, &next_cu_header,
            &header_cu_type,error);
        if (res == DW_DLV_ERROR) {
            return res;
        }
        if (res == DW_DLV_NO_ENTRY) {
            if (is_info == TRUE) {
                /*  Done with .debug_info, now check for
                    .debug_types. */
                is_info = FALSE;
                continue;
            }
            /*  No more CUs to read! Never found
                what we were looking for in either
                .debug_info or .debug_types. */
            return res;
        }
        /*  The CU will have a single sibling, a cu_die.
            It is essential to call this right after
            a call to dwarf_next_cu_header_d() because
            there is no explicit connection provided to
            dwarf_siblingof_b(), which returns a DIE
            from whatever CU was last accessed by
            dwarf_next_cu_header_d()!
            The lack of explicit connection was a
            design mistake in the API (made in 1992). */

        res = dwarf_siblingof_b(dbg,no_die,is_info,
            &cu_die,error);
        if (res == DW_DLV_ERROR) {
            return res;
        }
        if (res == DW_DLV_NO_ENTRY) {
            /*  Impossible */
            exit(EXIT_FAILURE);
        }
        record_die_and_siblingsd(dbg,cu_die,is_info,
            0, myrec,error);
        dwarf_dealloc_die(cu_die);
    }
    /*  Found what we looked for */
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup example6 Using dwarf_offdie_b()

    @brief Example accessing a DIE by its offset

    @code
*/
int example6(Dwarf_Debug dbg,Dwarf_Off die_offset,
    Dwarf_Bool is_info,
    Dwarf_Error *error)
{
    Dwarf_Die return_die = 0;
    int res = 0;

    res = dwarf_offdie_b(dbg,die_offset,is_info,&return_die, error);
    if (res != DW_DLV_OK) {
        /*  res could be NO ENTRY or ERROR, so no
            dealloc necessary.  */
        return res;
    }
    /* Use return_die here. */
    dwarf_dealloc_die(return_die);
    /*  return_die is no longer usable for anything, we
        ensure we do not use it accidentally
        though a bit silly here given the return_die
        goes out of scope... */
    return_die = 0;
    return res;
}
/*! @endcode */

/*! @defgroup example7 Using dwarf_offset_given_die()
    @brief Example finding the section offset of a DIE

    Here finding the offset of a CU-DIE.

    @code
*/
int example7(Dwarf_Debug dbg, Dwarf_Die in_die,
    Dwarf_Bool    is_info,
    Dwarf_Error * error)
{
    int res = 0;
    Dwarf_Off cudieoff = 0;
    Dwarf_Die cudie = 0;

    res = dwarf_CU_dieoffset_given_die(in_die,&cudieoff,error);
    if (res != DW_DLV_OK) {
        /*  FAIL */
        return res;
    }
    res = dwarf_offdie_b(dbg,cudieoff,is_info,&cudie,error);
    if (res != DW_DLV_OK) {
        /* FAIL */
        return res;
    }
    /* do something with cu_die */
    dwarf_dealloc_die(cudie);
    return res;
}
/*! @endcode */

/* See also example1, which is more complete */
/*! @defgroup example8 Using  dwarf_attrlist()
    @brief Example Calling dwarf_attrlist()

    @code
*/
int example8(Dwarf_Debug dbg, Dwarf_Die somedie, Dwarf_Error *error)
{
    Dwarf_Signed atcount = 0;
    Dwarf_Attribute *atlist = 0;
    int errv = 0;
    Dwarf_Signed i = 0;

    errv = dwarf_attrlist(somedie, &atlist,&atcount, error);
    if (errv != DW_DLV_OK) {
        return errv;
    }
    for (i = 0; i < atcount; ++i) {
        /* use atlist[i] */
        dwarf_dealloc_attribute(atlist[i]);
        atlist[i] = 0;
    }
    dwarf_dealloc(dbg, atlist, DW_DLA_LIST);
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup exampleoffset_list Using dwarf_offset_list()
    @brief Example using dwarf_offset_list

    An example calling  dwarf_offset_list

    @param dbg
    the Dwarf_Debug of interest
    @param dieoffset
    The section offset of a Dwarf_Die
    @param is_info
    Pass in TRUE if the dieoffset is for the
    .debug_info section, else pass in FALSE meaning
    the dieoffset is for the DWARF4 .debug_types section.
    @param error
    The usual error detail return.
    @return
    Returns DW_DLV_OK etc
    @code
*/
int exampleoffset_list(Dwarf_Debug dbg, Dwarf_Off dieoffset,
    Dwarf_Bool is_info,Dwarf_Error * error)
{
    Dwarf_Unsigned offcnt = 0;
    Dwarf_Off *offbuf = 0;
    int errv = 0;
    Dwarf_Unsigned i = 0;

    errv = dwarf_offset_list(dbg,dieoffset, is_info,
        &offbuf,&offcnt, error);
    if (errv != DW_DLV_OK) {
        return errv;
    }
    for (i = 0; i < offcnt; ++i) {
        /* use offbuf[i] */
        /*  No need to free the offbuf entry, it
            is just an offset value. */
    }
    dwarf_dealloc(dbg, offbuf, DW_DLA_LIST);
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup explainformblock Documenting Form_Block
    @brief Example documents Form_Block content

    Used with certain location information functions,
    a frame expression function, expanded
    frame instructions, and
    DW_FORM_block<> functions and more.

    @see dwarf_formblock
    @see Dwarf_Block_s

    @code
    struct Dwarf_Block_s fields {

    Dwarf_Unsigned  bl_len;
        Length of block bl_data points at

    Dwarf_Ptr       bl_data;
        Uninterpreted data bytes

    Dwarf_Small     bl_from_loclist;
        See libdwarf.h DW_LKIND, defaults to
        DW_LKIND_expression and except in certain
        location expressions the field is ignored.

    Dwarf_Unsigned  bl_section_offset;
        Section offset of what bl_data points to
    @endcode
*/

/*! @defgroup examplediscrlist Using dwarf_discr_list()
    @brief Example using dwarf_discr_list, dwarf_formblock

    An example calling dwarf_get_form_class,
    dwarf_discr_list, and dwarf_formblock.
    and the dwarf_deallocs applicable.

    @see dwarf_discr_list
    @see dwarf_get_form_class
    @see dwarf_formblock

    @param dw_dbg
    The applicable Dwarf_Debug
    @param dw_die
    The applicable Dwarf_Die
    @param dw_attr
    The applicable Dwarf_Attribute
    @param dw_attrnum,
    The attribute number passed in to shorten
    this example a bit.
    @param dw_isunsigned,
    The attribute number passed in to shorten
    this example a bit.
    @param dw_theform,
    The form number passed in to shorten
    this example a bit.
    @param dw_error
    The usual error pointer.
    @return
    Returns DW_DLV_OK etc
    @code
*/
int example_discr_list(Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Attribute attr,
    Dwarf_Half attrnum,
    Dwarf_Bool isunsigned,
    Dwarf_Half theform,
    Dwarf_Error *error)
{
    /*  The example here assumes that
        attribute attr is a DW_AT_discr_list.
        isunsigned should be set from the signedness
        of the parent of 'die' per DWARF rules for
        DW_AT_discr_list. */
    enum Dwarf_Form_Class fc = DW_FORM_CLASS_UNKNOWN;
    Dwarf_Half version = 0;
    Dwarf_Half offset_size = 0;
    int wres = 0;

    wres = dwarf_get_version_of_die(die,&version,&offset_size);
    if (wres != DW_DLV_OK) {
        /* FAIL */
        return wres;
    }
    fc = dwarf_get_form_class(version,attrnum,offset_size,theform);
    if (fc == DW_FORM_CLASS_BLOCK) {
        int fres = 0;
        Dwarf_Block *tempb = 0;
        fres = dwarf_formblock(attr, &tempb, error);
        if (fres == DW_DLV_OK) {
            Dwarf_Dsc_Head h = 0;
            Dwarf_Unsigned u = 0;
            Dwarf_Unsigned arraycount = 0;
            int sres = 0;

            sres = dwarf_discr_list(dbg,
                (Dwarf_Small *)tempb->bl_data,
                tempb->bl_len,
                &h,&arraycount,error);
            if (sres == DW_DLV_NO_ENTRY) {
                /* Nothing here. */
                dwarf_dealloc(dbg, tempb, DW_DLA_BLOCK);
                return sres;
            }
            if (sres == DW_DLV_ERROR) {
                /* FAIL . */
                dwarf_dealloc(dbg, tempb, DW_DLA_BLOCK);
                return sres ;
            }
            for (u = 0; u < arraycount; u++) {
                int u2res = 0;
                Dwarf_Half dtype = 0;
                Dwarf_Signed dlow = 0;
                Dwarf_Signed dhigh = 0;
                Dwarf_Unsigned ulow = 0;
                Dwarf_Unsigned uhigh = 0;

                if (isunsigned) {
                    u2res = dwarf_discr_entry_u(h,u,
                        &dtype,&ulow,&uhigh,error);
                } else {
                    u2res = dwarf_discr_entry_s(h,u,
                        &dtype,&dlow,&dhigh,error);
                }
                if (u2res == DW_DLV_ERROR) {
                    /* Something wrong */
                    dwarf_dealloc(dbg,h,DW_DLA_DSC_HEAD);
                    dwarf_dealloc(dbg, tempb, DW_DLA_BLOCK);
                    return u2res ;
                }
                if (u2res == DW_DLV_NO_ENTRY) {
                    /* Impossible. u < arraycount. */
                    dwarf_dealloc(dbg,h,DW_DLA_DSC_HEAD);
                    dwarf_dealloc(dbg, tempb, DW_DLA_BLOCK);
                    return u2res;
                }
                /*  Do something with dtype, and whichever
                    of ulow, uhigh,dlow,dhigh got set.
                    Probably save the values somewhere.
                    Simple casting of dlow to ulow (or vice versa)
                    will not get the right value due to the nature
                    of LEB values. Similarly for uhigh, dhigh.
                    One must use the right call.  */
            }
            dwarf_dealloc(dbg,h,DW_DLA_DSC_HEAD);
            dwarf_dealloc(dbg, tempb, DW_DLA_BLOCK);
        }
    }
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup example_loclistcv5 Location/expression access
    @brief Example using DWARF2-5 loclists and loc-expressions

    Valid for DWARF2 and later DWARF.

    This example simply *assumes* the attribute
    has a form which relates to location lists
    or location expressions. Use dwarf_get_form_class()
    to determine if this attribute fits.
    Use dwarf_get_version_of_die() to help get the
    data you need.
    @see dwarf_get_form_class
    @see dwarf_get_version_of_die
    @see example_locexprc

    @code
*/
int example_loclistcv5(Dwarf_Attribute someattr,
    Dwarf_Error *error)
{
    Dwarf_Unsigned lcount = 0;
    Dwarf_Loc_Head_c loclist_head = 0;
    int lres = 0;

    lres = dwarf_get_loclist_c(someattr,&loclist_head,
        &lcount,error);
    if (lres == DW_DLV_OK) {
        Dwarf_Unsigned i = 0;

        /*  Before any return remember to call
            dwarf_loc_head_c_dealloc(loclist_head); */
        for (i = 0; i < lcount; ++i) {
            Dwarf_Small loclist_lkind = 0;
            Dwarf_Small lle_value = 0;
            Dwarf_Unsigned rawval1 = 0;
            Dwarf_Unsigned rawval2 = 0;
            Dwarf_Bool debug_addr_unavailable = FALSE;
            Dwarf_Addr lopc = 0;
            Dwarf_Addr hipc = 0;
            Dwarf_Unsigned loclist_expr_op_count = 0;
            Dwarf_Locdesc_c locdesc_entry = 0;
            Dwarf_Unsigned expression_offset = 0;
            Dwarf_Unsigned locdesc_offset = 0;

            lres = dwarf_get_locdesc_entry_d(loclist_head,
                i,
                &lle_value,
                &rawval1,&rawval2,
                &debug_addr_unavailable,
                &lopc,&hipc,
                &loclist_expr_op_count,
                &locdesc_entry,
                &loclist_lkind,
                &expression_offset,
                &locdesc_offset,
                error);
            if (lres == DW_DLV_OK) {
                Dwarf_Unsigned j = 0;
                int opres = 0;
                Dwarf_Small op = 0;

                for (j = 0; j < loclist_expr_op_count; ++j) {
                    Dwarf_Unsigned opd1 = 0;
                    Dwarf_Unsigned opd2 = 0;
                    Dwarf_Unsigned opd3 = 0;
                    Dwarf_Unsigned offsetforbranch = 0;

                    opres = dwarf_get_location_op_value_c(
                        locdesc_entry, j,&op,
                        &opd1,&opd2,&opd3,
                        &offsetforbranch,
                        error);
                    if (opres == DW_DLV_OK) {
                        /*  Do something with the operators.
                            Usually you want to use opd1,2,3
                            as appropriate. Calculations
                            involving base addresses etc
                            have already been incorporated
                            in opd1,2,3.  */
                    } else {
                        dwarf_dealloc_loc_head_c(loclist_head);
                        /*Something is wrong. */
                        return opres;
                    }
                }
            } else {
                /* Something is wrong. Do something. */
                dwarf_dealloc_loc_head_c(loclist_head);
                return lres;
            }
        }
    }
    /*  Always call dwarf_loc_head_c_dealloc()
        to free all the memory associated with loclist_head.  */
    dwarf_dealloc_loc_head_c(loclist_head);
    loclist_head = 0;
    return lres;
}
/*! @endcode */

/*! @defgroup example_locexprc Reading a location expression
    @brief Example getting details of a location expression

    @see example_loclistcv5

    @code
*/
int example_locexprc(Dwarf_Debug dbg,Dwarf_Ptr expr_bytes,
    Dwarf_Unsigned expr_len,
    Dwarf_Half addr_size,
    Dwarf_Half offset_size,
    Dwarf_Half version,
    Dwarf_Error*error)
{
    Dwarf_Loc_Head_c head = 0;
    Dwarf_Locdesc_c  locentry = 0;
    int            res2 = 0;
    Dwarf_Unsigned rawlopc = 0;
    Dwarf_Unsigned rawhipc = 0;
    Dwarf_Bool     debug_addr_unavail = FALSE;
    Dwarf_Unsigned lopc = 0;
    Dwarf_Unsigned hipc = 0;
    Dwarf_Unsigned ulistlen = 0;
    Dwarf_Unsigned ulocentry_count = 0;
    Dwarf_Unsigned section_offset = 0;
    Dwarf_Unsigned locdesc_offset = 0;
    Dwarf_Small    lle_value = 0;
    Dwarf_Small    loclist_source = 0;
    Dwarf_Unsigned i = 0;

    res2 = dwarf_loclist_from_expr_c(dbg,
        expr_bytes,expr_len,
        addr_size,
        offset_size,
        version,
        &head,
        &ulistlen,
        error);
    if (res2 != DW_DLV_OK) {
        return res2;
    }
    /*  These are a location expression, not loclist.
        So we just need the 0th entry. */
    res2 = dwarf_get_locdesc_entry_d(head,
        0, /* Data from 0th because it is a loc expr,
            there is no list */
        &lle_value,
        &rawlopc, &rawhipc, &debug_addr_unavail, &lopc, &hipc,
        &ulocentry_count, &locentry,
        &loclist_source, &section_offset, &locdesc_offset,
        error);
    if (res2 == DW_DLV_ERROR) {
        dwarf_dealloc_loc_head_c(head);
        return res2;
    } else if (res2 == DW_DLV_NO_ENTRY) {
        dwarf_dealloc_loc_head_c(head);
        return res2;
    }
    /*  ASSERT: ulistlen == 1 */
    for (i = 0; i < ulocentry_count;++i) {
        Dwarf_Small op = 0;
        Dwarf_Unsigned opd1 = 0;
        Dwarf_Unsigned opd2 = 0;
        Dwarf_Unsigned opd3 = 0;
        Dwarf_Unsigned offsetforbranch = 0;

        res2 = dwarf_get_location_op_value_c(locentry,
            i, &op,&opd1,&opd2,&opd3,
            &offsetforbranch,
            error);
        /* Do something with the expression operator and operands */
        if (res2 != DW_DLV_OK) {
            dwarf_dealloc_loc_head_c(head);
            return res2;
        }
    }
    dwarf_dealloc_loc_head_c(head);
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup examplec Using dwarf_srclines_b()
    @brief Example using dwarf_srclines_b()

    An example calling dwarf_srclines_b

    dwarf_srclines_dealloc_b
    dwarf_srclines_from_linecontext
    dwarf_srclines_files_indexes
    dwarf_srclines_files_data_b
    dwarf_srclines_two_level_from_linecontext

    @param path
    Path to an object we wish to open.
    @param error
    Allows passing back error details to the caller.
    @return
    Return DW_DLV_OK etc.

    @code
*/
int examplec(Dwarf_Die cu_die,Dwarf_Error *error)
{
    /* EXAMPLE: DWARF2-DWARF5  access.  */
    Dwarf_Line    *linebuf = 0;
    Dwarf_Signed   linecount = 0;
    Dwarf_Line    *linebuf_actuals = 0;
    Dwarf_Signed   linecount_actuals = 0;
    Dwarf_Line_Context line_context = 0;
    Dwarf_Small    table_count = 0;
    Dwarf_Unsigned lineversion = 0;
    int sres = 0;
    /* ... */
    /*  we use 'return' here to signify we can do nothing more
        at this point in the code. */
    sres = dwarf_srclines_b(cu_die,&lineversion,
        &table_count,&line_context,error);
    if (sres != DW_DLV_OK) {
        /*  Handle the DW_DLV_NO_ENTRY  or DW_DLV_ERROR
            No memory was allocated so there nothing
            to dealloc here. */
        return sres;
    }
    if (table_count == 0) {
        /*  A line table with no actual lines.  */
        /*...do something, see dwarf_srclines_files_count()
            etc below. */

        dwarf_srclines_dealloc_b(line_context);
        /*  All the memory is released, the line_context
            and linebuf zeroed now
            as a reminder they are stale. */
        linebuf = 0;
        line_context = 0;
    } else if (table_count == 1) {
        Dwarf_Signed i = 0;
        Dwarf_Signed baseindex = 0;
        Dwarf_Signed file_count = 0;
        Dwarf_Signed endindex = 0;
        /*  Standard dwarf 2,3,4, or 5 line table */
        /*  Do something. */

        /*  First let us index through all the files listed
            in the line table header. */
        sres = dwarf_srclines_files_indexes(line_context,
            &baseindex,&file_count,&endindex,error);
        if (sres != DW_DLV_OK) {
            /* Something badly wrong! */
            return sres;
        }
        /*  Works for DWARF2,3,4 (one-based index)
            and DWARF5 (zero-based index) */
        for (i = baseindex; i < endindex; i++) {
            Dwarf_Unsigned dirindex = 0;
            Dwarf_Unsigned modtime = 0;
            Dwarf_Unsigned flength = 0;
            Dwarf_Form_Data16 *md5data = 0;
            int vres = 0;
            const char *name = 0;

            vres = dwarf_srclines_files_data_b(line_context,i,
                &name,&dirindex, &modtime,&flength,
                &md5data,error);
            if (vres != DW_DLV_OK) {
                /* something very wrong. */
                return vres;
            }
            /* do something */
        }

        /*  For this case where we have a line table we will likely
            wish to get the line details: */
        sres = dwarf_srclines_from_linecontext(line_context,
            &linebuf,&linecount,
            error);
        if (sres != DW_DLV_OK) {
            /* Error. Clean up the context information. */
            dwarf_srclines_dealloc_b(line_context);
            return sres;
        }
        /* The lines are normal line table lines. */
        for (i = 0; i < linecount; ++i) {
            /* use linebuf[i] */
        }
        dwarf_srclines_dealloc_b(line_context);
        /*  All the memory is released, the line_context
            and linebuf zeroed now as a reminder they are stale */
        linebuf = 0;
        line_context = 0;
        linecount = 0;
    } else {
        Dwarf_Signed i = 0;
        /*  ASSERT: table_count == 2,
            Experimental two-level line table. Version 0xf006
            We do not define the meaning of this non-standard
            set of tables here. */

        /*  For 'something C' (two-level line tables)
            one codes something like this
            Note that we do not define the meaning or
            use of two-level line
            tables as these are experimental, not standard DWARF. */
        sres = dwarf_srclines_two_level_from_linecontext(line_context,
            &linebuf,&linecount,
            &linebuf_actuals,&linecount_actuals,
            error);
        if (sres == DW_DLV_OK) {
            for (i = 0; i < linecount; ++i) {
                /*  use linebuf[i], these are the 'logicals'
                    entries. */
            }
            for (i = 0; i < linecount_actuals; ++i) {
                /*  use linebuf_actuals[i], these are the
                    actuals entries */
            }
            dwarf_srclines_dealloc_b(line_context);
            line_context = 0;
            linebuf = 0;
            linecount = 0;
            linebuf_actuals = 0;
            linecount_actuals = 0;
        } else if (sres == DW_DLV_NO_ENTRY) {
            /* This should be impossible, but do something.   */
            /* Then Free the line_context */
            dwarf_srclines_dealloc_b(line_context);
            line_context = 0;
            linebuf = 0;
            linecount = 0;
            linebuf_actuals = 0;
            linecount_actuals = 0;
        } else {
            /*  ERROR, show the error or something.
                Free the line_context. */
            dwarf_srclines_dealloc_b(line_context);
            line_context = 0;
            linebuf = 0;
            linecount = 0;
            linebuf_actuals = 0;
            linecount_actuals = 0;
        }
    }
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup exampled Using dwarf_srclines_b() and linecontext
    @brief Example two using dwarf_srclines_b(), dwarf_linesrc().
    @see dwarf_srclines_b
    @see dwarf_srclines_from_linecontext
    @see dwarf_srclines_dealloc_b

    @code
*/
int exampled(Dwarf_Debug dbg,Dwarf_Die somedie,Dwarf_Error *error)
{
    Dwarf_Signed       count = 0;
    Dwarf_Line_Context context = 0;
    Dwarf_Line        *linebuf = 0;
    Dwarf_Signed       i = 0;
    Dwarf_Line        *line;
    Dwarf_Small        table_count =0;
    Dwarf_Unsigned     version = 0;
    int                sres = 0;

    sres = dwarf_srclines_b(somedie,
        &version, &table_count,&context,error);
    if (sres != DW_DLV_OK) {
        return sres;
    }
    sres = dwarf_srclines_from_linecontext(context,
        &linebuf,&count,error);
    if (sres != DW_DLV_OK) {
        dwarf_srclines_dealloc_b(context);
        return sres;
    }
    line = linebuf;
    for (i = 0; i < count; ++line,++i) {
        char * filename = 0;
        int lres = 0;
        Dwarf_Line dline = linebuf[i];

        lres = dwarf_linesrc(dline,&filename,error);
        if (lres != DW_DLV_OK) {
            dwarf_srclines_dealloc_b(context);
            return lres;
        }
        /* use filename */
        dwarf_dealloc(dbg, filename, DW_DLA_STRING);
    }
    dwarf_srclines_dealloc_b(context);
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup examplee Using dwarf_srcfiles()
    @brief Example getting source file names given a DIE
    @code
*/
int examplee(Dwarf_Debug dbg,Dwarf_Die somedie,Dwarf_Error *error)
{
    /*  It is an annoying historical mistake in libdwarf
        that the count is a signed value. */
    Dwarf_Signed       count = 0;
    char             **srcfiles = 0;
    Dwarf_Signed       i = 0;
    int                res = 0;
    Dwarf_Line_Context line_context = 0;
    Dwarf_Small        table_count = 0;
    Dwarf_Unsigned     lineversion = 0;

    res = dwarf_srclines_b(somedie,&lineversion,
        &table_count,&line_context,error);
    if (res != DW_DLV_OK) {
        /*  dwarf_finish() will dealloc srcfiles, not doing
            that here.  */
        return res;
    }
    res = dwarf_srcfiles(somedie, &srcfiles,&count,error);
    if (res != DW_DLV_OK) {
        dwarf_srclines_dealloc_b(line_context);
        return res;
    }

    for (i = 0; i < count; ++i) {
        Dwarf_Signed propernumber = 0;

        /*  Use srcfiles[i] If you  wish to print 'i'
            mostusefully
            you should reflect the numbering that
            a DW_AT_decl_file attribute would report in
            this CU. */
        if (lineversion ==  5) {
            propernumber = i;
        } else {
            propernumber = i+1;
        }
        printf("File %4ld %s\n",(unsigned long)propernumber,
            srcfiles[i]);
        dwarf_dealloc(dbg, srcfiles[i], DW_DLA_STRING);
        srcfiles[i] = 0;
    }
    /*  We could leave all dealloc to dwarf_finish() to
        handle, but this tidies up sooner. */
    dwarf_dealloc(dbg, srcfiles, DW_DLA_LIST);
    dwarf_srclines_dealloc_b(line_context);
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup examplef Using dwarf_get_globals()
    @brief Example using global symbol names

    For 0.4.2 and earlier this returned .debug_pubnames
    content.
    As of version 0.5.0 (October 2022) this  returns
    .debug_pubnames  (if it exists) and the relevant
    portion of  .debug_names (if .debug_names exists) data.

    @code
*/
int examplef(Dwarf_Debug dbg,Dwarf_Error *error)
{
    Dwarf_Signed  count = 0;
    Dwarf_Global *globs = 0;
    Dwarf_Signed  i = 0;
    int           res = 0;

    res = dwarf_get_globals(dbg, &globs,&count, error);
    if (res != DW_DLV_OK) {
        return res;
    }
    for (i = 0; i < count; ++i) {
        /* use globs[i] */
        char *name = 0;
        res = dwarf_globname(globs[i],&name,error);
        if (res != DW_DLV_OK) {
            dwarf_globals_dealloc(dbg,globs,count);
            return res;
        }
    }
    dwarf_globals_dealloc(dbg, globs, count);
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup exampleg Using dwarf_globals_by_type()
    @brief Example reading .debug_pubtypes

    The .debug_pubtypes section was in DWARF4,
    it could appear as an extension
    in other DWARF versions..
    In libdwarf 0.5.0 and earlier the function
    dwarf_get_pubtypes() was used instead.

    @code
*/
int exampleg(Dwarf_Debug dbg, Dwarf_Error *error)
{
    Dwarf_Signed  count = 0;
    Dwarf_Global *types = 0;
    Dwarf_Signed  i = 0;
    int           res = 0;

    res = dwarf_globals_by_type(dbg,DW_GL_PUBTYPES,
        &types,&count,error);
    /*  Alternatively the 0.5.0 and earlier call:
        res=dwarf_get_pubtypes(dbg, &types,&count, error); */
    if (res != DW_DLV_OK) {
        return res;
    }
    for (i = 0; i < count; ++i) {
        /* use types[i] */
    }
    dwarf_globals_dealloc(dbg, types, count);
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup exampleh Reading .debug_weaknames (nonstandard)
    @brief Example. weaknames was IRIX/MIPS only

    This section is an SGI/MIPS extension, not created
    by modern compilers.

    @code
*/
int exampleh(Dwarf_Debug dbg,Dwarf_Error *error)
{
    Dwarf_Signed  count = 0;
    Dwarf_Global *weaks = 0;
    Dwarf_Signed  i = 0;
    int           res = 0;

    res = dwarf_globals_by_type(dbg,DW_GL_WEAKS,
        &weaks,&count,error);
    if (res != DW_DLV_OK) {
        return res;
    }
    for (i = 0; i < count; ++i) {
        /* use weaks[i] */
    }
    dwarf_globals_dealloc(dbg, weaks, count);
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup examplej Reading .debug_funcnames (nonstandard)
    @brief Example. funcnames was IRIX/MIPS only

    This section is an SGI/MIPS extension, not created
    by modern compilers.

    @code
*/
int examplej(Dwarf_Debug dbg, Dwarf_Error*error)
{
    Dwarf_Signed  count = 0;
    Dwarf_Global *funcs = 0;
    Dwarf_Signed  i = 0;
    int           fres = 0;

    fres = dwarf_globals_by_type(dbg,DW_GL_FUNCS,
        &funcs,&count,error);
    if (fres != DW_DLV_OK) {
        return fres;
    }
    for (i = 0; i < count; ++i) {
        /* use funcs[i] */
    }
    dwarf_globals_dealloc(dbg, funcs, count);
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup examplel Reading .debug_types (nonstandard)
    @brief Example .debug_types was IRIX/MIPS only

    This section is an SGI/MIPS extension, not created
    by modern compilers.

    @code
*/
int examplel(Dwarf_Debug dbg, Dwarf_Error *error)
{
    Dwarf_Signed  count = 0;
    Dwarf_Global *types = 0;
    Dwarf_Signed  i = 0;
    int           res = 0;

    res = dwarf_globals_by_type(dbg,DW_GL_TYPES,
        &types,&count,error);
    if (res != DW_DLV_OK) {
        return res;
    }
    for (i = 0; i < count; ++i) {
        /* use types[i] */
    }
    dwarf_globals_dealloc(dbg, types, count);
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup examplen Reading .debug_varnames data (nonstandard)
    @brief Example .debug_varnames was IRIX/MIPS only

    This section is an SGI/MIPS extension, not created
    by modern compilers.

    @code
*/
int examplen(Dwarf_Debug dbg,Dwarf_Error *error)
{
    Dwarf_Signed  count = 0;
    Dwarf_Global *vars = 0;
    Dwarf_Signed  i = 0;
    int           res = 0;

    res = dwarf_globals_by_type(dbg,DW_GL_VARS,
        &vars,&count,error);
    if (res != DW_DLV_OK) {
        return res;
    }
    for (i = 0; i < count; ++i) {
        /* use vars[i] */
    }
    dwarf_globals_dealloc(dbg, vars, count);
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup exampledebugnames Reading .debug_names data

    @brief Example access to .debug_names

    This is accessing DWARF5 .debug_names, a section
    intended to provide fast access to DIEs.

    It bears a strong resemblance to what libdwarf does
    in dwarf_global.c.

    Making this a single (long) function here, though that is
    not how libdwarf or dwarfdump are written.

    That is just one possible sort of access. There are many,
    and we would love to hear suggestions for specific
    new API functions in the library.

    There is a wealth of information in .debug_names
    and the following is all taken care of for you
    by dwarf_get_globals().

    @code
*/
#define MAXPAIRS 8 /* The standard defines 5.*/
int exampledebugnames(Dwarf_Debug dbg,
    Dwarf_Unsigned *dnentrycount,
    Dwarf_Error *error)
{
    int               res = DW_DLV_OK;
    Dwarf_Unsigned    offset = 0;
    Dwarf_Dnames_Head dn  = 0;
    Dwarf_Unsigned    new_offset = 0;

    for (   ;res == DW_DLV_OK; offset = new_offset) {
        Dwarf_Unsigned comp_unit_count = 0;
        Dwarf_Unsigned local_type_unit_count = 0;
        Dwarf_Unsigned foreign_type_unit_count = 0;
        Dwarf_Unsigned bucket_count = 0;
        Dwarf_Unsigned name_count = 0;
        Dwarf_Unsigned abbrev_table_size = 0;
        Dwarf_Unsigned entry_pool_size = 0;
        Dwarf_Unsigned augmentation_string_size = 0;
        char          *aug_string = 0;
        Dwarf_Unsigned section_size = 0;
        Dwarf_Half     table_version = 0;
        Dwarf_Half     offset_size = 0;
        Dwarf_Unsigned i = 0;

        res = dwarf_dnames_header(dbg,offset,&dn,
            &new_offset,error);
        if (res == DW_DLV_ERROR) {
            /*  Something wrong. */
            return res;
        }
        if (res == DW_DLV_NO_ENTRY) {
            /*  Done. Normal end of the .debug_names section. */
            break;
        }
        *dnentrycount += 1;

        res = dwarf_dnames_sizes(dn,&comp_unit_count,
            &local_type_unit_count,
            &foreign_type_unit_count,
            &bucket_count,
            &name_count,&abbrev_table_size,
            &entry_pool_size,&augmentation_string_size,
            &aug_string,
            &section_size,&table_version,
            &offset_size,
            error);
        if (res != DW_DLV_OK) {
            /*  Something wrong. */
            return res;
        }
        /*  name indexes start with one */
        for (i = 1 ; i <= name_count; ++i) {
            Dwarf_Unsigned j = 0;
            /* dnames_name data */
            Dwarf_Unsigned bucketnum = 0;
            Dwarf_Unsigned hashvalunsign = 0;
            Dwarf_Unsigned offset_to_debug_str = 0;
            char          *ptrtostr          = 0;
            Dwarf_Unsigned offset_in_entrypool = 0;
            Dwarf_Unsigned abbrev_code = 0;
            Dwarf_Half     abbrev_tag    = 0;
            Dwarf_Half     nt_idxattr_array[MAXPAIRS];
            Dwarf_Half     nt_form_array[MAXPAIRS];
            Dwarf_Unsigned attr_count = 0;

            /* dnames_entrypool data */
            Dwarf_Half     tag         = 0;
            Dwarf_Bool     single_cu_case = 0;
            Dwarf_Unsigned single_cu_offset = 0;
            Dwarf_Unsigned value_count = 0;
            Dwarf_Unsigned index_of_abbrev = 0;
            Dwarf_Unsigned offset_of_initial_value = 0;
            Dwarf_Unsigned offset_next_entry_pool = 0;
            Dwarf_Half     idx_array[MAXPAIRS];
            Dwarf_Half     form_array[MAXPAIRS];
            Dwarf_Unsigned offsets_array[MAXPAIRS];
            Dwarf_Sig8     signatures_array[MAXPAIRS];

            Dwarf_Unsigned cu_table_index = 0;
            Dwarf_Unsigned tu_table_index = 0;
            Dwarf_Unsigned local_die_offset = 0;
            Dwarf_Unsigned parent_index = 0;
            Dwarf_Sig8     parenthash;

            (void)parent_index;     /* avoids warning */
            (void)local_die_offset; /* avoids warning */
            (void)tu_table_index;   /* avoids warning */
            (void)cu_table_index;   /* avoids warning */

            memset(&parenthash,0,sizeof(parenthash));
            /*  This gets us the entry pool offset we need.
                we provide idxattr and nt_form arrays (need
                not be initialized) and on return
                attr_count of those arrays are filled in.
                if attr_count < array_size then array_size
                is too small and things will not go well!
                See the count of DW_IDX entries in dwarf.h
                and make the arrays (say) 2 or more larger
                ensuring against future new DW_IDX index
                attributes..

                ptrtostring is the name in the Names Table. */
            res = dwarf_dnames_name(dn,i,
                &bucketnum, &hashvalunsign,
                &offset_to_debug_str,&ptrtostr,
                &offset_in_entrypool, &abbrev_code,
                &abbrev_tag,
                MAXPAIRS,
                nt_idxattr_array, nt_form_array,
                &attr_count,error);
            if (res == DW_DLV_NO_ENTRY) {
                /* past end. Normal. */
                break;
            }
            if (res == DW_DLV_ERROR) {
                dwarf_dealloc_dnames(dn);
                return res;
            }

            /*  Check attr_count < MAXPAIRS ! */
            /*  Now check the value of TAG to ensure it
                is something of interest as data or function.
                Plausible choices: */
            switch (abbrev_tag) {
            case DW_TAG_subprogram:
            case DW_TAG_variable:
            case DW_TAG_label:
            case DW_TAG_member:
            case DW_TAG_common_block:
            case DW_TAG_enumerator:
            case DW_TAG_namelist:
            case DW_TAG_module:
                break;
            default:
                /*  Not data or variable DIE involved.
                    Loop on the next i */
                continue;
            }

            /*  We need the number of values for this name
                from this call. tag will match abbrev_tag.  */
            res = dwarf_dnames_entrypool(dn,
                offset_in_entrypool,
                &abbrev_code,&tag,&value_count,&index_of_abbrev,
                &offset_of_initial_value,
                error);
            if (res != DW_DLV_OK) {
                dwarf_dealloc_dnames(dn);
                return res;
            }

            /*  This gets us an actual array of values
                as the library combines abbreviations,
                IDX attributes and values. We use
                the idx_array and form_array data
                created above. */

            res = dwarf_dnames_entrypool_values(dn,
                index_of_abbrev,
                offset_of_initial_value,
                value_count,
                idx_array,
                form_array,
                offsets_array,
                signatures_array,
                &single_cu_case,&single_cu_offset,
                &offset_next_entry_pool,
                error);
            if (res != DW_DLV_OK) {
                dwarf_dealloc_dnames(dn);
                return res;
            }
            for (j = 0; j < value_count; ++j) {
                Dwarf_Half idx = idx_array[j];

                switch(idx) {
                case DW_IDX_compile_unit:
                    cu_table_index = offsets_array[j];
                    break;
                case DW_IDX_die_offset:
                    local_die_offset = offsets_array[j];
                    break;
                /* The following are not meaninful when
                    reading globals. */
                case DW_IDX_type_unit:
                    tu_table_index = offsets_array[j];
                    break;
                case DW_IDX_parent:
                    parent_index = offsets_array[j];
                    break;
                case DW_IDX_type_hash:
                    parenthash = signatures_array[j];
                    break;
                default:
                    /* Not handled DW_IDX_GNU... */
                    break;
                }
            }
            /*  Now do something with the data aggregated */

        }
        dwarf_dealloc_dnames(dn);
    }
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup examplep5 Reading .debug_macro data (DWARF5)

    @brief Example reading DWARF5 macro data

    This builds an list or some other data structure
    (not defined) to give an import somewhere to list
    the import offset and then later to enquire
    if the list has unexamined offsets.
    The code compiles but is not yet tested.

    This example does not actually do the import at
    the correct time as this is just checking
    import offsets, not creating a proper full
    list (in the proper order) of the
    macros with the imports inserted.
    Here we find the macro context for a DIE,
        report those macro entries, noting
        any macro_import in a list
        loop extracting unchecked macro offsets from the list
            note any import in a list
    Of course some functions are not implemented here...

    @code
*/
int   has_unchecked_import_in_list(void)
{
    /* Do something */
    return DW_DLV_OK;
}
Dwarf_Unsigned get_next_import_from_list(void)
{
    /* Do something */
    return 22;
}
void  mark_this_offset_as_examined(
    Dwarf_Unsigned macro_unit_offset)
{
    /* do something */
    /*  avoid compiler warnings. */
    (void)macro_unit_offset;
}
void  add_offset_to_list(Dwarf_Unsigned offset)
{
    /* do something */
    /*  avoid compiler warnings. */
    (void)offset;;
}
int  examplep5(Dwarf_Die cu_die,Dwarf_Error *error)
{
    int lres = 0;
    Dwarf_Unsigned      k = 0;
    Dwarf_Unsigned      version = 0;
    Dwarf_Macro_Context macro_context = 0;
    Dwarf_Unsigned      macro_unit_offset = 0;
    Dwarf_Unsigned      number_of_ops = 0;
    Dwarf_Unsigned      ops_total_byte_len = 0;
    Dwarf_Bool          is_primary = TRUE;

    /*  Just call once each way to test both.
        Really the second is just for imported units.*/
    for ( ; ; ) {
        if (is_primary) {
            lres = dwarf_get_macro_context(cu_die,
                &version,&macro_context,
                &macro_unit_offset,
                &number_of_ops,
                &ops_total_byte_len,
                error);
            is_primary = FALSE;
        } else {
            if (has_unchecked_import_in_list()) {
                macro_unit_offset = get_next_import_from_list();
            } else {
                /* We are done */
                break;
            }
            lres = dwarf_get_macro_context_by_offset(cu_die,
                macro_unit_offset,
                &version,
                &macro_context,
                &number_of_ops,
                &ops_total_byte_len,
                error);
            mark_this_offset_as_examined(macro_unit_offset);
        }

        if (lres == DW_DLV_ERROR) {
            /* Something is wrong. */
            return lres;
        }
        if (lres == DW_DLV_NO_ENTRY) {
            /* We are done. */
            break;
        }
        /* lres ==  DW_DLV_OK) */
        for (k = 0; k < number_of_ops; ++k) {
            Dwarf_Unsigned  section_offset = 0;
            Dwarf_Half      macro_operator = 0;
            Dwarf_Half      forms_count = 0;
            const Dwarf_Small *formcode_array = 0;
            Dwarf_Unsigned  line_number = 0;
            Dwarf_Unsigned  index = 0;
            Dwarf_Unsigned  offset =0;
            const char    * macro_string =0;
            int lres2 = 0;

            lres2 = dwarf_get_macro_op(macro_context,
                k, &section_offset,&macro_operator,
                &forms_count, &formcode_array,error);
            if (lres2 != DW_DLV_OK) {
                /* Some error. Deal with it */
                dwarf_dealloc_macro_context(macro_context);
                return lres2;
            }
            switch(macro_operator) {
            case 0:
                /* Nothing to do. */
                break;
            case DW_MACRO_end_file:
                /* Do something */
                break;
            case DW_MACRO_define:
            case DW_MACRO_undef:
            case DW_MACRO_define_strp:
            case DW_MACRO_undef_strp:
            case DW_MACRO_define_strx:
            case DW_MACRO_undef_strx:
            case DW_MACRO_define_sup:
            case DW_MACRO_undef_sup: {
                lres2 = dwarf_get_macro_defundef(macro_context,
                    k,
                    &line_number,
                    &index,
                    &offset,
                    &forms_count,
                    &macro_string,
                    error);
                if (lres2 != DW_DLV_OK) {
                    /* Some error. Deal with it */
                    dwarf_dealloc_macro_context(macro_context);
                    return lres2;
                }
                /* do something */
                }
                break;
            case DW_MACRO_start_file: {
                lres2 = dwarf_get_macro_startend_file(macro_context,
                    k,&line_number,
                    &index,
                    &macro_string,error);
                if (lres2 != DW_DLV_OK) {
                    /* Some error. Deal with it */
                    dwarf_dealloc_macro_context(macro_context);
                    return lres2;
                }
                /* do something */
                }
                break;
            case DW_MACRO_import: {
                lres2 = dwarf_get_macro_import(macro_context,
                    k,&offset,error);
                if (lres2 != DW_DLV_OK) {
                    /* Some error. Deal with it */
                    dwarf_dealloc_macro_context(macro_context);
                    return lres2;
                }
                add_offset_to_list(offset);
                }
                break;
            case DW_MACRO_import_sup: {
                lres2 = dwarf_get_macro_import(macro_context,
                    k,&offset,error);
                if (lres2 != DW_DLV_OK) {
                    /* Some error. Deal with it */
                    dwarf_dealloc_macro_context(macro_context);
                    return lres2;
                }
                /* do something */
                }
                break;
            default:
                /*  This is an error or an omission
                    in the code here.  We do not
                    know what to do.
                    Do something appropriate, print something?. */
                break;
            }
        }
        dwarf_dealloc_macro_context(macro_context);
        macro_context = 0;
    }
    return DW_DLV_OK;
}
/* @endcode */

/*! @defgroup examplep2 Reading .debug_macinfo (DWARF2-4)
    @brief Example reading .debug_macinfo, DWARF2-4

    @code
*/

void functionusingsigned(Dwarf_Signed s) {
    /* Do something */
    /* Avoid compiler warnings. */
    (void)s;
}

int examplep2(Dwarf_Debug dbg, Dwarf_Off cur_off,
    Dwarf_Error*error)
{
    Dwarf_Signed         count = 0;
    Dwarf_Macro_Details *maclist = 0;
    Dwarf_Signed         i = 0;
    Dwarf_Unsigned       max = 500000; /* sanity limit */
    int errv = 0;

    /*  This is for DWARF2,DWARF3, and DWARF4
        .debug_macinfo section only.*/
    /*  Given an offset from a compilation unit,
        start at that offset (from DW_AT_macroinfo)
        and get its macro details. */
    errv = dwarf_get_macro_details(dbg, cur_off,max,
        &count,&maclist,error);
    if (errv == DW_DLV_OK) {
        for (i = 0; i < count; ++i) {
            Dwarf_Macro_Details *  mentry = maclist +i;
            /* example of use */
            Dwarf_Signed lineno = mentry->dmd_lineno;
            functionusingsigned(lineno);
        }
        dwarf_dealloc(dbg, maclist, DW_DLA_STRING);
    }
    /*  Loop through all the compilation units macro info from zero.
        This is not guaranteed to work because DWARF does not
        guarantee every byte in the section is meaningful:
        there can be garbage between the macro info
        for CUs.  But this loop will sometimes work.
    */
    cur_off = 0;
    while((errv = dwarf_get_macro_details(dbg, cur_off,max,
        &count,&maclist,error))== DW_DLV_OK) {
        for (i = 0; i < count; ++i) {
            Dwarf_Macro_Details *  mentry = maclist +i;
            /* example of use */
            Dwarf_Signed lineno = mentry->dmd_lineno;
            functionusingsigned(lineno);
        }
        cur_off = maclist[count-1].dmd_offset + 1;
        dwarf_dealloc(dbg, maclist, DW_DLA_STRING);
    }
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup exampleq Extracting fde, cie lists.
    @brief Example Opening FDE and CIE lists

    @code
*/
int exampleq(Dwarf_Debug dbg,Dwarf_Error *error)
{
    Dwarf_Cie   *cie_data = 0;
    Dwarf_Signed cie_count = 0;
    Dwarf_Fde   *fde_data = 0;
    Dwarf_Signed fde_count = 0;
    int          fres = 0;

    fres = dwarf_get_fde_list(dbg,&cie_data,&cie_count,
        &fde_data,&fde_count,error);
    if (fres != DW_DLV_OK) {
        return fres;
    }
    /*  Do something with the lists*/
    dwarf_dealloc_fde_cie_list(dbg, cie_data, cie_count,
        fde_data,fde_count);
    return fres;
}
/*! @endcode */

/*! @defgroup exampler Reading the .eh_frame section
    @brief Example access to .eh_frame

    @code
*/
int exampler(Dwarf_Debug dbg,Dwarf_Addr mypcval,Dwarf_Error *error)
{
    /*  Given a pc value
        for a function find the FDE and CIE data for
        the function.
        Example shows basic access to FDE/CIE plus
        one way to access details given a PC value.
        dwarf_get_fde_n() allows accessing all FDE/CIE
        data so one could build up an application-specific
        table of information if that is more useful.  */
    Dwarf_Cie   *cie_data = 0;
    Dwarf_Signed cie_count = 0;
    Dwarf_Fde   *fde_data = 0;
    Dwarf_Signed fde_count = 0;
    int          fres = 0;

    fres = dwarf_get_fde_list_eh(dbg,&cie_data,&cie_count,
        &fde_data,&fde_count,error);
    if (fres == DW_DLV_OK) {
        Dwarf_Fde myfde = 0;
        Dwarf_Addr low_pc = 0;
        Dwarf_Addr high_pc = 0;

        fres = dwarf_get_fde_at_pc(fde_data,mypcval,
            &myfde,&low_pc,&high_pc,
            error);
        if (fres == DW_DLV_OK) {
            Dwarf_Cie mycie = 0;
            fres = dwarf_get_cie_of_fde(myfde,&mycie,error);
            if (fres == DW_DLV_ERROR) {
                return fres;
            }
            if (fres == DW_DLV_OK) {
                /*  Now we can access a range of information
                    about the fde and cie applicable. */
            }
        }
        dwarf_dealloc_fde_cie_list(dbg, cie_data, cie_count,
            fde_data,fde_count);
        return fres;
    }
    return fres;
}
/*! @endcode */

/*! @defgroup examples Using  dwarf_expand_frame_instructions
    @brief Example using dwarf_expand_frame_instructions

    @code
*/
int examples(Dwarf_Cie cie,
    Dwarf_Ptr instruction,Dwarf_Unsigned len,
    Dwarf_Error *error)
{
    Dwarf_Frame_Instr_Head head = 0;
    Dwarf_Unsigned         count = 0;
    int                    res = 0;
    Dwarf_Unsigned         i = 0;

    res = dwarf_expand_frame_instructions(cie,instruction,len,
        &head,&count, error);
    if (res != DW_DLV_OK) {
        return res;
    }

    for (i = 0; i < count; ++i) {
        Dwarf_Unsigned  instr_offset_in_instrs = 0;
        Dwarf_Small     cfa_operation          = 0;
        const char     *fields_description     = 0;
        Dwarf_Unsigned  u0 = 0;
        Dwarf_Unsigned  u1 = 0;
        Dwarf_Signed    s0 = 0;
        Dwarf_Signed    s1 = 0;
        Dwarf_Unsigned  code_alignment_factor = 0;
        Dwarf_Signed    data_alignment_factor = 0;
        Dwarf_Block     expression_block;
        const char *    op_name = 0;

        memset(&expression_block,0,sizeof(expression_block));
        res = dwarf_get_frame_instruction(head,i,
            &instr_offset_in_instrs,&cfa_operation,
            &fields_description,&u0,&u1,
            &s0,&s1,
            &code_alignment_factor,
            &data_alignment_factor,
            &expression_block,error);
        if (res == DW_DLV_ERROR) {
            dwarf_dealloc_frame_instr_head(head);
            return res;
        }
        if (res == DW_DLV_OK) {
            res = dwarf_get_CFA_name(cfa_operation,
                &op_name);
            if (res != DW_DLV_OK) {
                op_name = "unknown op";
            }
            printf("Instr %2lu %-22s %s\n",
                (unsigned long)i,
                op_name,
                fields_description);
            /*  Do something with the various data
                as guided by the fields_description. */
        }
    }
    dwarf_dealloc_frame_instr_head(head);
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup examplestrngoffsets Reading string offsets section data
    @brief Example accessing the string offsets section

    An example accessing the string offsets section
    @param dbg
    The Dwarf_Debug of interest.
    @param dw_error
    On error dw_error is set to point to the error details.
    @return
    DW_DLV_OK etc.

    @code
*/
int examplestrngoffsets(Dwarf_Debug dbg,Dwarf_Error *error)
{
    int                     res = 0;
    Dwarf_Str_Offsets_Table sot = 0;
    Dwarf_Unsigned          wasted_byte_count = 0;
    Dwarf_Unsigned          table_count = 0;
    Dwarf_Error             closeerror = 0;

    res = dwarf_open_str_offsets_table_access(dbg, &sot,error);
    if (res == DW_DLV_NO_ENTRY) {
        /* No such table */
        return res;
    }
    if (res == DW_DLV_ERROR) {
        /* Something is very wrong. Print the error? */
        return res;
    }
    for (;;) {
        Dwarf_Unsigned unit_length =0;
        Dwarf_Unsigned unit_length_offset =0;
        Dwarf_Unsigned table_start_offset =0;
        Dwarf_Half     entry_size = 0;
        Dwarf_Half     version =0;
        Dwarf_Half     padding =0;
        Dwarf_Unsigned table_value_count =0;
        Dwarf_Unsigned i = 0;
        Dwarf_Unsigned table_entry_value = 0;

        res = dwarf_next_str_offsets_table(sot,
            &unit_length, &unit_length_offset,
            &table_start_offset,
            &entry_size,&version,&padding,
            &table_value_count,error);
        if (res == DW_DLV_NO_ENTRY) {
            /* We have dealt with all tables */

            break;
        }
        if (res == DW_DLV_ERROR) {
            /* Something badly wrong. Do something. */
            dwarf_close_str_offsets_table_access(sot,&closeerror);
            dwarf_dealloc_error(dbg,closeerror);
            return res;
        }
        /*  One could call dwarf_str_offsets_statistics to
            get the wasted bytes so far, but we do not do that
            in this example. */
        /*  Possibly print the various table-related values
            returned just above. */
        for (i=0; i < table_value_count; ++i) {
            res = dwarf_str_offsets_value_by_index(sot,i,
                &table_entry_value,error);
            if (res != DW_DLV_OK) {
                /* Something is badly wrong. Do something. */
                dwarf_close_str_offsets_table_access(sot,&closeerror);
                dwarf_dealloc_error(dbg,closeerror);
                return res;
            }
            /*  Do something with the table_entry_value
                at this index. Maybe just print it.
                It is an offset in .debug_str. */
        }
    }
    res = dwarf_str_offsets_statistics(sot,&wasted_byte_count,
        &table_count,error);
    if (res != DW_DLV_OK) {
        dwarf_close_str_offsets_table_access(sot,&closeerror);
        dwarf_dealloc_error(dbg,closeerror);
        return res;
    }
    res = dwarf_close_str_offsets_table_access(sot,error);
    /*  little can be done about any error. */
    sot = 0;
    return res;
}
/* @endcode */

/*! @defgroup exampleu Reading an aranges section
    @brief Example reading .debug_aranges

    An example accessing the .debug_aranges
    section. Looking all the aranges entries.
    This example is not searching for anything.

    @param dbg
    The Dwarf_Debug of interest.
    @param dw_error
    On error dw_error is set to point to the error details.
    @return
    DW_DLV_OK etc.

    @code
*/
static void cleanupbadarange(Dwarf_Debug dbg,
    Dwarf_Arange *arange, Dwarf_Signed i, Dwarf_Signed count)
{
    Dwarf_Signed k = i;

    for ( ; k < count; ++k) {
        dwarf_dealloc(dbg,arange[k] , DW_DLA_ARANGE);
        arange[k] = 0;
    }
}
int exampleu(Dwarf_Debug dbg,Dwarf_Error *error)
{
    /*  It is a historical accident that the count is signed.
        No negative count is possible. */
    Dwarf_Signed  count = 0;
    Dwarf_Arange *arange = 0;
    int           res = 0;

    res = dwarf_get_aranges(dbg, &arange,&count, error);
    if (res == DW_DLV_OK) {
        Dwarf_Signed i = 0;

        for (i = 0; i < count; ++i) {
            Dwarf_Arange ara = arange[i];
            Dwarf_Unsigned segment = 0;
            Dwarf_Unsigned segment_entry_size = 0;
            Dwarf_Addr start = 0;
            Dwarf_Unsigned length = 0;
            Dwarf_Off  cu_die_offset = 0;

            res = dwarf_get_arange_info_b(ara,
                &segment,&segment_entry_size,
                &start, &length,
                &cu_die_offset,error);
            if (res != DW_DLV_OK) {
                cleanupbadarange(dbg,arange,i,count);
                dwarf_dealloc(dbg, arange, DW_DLA_LIST);
                return res;
            }
            /*  Do something with ara */
            dwarf_dealloc(dbg, ara, DW_DLA_ARANGE);
            arange[i] = 0;
        }
        dwarf_dealloc(dbg, arange, DW_DLA_LIST);
    }
    return res;
}
/*! @endcode */

/*! @defgroup examplev Example getting .debug_ranges data
    @brief Example accessing ranges data.

    If have_base_addr is false there is no die
    (as in reading the raw .debug_ranges section)
    or there is some serious data corruption somewhere.
    @code
*/

static
void functionusingrange(Dwarf_Signed i,Dwarf_Ranges *r,
    Dwarf_Bool *have_base_addr,
    Dwarf_Unsigned *baseaddr)
{
    Dwarf_Unsigned base = *baseaddr;

    printf("[%4ld] ",(signed long)i);
    switch(r->dwr_type) {
    case DW_RANGES_ENTRY:
        printf(
            "DW_RANGES_ENTRY: raw    addr1 " PRX
            " addr2 " PRX,
            r->dwr_addr1,r->dwr_addr2);
        if (r->dwr_addr1 == r->dwr_addr2) {
            printf(" (empty range)");
        }
        printf("\n");
        if (*have_base_addr) {
            printf("       "
            "DW_RANGES_ENTRY: cooked addr1 0x%08llx"
            " addr2  " PRX "\n" ,
            r->dwr_addr1+base,r->dwr_addr2+base);
        }
        break;
    case DW_RANGES_ADDRESS_SELECTION:
        printf(
            "Base Address   : " PRX "\n",
            r->dwr_addr2);
        *have_base_addr = TRUE;
        *baseaddr = r->dwr_addr2;
        break;
    case DW_RANGES_END:
        printf(
            "DW_RANGES_END  : 0,0\n");
        *have_base_addr = FALSE;
        *baseaddr = 0;
        break;
    default:
        printf(
            "ERROR          : incorrect dwr_type is 0x%lx\n",
            (unsigned long)r->dwr_type);
    }
}

/* On call the rangesoffset is a default zero. */
int examplev(Dwarf_Debug dbg,Dwarf_Off rangesoffset_in,
    Dwarf_Die die, Dwarf_Error*error)
{
    Dwarf_Signed   count = 0;
    Dwarf_Off      realoffset = 0;
    Dwarf_Ranges  *rangesbuf = 0;
    Dwarf_Unsigned bytecount = 0;
    int            res = 0;
    Dwarf_Unsigned base_address = 0;
    Dwarf_Bool have_base_addr = FALSE;
    Dwarf_Bool have_rangesoffset = FALSE;
    Dwarf_Unsigned rangesoffset = (Dwarf_Unsigned)rangesoffset_in;

    (void)have_rangesoffset;
    if (die) {
        /*  Find the ranges for a specific DIE */
        res = dwarf_get_ranges_baseaddress(dbg,die,&have_base_addr,
        &base_address,&have_rangesoffset,&rangesoffset,error);
        if (res == DW_DLV_ERROR) {
            /* Just pretend not an error. */
            dwarf_dealloc_error(dbg,*error);
            *error = 0;
        }
    } else {
        /*  To test getting all ranges and no knowledge
            of the base address (so cooked values
            cannot be definitely known unless
            the base is in the .debug_ranges entries
            themselves */

    }
    res = dwarf_get_ranges_b(dbg,rangesoffset,die,
        &realoffset,
        &rangesbuf,&count,&bytecount,error);
    if (res != DW_DLV_OK) {
        if (res == DW_DLV_ERROR) {
            printf("ERROR dwarf_get_ranges_b %s\n",
                dwarf_errmsg(*error));
        } else {
            printf("NO_ENTRY dwarf_get_ranges_b\n");
        }
        return res;
    }
    {
        Dwarf_Signed i = 0;
        printf("Range group base address: " PRX
            ", offset in .debug_ranges:"
            " 0x%08llx\n",
            base_address, rangesoffset);
        for ( i = 0; i < count; ++i ) {
            Dwarf_Ranges *cur = rangesbuf+i;
            /* Use cur. */
            functionusingrange(i,cur,&have_base_addr,&base_address);
        }
        dwarf_dealloc_ranges(dbg,rangesbuf,count);
    }
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup examplew Reading gdbindex data
    @brief Example accessing  gdbindex section data

    @code
*/
int examplew(Dwarf_Debug dbg,Dwarf_Error *error)
{
    Dwarf_Gdbindex gindexptr = 0;
    Dwarf_Unsigned version = 0;
    Dwarf_Unsigned cu_list_offset = 0;
    Dwarf_Unsigned types_cu_list_offset = 0;
    Dwarf_Unsigned address_area_offset = 0;
    Dwarf_Unsigned symbol_table_offset = 0;
    Dwarf_Unsigned constant_pool_offset = 0;
    Dwarf_Unsigned section_size = 0;
    const char *   section_name = 0;
    int            res = 0;

    res = dwarf_gdbindex_header(dbg,&gindexptr,
        &version,&cu_list_offset, &types_cu_list_offset,
        &address_area_offset,&symbol_table_offset,
        &constant_pool_offset, &section_size,
        &section_name,error);
    if (res != DW_DLV_OK) {
        return res;
    }
    {
        /* do something with the data */
        Dwarf_Unsigned length = 0;
        Dwarf_Unsigned typeslength = 0;
        Dwarf_Unsigned i = 0;
        res = dwarf_gdbindex_culist_array(gindexptr,
            &length,error);
        /* Example actions. */
        if (res != DW_DLV_OK) {
            dwarf_dealloc_gdbindex(gindexptr);
            return res;
        }
        for (i = 0; i < length; ++i) {
            Dwarf_Unsigned cuoffset = 0;
            Dwarf_Unsigned culength = 0;
            res = dwarf_gdbindex_culist_entry(gindexptr,
                i,&cuoffset,&culength,error);
            if (res != DW_DLV_OK) {
                return res;
            }
            /* Do something with cuoffset, culength */
        }
        res = dwarf_gdbindex_types_culist_array(gindexptr,
            &typeslength,error);
        if (res != DW_DLV_OK) {
            dwarf_dealloc_gdbindex(gindexptr);
            return res;
        }
        for (i = 0; i < typeslength; ++i) {
            Dwarf_Unsigned cuoffset = 0;
            Dwarf_Unsigned tuoffset = 0;
            Dwarf_Unsigned type_signature  = 0;
            res = dwarf_gdbindex_types_culist_entry(gindexptr,
                i,&cuoffset,&tuoffset,&type_signature,error);
            if (res != DW_DLV_OK) {
                dwarf_dealloc_gdbindex(gindexptr);
                return res;
            }
            /* Do something with cuoffset etc. */
        }
        dwarf_dealloc_gdbindex(gindexptr);
    }
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup examplewgdbindex Reading  gdbindex addressarea
    @brief Example accessing gdbindex addressarea data

    @code
*/
int examplewgdbindex(Dwarf_Gdbindex gdbindex,
    Dwarf_Error *error)
{
    Dwarf_Unsigned list_len = 0;
    Dwarf_Unsigned i = 0;
    int            res = 0;

    res = dwarf_gdbindex_addressarea(gdbindex, &list_len,error);
    if (res != DW_DLV_OK) {
        /* Something wrong, ignore the addressarea */
        return res;
    }
    /* Iterate through the address area. */
    for (i  = 0; i < list_len; i++) {
        Dwarf_Unsigned lowpc = 0;
        Dwarf_Unsigned highpc = 0;
        Dwarf_Unsigned cu_index = 0;

        res = dwarf_gdbindex_addressarea_entry(gdbindex,i,
            &lowpc,&highpc,
            &cu_index,
            error);
        if (res != DW_DLV_OK) {
            /* Something wrong, ignore the addressarea */
            return res;
        }
        /*  We have a valid address area entry, do something
            with it. */
    }
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup examplex Reading the gdbindex symbol table
    @brief Example accessing gdbindex symbol table data

    @code
*/
int examplex(Dwarf_Gdbindex gdbindex,Dwarf_Error*error)
{
    Dwarf_Unsigned symtab_list_length = 0;
    Dwarf_Unsigned i = 0;
    int            res = 0;

    res = dwarf_gdbindex_symboltable_array(gdbindex,
        &symtab_list_length,error);
    if (res != DW_DLV_OK) {
        return res;
    }
    for (i  = 0; i < symtab_list_length; i++) {
        Dwarf_Unsigned symnameoffset = 0;
        Dwarf_Unsigned cuvecoffset = 0;
        Dwarf_Unsigned cuvec_len = 0;
        Dwarf_Unsigned ii = 0;
        const char *name = 0;
        int resl = 0;

        resl = dwarf_gdbindex_symboltable_entry(gdbindex,i,
            &symnameoffset,&cuvecoffset,
            error);
        if (resl != DW_DLV_OK) {
            return resl;
        }
        resl = dwarf_gdbindex_string_by_offset(gdbindex,
            symnameoffset,&name,error);
        if (resl != DW_DLV_OK) {
            return resl;
        }
        resl = dwarf_gdbindex_cuvector_length(gdbindex,
            cuvecoffset,&cuvec_len,error);
        if (resl != DW_DLV_OK) {
            return resl;
        }
        for (ii = 0; ii < cuvec_len; ++ii ) {
            Dwarf_Unsigned attributes = 0;
            Dwarf_Unsigned cu_index = 0;
            Dwarf_Unsigned symbol_kind = 0;
            Dwarf_Unsigned is_static = 0;
            int res2 = 0;

            res2 = dwarf_gdbindex_cuvector_inner_attributes(
                gdbindex,cuvecoffset,ii,
                &attributes,error);
            if (res2 != DW_DLV_OK) {
                return res2;
            }
            /*  'attributes' is a value with various internal
                fields so we expand the fields. */
            res2 = dwarf_gdbindex_cuvector_instance_expand_value(
                gdbindex, attributes, &cu_index,
                &symbol_kind, &is_static,
                error);
            if (res2 != DW_DLV_OK) {
                return res2;
            }
            /* Do something with the attributes. */
        }
    }
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup exampley Reading cu and tu Debug Fission data
    @brief Example using dwarf_get_xu_index_header

    Debug Fission is an older name for Split Dwarf.

    @code
*/
int exampley(Dwarf_Debug dbg, const char *type,
    Dwarf_Error *error)
{
    /* type is "tu" or "cu" */
    int                   res = 0;
    Dwarf_Xu_Index_Header xuhdr = 0;
    Dwarf_Unsigned        version_number = 0;
    Dwarf_Unsigned        offsets_count = 0; /*L */
    Dwarf_Unsigned        units_count = 0; /* M */
    Dwarf_Unsigned        hash_slots_count = 0; /* N */
    const char           *section_name = 0;

    res = dwarf_get_xu_index_header(dbg,
        type,
        &xuhdr,
        &version_number,
        &offsets_count,
        &units_count,
        &hash_slots_count,
        &section_name,
        error);
    if (res != DW_DLV_OK) {
        return res;
    }
    /* Do something with the xuhdr here . */
    dwarf_dealloc_xu_header(xuhdr);
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup examplez Reading Split Dwarf (Debug Fission) hash slots
    @brief Example using dwarf_get_xu_hash_entry()

    @code
*/
int examplez( Dwarf_Xu_Index_Header xuhdr,
    Dwarf_Unsigned hash_slots_count,
    Dwarf_Error *error)
{
    /*  hash_slots_count returned by
        dwarf_get_xu_index_header() */
    static Dwarf_Sig8 zerohashval;
    Dwarf_Unsigned h = 0;

    for (h = 0; h < hash_slots_count; h++) {
        Dwarf_Sig8 hashval;
        Dwarf_Unsigned index = 0;
        int res = 0;

        res = dwarf_get_xu_hash_entry(xuhdr,h,
            &hashval,&index,error);
        if (res != DW_DLV_OK) {
            return res;
        }
        if (!memcmp(&hashval,&zerohashval,
            sizeof(Dwarf_Sig8)) && index == 0 ) {
            /* An unused hash slot */
            continue;
        }
        /*  Here, hashval and index (a row index into
            offsets and lengths) are valid. Do
            something with them */
    }
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup examplehighpc Reading high pc from a DIE.
    @brief Example get high-pc from a DIE

    @code
*/
int examplehighpc(Dwarf_Die die,
    Dwarf_Addr *highpc,
    Dwarf_Error *error)
{
    int        res = 0;
    Dwarf_Addr localhighpc = 0;
    Dwarf_Half form = 0;
    enum Dwarf_Form_Class formclass = DW_FORM_CLASS_UNKNOWN;

    res = dwarf_highpc_b(die,&localhighpc,
        &form,&formclass, error);
    if (res != DW_DLV_OK) {
        return res;
    }
    if (form != DW_FORM_addr &&
        !dwarf_addr_form_is_indexed(form)) {
        Dwarf_Addr low_pc = 0;

        /*  The localhighpc is an offset from
            DW_AT_low_pc. */
        res = dwarf_lowpc(die,&low_pc,error);
        if (res != DW_DLV_OK) {
            return res;
        } else  {
            localhighpc += low_pc;
        }
    }
    *highpc = localhighpc;
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup exampleza Reading Split Dwarf (Debug Fission) data
    @brief Example getting cu/tu name, offset

    @code
*/
int exampleza(Dwarf_Xu_Index_Header xuhdr,
    Dwarf_Unsigned offsets_count,
    Dwarf_Unsigned index,
    Dwarf_Error *error)
{
    Dwarf_Unsigned col = 0;

    /*  We use  'offsets_count' returned by
        a dwarf_get_xu_index_header() call.
        We use 'index' returned by a
        dwarf_get_xu_hash_entry() call. */
    for (col = 0; col < offsets_count; col++) {
        Dwarf_Unsigned off = 0;
        Dwarf_Unsigned len = 0;
        const char    *name = 0;
        Dwarf_Unsigned num = 0;
        int res = 0;

        res = dwarf_get_xu_section_names(xuhdr,
            col,&num,&name,error);
        if (res == DW_DLV_ERROR) {
            return res;
        }
        if (res == DW_DLV_NO_ENTRY) {
            break;
        }
        res = dwarf_get_xu_section_offset(xuhdr,
            index,col,&off,&len,error);
        if (res == DW_DLV_ERROR) {
            return res;
        }
        if (res == DW_DLV_NO_ENTRY) {
            break;
        }
        /*  Here we have the DW_SECT_ name and number
            and the base offset and length of the
            section data applicable to the hash
            that got us here.
            Use the values.*/
    }
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup examplezb Retrieving tag,attribute,etc names
    @brief Example getting tag,attribute,etc names as strings.

    @code
*/
void examplezb(void)
{
    const char * out = "unknown something";
    int          res = 0;

    /*  The following is wrong, do not do it!
        Confusing TAG with ACCESS!  */
    res = dwarf_get_ACCESS_name(DW_TAG_entry_point,&out);
    /*  Nothing one does here with 'res' or 'out'
        is meaningful. */

    out = "<unknown TAG>"; /* Not a malloc'd string! */
    /* The following is meaningful.*/
    res = dwarf_get_TAG_name(DW_TAG_entry_point,&out);
    (void)res; /*  avoids unused var compiler warning */
    /*  If res == DW_DLV_ERROR or DW_DLV_NO_ENTRY
        out will be the locally assigned static string.
        If res == DW_DLV_OK it will be a usable
        TAG name string.
        In no case should a returned string be free()d. */
}
/*! @endcode */

/*! @defgroup exampledebuglink Using GNU debuglink data
    @brief Example showing dwarf_add_debuglink_global_path

    An example using both dwarf_add_debuglink_global_path
    and dwarf_gnu_debuglink .
    @code
*/
int exampledebuglink(Dwarf_Debug dbg, Dwarf_Error* error)
{
    int      res = 0;
    char    *debuglink_path = 0;
    unsigned char *crc = 0;
    char    *debuglink_fullpath = 0;
    unsigned debuglink_fullpath_strlen = 0;
    unsigned buildid_type = 0;
    char *   buildidowner_name = 0;
    unsigned char *buildid_itself = 0;
    unsigned buildid_length = 0;
    char **  paths = 0;
    unsigned paths_count = 0;
    unsigned i = 0;

    /*  This is just an example if one knows
        of another place full-DWARF objects
        may be. "/usr/lib/debug" is automatically
        set. */
    res = dwarf_add_debuglink_global_path(dbg,
        "/some/path/debug",error);
    if (res != DW_DLV_OK) {
        /*  Something is wrong*/
        return res;
    }
    res = dwarf_gnu_debuglink(dbg,
        &debuglink_path,
        &crc,
        &debuglink_fullpath,
        &debuglink_fullpath_strlen,
        &buildid_type,
        &buildidowner_name,
        &buildid_itself,
        &buildid_length,
        &paths,
        &paths_count,
        error);
    if (res == DW_DLV_ERROR) {
        return res;
    }
    if (res == DW_DLV_NO_ENTRY) {
        /*  No such sections as .note.gnu.build-id
            or .gnu_debuglink  */
        return res;
    }
    if (debuglink_fullpath_strlen) {
        printf("debuglink     path: %s\n",debuglink_path);
        printf("crc length        : %u  crc: ",4);
        for (i = 0; i < 4;++i ) {
            printf("%02x",crc[i]);
        }
        printf("\n");
        printf("debuglink fullpath: %s\n",debuglink_fullpath);
    }
    if (buildid_length) {
        printf("buildid type      : %u\n",buildid_type);
        printf("Buildid owner     : %s\n",buildidowner_name);
        printf("buildid byte count: %u\n",buildid_length);
        printf(" ");
        /*   buildid_length should be 20. */
        for (i = 0; i < buildid_length;++i) {
            printf("%02x",buildid_itself[i]);
        }
        printf("\n");
    }
    printf("Possible paths count %u\n",paths_count);
    for ( ; i < paths_count; ++i ){
        printf("%2u: %s\n",i,paths[i]);
    }
    free(debuglink_fullpath);
    free(paths);
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup example_raw_rnglist Accessing accessing raw rnglist
    @brief Example showing access to rnglist

    This is accessing DWARF5 .debug_rnglists.

    @code
*/
int example_raw_rnglist(Dwarf_Debug dbg,Dwarf_Error *error)
{
    Dwarf_Unsigned count = 0;
    int            res = 0;
    Dwarf_Unsigned i = 0;

    res = dwarf_load_rnglists(dbg,&count,error);
    if (res != DW_DLV_OK) {
        return res;
    }
    for (i =0  ; i < count ; ++i) {
        Dwarf_Unsigned header_offset = 0;
        Dwarf_Small   offset_size = 0;
        Dwarf_Small   extension_size = 0;
        unsigned      version = 0; /* 5 */
        Dwarf_Small   address_size = 0;
        Dwarf_Small   segment_selector_size = 0;
        Dwarf_Unsigned offset_entry_count = 0;
        Dwarf_Unsigned offset_of_offset_array = 0;
        Dwarf_Unsigned offset_of_first_rangeentry = 0;
        Dwarf_Unsigned offset_past_last_rangeentry = 0;

        res = dwarf_get_rnglist_context_basics(dbg,i,
            &header_offset,&offset_size,&extension_size,
            &version,&address_size,&segment_selector_size,
            &offset_entry_count,&offset_of_offset_array,
            &offset_of_first_rangeentry,
            &offset_past_last_rangeentry,error);
        if (res != DW_DLV_OK) {
            return res;
        }
        {
            Dwarf_Unsigned e = 0;
            unsigned colmax = 4;
            unsigned col = 0;
            Dwarf_Unsigned global_offset_of_value = 0;

            for ( ; e < offset_entry_count; ++e) {
                Dwarf_Unsigned value = 0;
                int resc = 0;

                resc = dwarf_get_rnglist_offset_index_value(dbg,
                    i,e,&value,
                    &global_offset_of_value,error);
                if (resc != DW_DLV_OK) {
                    return resc;
                }
                /*  Do something */
                col++;
                if (col == colmax) {
                    col = 0;
                }
            }
        }
        {
            Dwarf_Unsigned curoffset = offset_of_first_rangeentry;
            Dwarf_Unsigned endoffset = offset_past_last_rangeentry;
            int            rese = 0;
            Dwarf_Unsigned ct = 0;

            for ( ; curoffset < endoffset; ++ct ) {
                unsigned entrylen = 0;
                unsigned code = 0;
                Dwarf_Unsigned v1 = 0;
                Dwarf_Unsigned v2 = 0;
                rese = dwarf_get_rnglist_rle(dbg,i,
                    curoffset,endoffset,
                    &entrylen,
                    &code,&v1,&v2,error);
                if (rese != DW_DLV_OK) {
                    return rese;
                }
                /*  Do something with the values */
                curoffset += entrylen;
                if (curoffset > endoffset) {
                    return DW_DLV_ERROR;
                }
            }
        }
    }
    return DW_DLV_OK;
}
/*! @endcode */

/*! @defgroup example_rnglist_for_attribute Accessing rnglists section
    @brief Example showing access to rnglists on an Attribute

    This is accessing DWARF5 .debug_rnglists.
    The section first appears in DWARF5.

    @code
*/
int example_rnglist_for_attribute(Dwarf_Attribute attr,
    Dwarf_Unsigned attrvalue,Dwarf_Error *error)
{
    /*  attrvalue must be the DW_AT_ranges
        DW_FORM_rnglistx or DW_FORM_sec_offset value
        extracted from attr. */
    int                 res = 0;
    Dwarf_Half          theform = 0;
    Dwarf_Unsigned      entries_count;
    Dwarf_Unsigned      global_offset_of_rle_set;
    Dwarf_Rnglists_Head rnglhead = 0;
    Dwarf_Unsigned      i = 0;

    res = dwarf_rnglists_get_rle_head(attr,
        theform,
        attrvalue,
        &rnglhead,
        &entries_count,
        &global_offset_of_rle_set,
        error);
    if (res != DW_DLV_OK) {
        return res;
    }
    for (i = 0; i < entries_count; ++i) {
        unsigned entrylen     = 0;
        unsigned       code         = 0;
        Dwarf_Unsigned rawlowpc  = 0;
        Dwarf_Unsigned rawhighpc = 0;
        Dwarf_Bool     debug_addr_unavailable = FALSE;
        Dwarf_Unsigned lowpc  = 0;
        Dwarf_Unsigned highpc = 0;

        /*  Actual addresses are most likely what one
            wants to know, not the lengths/offsets
            recorded in .debug_rnglists. */
        res = dwarf_get_rnglists_entry_fields_a(rnglhead,
            i,&entrylen,&code,
            &rawlowpc,&rawhighpc,
            &debug_addr_unavailable,
            &lowpc,&highpc,error);
        if (res != DW_DLV_OK) {
            dwarf_dealloc_rnglists_head(rnglhead);
            return res;
        }
        if (code == DW_RLE_end_of_list) {
            /* we are done */
            break;
        }
        if (code == DW_RLE_base_addressx ||
            code == DW_RLE_base_address) {
            /*  We do not need to use these, they
                have been accounted for already. */
            continue;
        }
        if (debug_addr_unavailable) {
            /* lowpc and highpc are not real addresses */
            continue;
        }
        /*  Here do something with lowpc and highpc, these
            are real addresses */
    }
    dwarf_dealloc_rnglists_head(rnglhead);
    return DW_DLV_OK;
}
/*! @endcode */
