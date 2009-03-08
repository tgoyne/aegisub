#*************************************************************************
#
#   $RCSfile: makefile.mk,v $
#
#   $Revision: 1.7 $
#
#   last change: $Author: vg $ $Date: 2003/06/12 10:38:24 $
#
#   The Contents of this file are made available subject to the terms of
#   either of the following licenses
#
#          - GNU Lesser General Public License Version 2.1
#          - Sun Industry Standards Source License Version 1.1
#
#   Sun Microsystems Inc., October, 2000
#
#   GNU Lesser General Public License Version 2.1
#   =============================================
#   Copyright 2000 by Sun Microsystems, Inc.
#   901 San Antonio Road, Palo Alto, CA 94303, USA
#
#   This library is free software; you can redistribute it and/or
#   modify it under the terms of the GNU Lesser General Public
#   License version 2.1, as published by the Free Software Foundation.
#
#   This library is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public
#   License along with this library; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston,
#   MA  02111-1307  USA
#
#
#   Sun Industry Standards Source License Version 1.1
#   =================================================
#   The contents of this file are subject to the Sun Industry Standards
#   Source License Version 1.1 (the "License"); You may not use this file
#   except in compliance with the License. You may obtain a copy of the
#   License at http://www.openoffice.org/license.html.
#
#   Software provided under this License is provided on an "AS IS" basis,
#   WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
#   WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
#   MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
#   See the License for the specific provisions governing your rights and
#   obligations concerning the Software.
#
#   The Initial Developer of the Original Code is: Sun Microsystems, Inc.
#
#   Copyright: 2000 by Sun Microsystems, Inc.
#
#   All Rights Reserved.
#
#   Contributor(s): _______________________________________
#
#
#
#*************************************************************************

PRJ = ..

PRJNAME	= hunspell
TARGET	= hunspell
LIBTARGET=NO

#----- Settings ---------------------------------------------------------

.INCLUDE : settings.mk

# --- Files --------------------------------------------------------

# all_target: ALLTAR DICTIONARY
all_target: ALLTAR

##CXXFLAGS += -I..$/..$/lingutil
##CFLAGSCXX += -I..$/..$/lingutil
##CFLAGSCC += -I..$/..$/lingutil

CDEFS+=-DOPENOFFICEORG

SLOFILES=	\
		$(SLO)$/affentry.obj \
		$(SLO)$/affixmgr.obj \
		$(SLO)$/dictmgr.obj \
		$(SLO)$/csutil.obj \
		$(SLO)$/utf_info.obj \
		$(SLO)$/hashmgr.obj \
		$(SLO)$/suggestmgr.obj \
		$(SLO)$/hunspell.obj

LIB1TARGET= $(SLB)$/lib$(TARGET).lib
LIB1ARCHIV= $(LB)/lib$(TARGET).a
LIB1OBJFILES= $(SLOFILES)

# DIC2BIN= \
#	en_US.aff \
#	en_US.dic
#
#	de_DE.aff \
#	de_DE.dic


# DICTIONARY : 
#	+$(COPY) $(foreach,i,$(DIC2BIN) $i) $(BIN)


# --- Targets ------------------------------------------------------

.INCLUDE : target.mk

