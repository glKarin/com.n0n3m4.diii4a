/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __DARKMOD_DECLXDATA_H__
#define __DARKMOD_DECLXDATA_H__

/// An external data declaration class.
/** The purpose of this class is to allow scripts to have settings defined
 *  externally. For example, many people want to be able to set up readables
 *  using declarations instead of settings on the readable entity. For this
 *  purpose, xdata (eXternal data) declarations can be used. The contents of
 *  a parsed xdata decl are a list of key/value pairs that a script can
 *  copy into an entity.
 *  
 *  Example xdata format:
 *  
 *  path/of/xdata/decl
 *  {
 *      "key" : "value"
 *      "key2" :
 *      {
 *          "A list of strings"
 *          "that are concatenated together"
 *          "with newlines in between them"
 *          ""
 *          "    -Bob"
 *      }
 *      "key3" :
 *          "A list of strings\n" \
 *          "that are concatenated together\n" \
 *          "with newlines in between them\n" \
 *          "\n    -Bob\n"
 *  
 *      precache
 *  
 *      import "path/to/xdata/decl"
 *  
 *      import {
 *          "key"
 *          "another key"
 *          "key name in the other decl" -> "what to call the key in our decl"
 *          "blah"
 *          "foo" -> "bar"
 *      } from "path/to/xdata/decl"
 *  
 *  }
 *  
 *  The most important part of an xdata declaration are its key/value pairs,
 *  which contain the actual data for scripts to use. The syntax for setting
 *  a key/value pair is as follows:
 *  
 *  "name of key" : "data value"
 *  
 *  If you want to write a paragraph with a lot of newlines, there's another
 *  syntax that may be more convenient:
 *  
 *  "name of key" : { "first line" "second line" "third line" "etc" }
 *  
 *  This syntax treats each string in the braces as ending with an implicit
 *  newline.
 *  
 *  You may want to grab keys/values from other xdata declarations. (this is
 *  useful for simulating inheritance, among other things) To do so, you may
 *  use the import command. There are two syntaxes:
 *  
 *  import "/path/of/xdata/decl"
 *  
 *  This will import all key/value pairs from /path/of/xdata/decl, overwriting
 *  any previously defined ones. If you only want to import a few specific
 *  key/value pairs, you can use another syntax:
 *  
 *  import { "key1" "key2" "key3" "etc" } from "/path/of/xdata/decl"
 *  
 *  This will only import the key/value pairs with the keys "key1", "key2,
 *  "key3" and "etc". If you wish to rename a key, that you're importing,
 *  you can use the following syntax:
 *  
 *  import { "originalKey" -> "usefulKey" } from "/path/of/xdata/decl"
 *  
 *  This will set "originalKey" in our xdata decl to the value of "originalKey"
 *  in "/path/of/xdata/decl". These last two syntaxes can be used together:
 *  
 *  import { "key1" "badKey"->"goodKey" "key3" "key4" } from "/path/of/xdata/decl"
 *  
 *  Let's say you're using an xdata decl to keep track of assets that you want
 *  to use in your level. By default, even though the xdata decl may be pre-cached,
 *  its contents won't be, so when you make use of the assets mentioned in it, the
 *  game may stutter. If you wish for D3 to traverse your xdata decl and pre-cache
 *  all resources it mentions, use the precache command. The syntax is simple:
 *  
 *  precache
 *  
 *  This will cause D3 to treat the contents of your xdata decl like the
 *  spawn-arguments of an entity; keys with special prefixes like "gui" will
 *  have their contents preloaded, if possible. The position of the precache
 *  command in your xdata decl does't chance its effect; Pre-cache is always
 *  done when the xdata decl is finished loading.
 */
class tdmDeclXData : public idDecl
{
public:
	tdmDeclXData();
	virtual ~tdmDeclXData() override;

	virtual size_t			Size( void ) const override;
	virtual const char *	DefaultDefinition( void ) const override;
	virtual void			FreeData( void ) override;
	virtual bool			Parse( const char *text, const int textLength ) override;

	/// Key/value data parsed from the xdata decl.
	idDict					m_data;

};

#endif /* __DARKMOD_DECLXDATA_H__ */
