/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * Copyright (C) 2006
 * Miha Vrhovnik (http://simail.sf.net, http://xcollect.sf.net)
 * All Rights Reserved.
 *
 * Contributor(s):
 * 
 *  
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** **/
#include "hunspell.hxx"

#ifndef _DLL_H_
#define _DLL_H_

#ifdef HUNSPELL_STATIC
	#define DLLEXPORT
#else
	#ifdef HUNSPELL_EXPORTS
		#define DLLEXPORT  __declspec( dllexport )
	#else
		#define DLLEXPORT  __declspec( dllimport )
	#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

//returns pointer to spell object, params are aff file name and dict file name
DLLEXPORT void *hunspell_initialize(char *aff_file, char *dict_file);
//frees spell object
DLLEXPORT void hunspell_uninitialize(Hunspell *pMS);
//spellcheck word, returns 1 if word ok otherwise 0
DLLEXPORT int hunspell_spell(Hunspell *pMS, char *word);
//suggest words for word, returns number of words in slst
// YOU NEED TO CALL hunspell_suggest_free after you've done with words
DLLEXPORT int hunspell_suggest(Hunspell *pMS, char *word, char ***slst);
DLLEXPORT int hunspell_suggest_auto(Hunspell *pMS, char *word, char ***slst);
//free slst array
DLLEXPORT void hunspell_suggest_free(Hunspell *pMS, char **slst, int len);
//make local copy of returned string!!
DLLEXPORT char * hunspell_get_dic_encoding(Hunspell *pMS);
//add word to dict (word is valid until spell object is not destroyed)
DLLEXPORT int hunspell_put_word(Hunspell *pMS, char *word);

#ifdef __cplusplus
}
#endif

#endif /* _DLL_H_ */
