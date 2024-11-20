#ifndef KEY_VALUE_INI_H

#define KEY_VALUE_INI_H

/*!
**
** Copyright (c) 2007 by John W. Ratcliff mailto:jratcliff@infiniplex.net
**
** Portions of this source has been released with the PhysXViewer application, as well as
** Rocket, CreateDynamics, ODF, and as a number of sample code snippets.
**
** If you find this code useful or you are feeling particularily generous I would
** ask that you please go to http://www.amillionpixels.us and make a donation
** to Troy DeMolay.
**
** DeMolay is a youth group for young men between the ages of 12 and 21.
** It teaches strong moral principles, as well as leadership skills and
** public speaking.  The donations page uses the 'pay for pixels' paradigm
** where, in this case, a pixel is only a single penny.  Donations can be
** made for as small as $4 or as high as a $100 block.  Each person who donates
** will get a link to their own site as well as acknowledgement on the
** donations blog located here http://www.amillionpixels.blogspot.com/
**
** If you wish to contact me you can use the following methods:
**
** Skype Phone: 636-486-4040 (let it ring a long time while it goes through switches)
** Skype ID: jratcliff63367
** Yahoo: jratcliff63367
** AOL: jratcliff1961
** email: jratcliff@infiniplex.net
**
**
** The MIT license:
**
** Permission is hereby granted, MEMALLOC_FREE of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is furnished
** to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.

** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
** WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
** CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

class KeyValueIni;
class KeyValueSection;

/*!
 * \brief
 * Write loadKeyValueIni loads an INI file by filename and returns an opaque handle to the interface.  If null, then the file was not found or was empty.
 *
 * \param fname
 * The name of the file on disk to load.
 *
 * \param sections
 * A reference to an unsigned integer that will report the number of init sections in the file.  A special init section called '@HEADER' contains values that were found before any section block.
 *
 * \returns
 * Returns an opaque pointer to the KeyValueIni interface.
 *
 * Write detailed description for loadKeyValueIni here.
 *
 * Loads .INI files that can be separated into sections using the bracket keys.  Example [RENDER] [PHYSICS] etc.  Supports key value pairs in the form
 * of KEY=VALUE or it will support a single KEY entry with no accompanying value.  Keys encountered prior to encountering any section are placed in the
 * default section of '@HEADER'
 *
 * Note that if you have a section called [RENDER] and then a section called [MESH] but later refer to [RENDER] again, this does *not* create a new section, it
 * adds the keys to the previously defined section.
 *
 * Note also that all returned pointers are persistent up until the KeyValueIni file is released.  That means you can avoid string copies and instead only cache the
 * pointers internally so long as you keep the INI file loaded.
 */
//KeyValueIni *     loadKeyValueIni(const char *fname,unsigned int &sections);

/*!
 * \brief
 * Loads an INI file from a block of memory rather than a file on disk.
 *
 * \param mem
 * An address in memory pointing to the text file containing initialization data.
 *
 * \param len
 * The length of the input data.
 *
 * \param sections
 * A reference that is filled with the number of sections that were encountered.
 *
 * \returns
 * Returns an opaque pointer to the KeyValueIni interface.
 *
 * Write detailed description for loadKeyValueIni here.
 *
 * The memory passed in is copied internally so the caller does not need to keep it around.  There are no memory copies in this system, the initialization data
 * points back to the original source content in memory.  This system is extremely fast.
 *
 * Comment symbols for INI files are '#' '!' and ';'
 */
KeyValueIni *     loadKeyValueIni(const char *mem,unsigned int len,unsigned int &sections);

/*!
 * \brief
 * Locates a section by name.
 *
 * \param ini
 * The KeyValueIni pointer from a previously loaded INI file.
 *
 * \param section
 * The name of the initialization section you want to located.
 *
 * \param keycount
 * A reference that will be assigned the number of keys in this section.
 *
 * \param lineno
 * A reference to the line number in the original INI file where this key is found.
 *
 * \returns
 * Returns an opaque pointer to the corresponding section.
 *
 */
const KeyValueSection * locateSection(const KeyValueIni *ini,const char *section,unsigned int &keycount,unsigned int &lineno);

/*!
 * \brief
 * gets a specific section by array index.  If the index is out of range it will return a null.
 *
 * \param ini
 * The opaque pointer to a previously loaded INI file
 *
 * \param index
 * The array index (zero based) for the corresponding section.
 *
 * \param keycount
 * A reference that will be assigned the number of keys in this section.
 *
 * \param lineno
 * A reference that will be assigned the line number where this section was defined in the original source data.
 * 
 * \returns
 * Returns an opaque pointer to the corresponding section, or null if the array index is out of ragne.
 */
const KeyValueSection * getSection(const KeyValueIni *ini,unsigned int index,unsigned int &keycount,unsigned int &lineno);

/*!
 * \brief
 * Returns the ASCIIZ name field for a particular section.
 * 
 * \param section
 * The previously obtained opaque KeyValueSection pointer.
 * 
 * \returns
 * Returns the ASCIIZ name of the section.
 * 
 */
const char *            getSectionName(const KeyValueSection *section);

/*!
 * \brief
 * locates the value for a particular Key/Value pair (i.e. KEY=VALUE)
 * 
 * \param section
 * The opaque pointer to the KeyValueSection to query.
 * 
 * \param key
 * The name of the key we are searching for.
 * 
 * \param lineno
 * A reference that will be assigned to the line number in the original INI file that this key was found.
 * 
 * \returns
 * Returns a pointer to the value component.  If this is null it means only a key was found when the file was parsed, no corresponding value was encountered.
 * 
 */
const char *      locateValue(const KeyValueSection *section,const char *key,unsigned int &lineno);

/*!
 * \brief
 * Gets a key by array index.
 * 
 * \param section
 * The pointer to the KeyValueSection we are indexing.
 * 
 * \param keyindex
 * The array index for the key we are requesting.
 * 
 * \param lineno
 * A reference that will be assigned the linenumber in the original text file where the key was found.
 *
 * \returns
 * Returns the key found at this location.
 */
const char *      getKey(const KeyValueSection *section,unsigned int keyindex,unsigned int &lineno);

/*!
 * \brief
 * Returns the value field based on an array index.
 *
 * \param section
 * A pointer to the KeyValueSection we are searching.
 *
 * \param keyindex
 * The array index to look the value up against.
 *
 * \param lineno
 * A reference to the line number in the original file where this key was located.
 *
 * \returns
 * Returns a pointer to the value at this entry or a null if there was only a key but no value.
 *
 */
const char *      getValue(const KeyValueSection *section,unsigned int keyindex,unsigned int &lineno);

/*!
 * \brief
 * This method releases the previously loaded and parsed INI file.  This frees up memory and any previously cached pointers
 * will no longer be valid.
 *
 * \param ini
 * The pointer to the previously loaded INI file.
 *
 */
void              releaseKeyValueIni(KeyValueIni *ini);

//bool              saveKeyValueIni(const KeyValueIni *ini,const char *fname);
void *            saveKeyValueIniMem(const KeyValueIni *ini,unsigned int &len); // save it to a buffer in memory..
bool              releaseIniMem(void *mem);

KeyValueIni      *createKeyValueIni(void); // create an empty .INI file in memory for editing.

KeyValueSection  *createKeyValueSection(KeyValueIni *ini,const char *section_name,bool reset);  // creates, or locates and existing section for editing.  If reset it true, will erase previous contents of the section.
bool              addKeyValue(KeyValueSection *section,const char *key,const char *value); // adds a key-value pair.  These pointers *must* be persistent for the lifetime of the INI file!


#endif
