#!/bin/sh

PKG_DIR=${1}.app
SKEL_DIR="packages/osx_bundle"
AEGISUB_VERSION_DATA="${2}"
AEGISUB_BIN="aegisub-${AEGISUB_VERSION_DATA}"
SRCDIR=`pwd`
HOME_DIR=`echo ~`
WX_CONFIG="wx-config"
WX_PREFIX=`${WX_CONFIG} --prefix`

if test -z "${CC}"; then
  CC="cc"
fi

if ! test -d packages/osx_bundle; then
  echo
  echo "Make sure you're in the toplevel source directory"
  exit 1;
fi

if test -d "${PKG_DIR}"; then
  echo "**** USING OLD ${PKG_DIR} ****"
fi

echo
echo "---- Directory Structure ----"
mkdir -v "${PKG_DIR}"
mkdir -v "${PKG_DIR}/Contents"
mkdir -v "${PKG_DIR}/Contents/MacOS"
mkdir -v "${PKG_DIR}/Contents/Resources"
mkdir -v "${PKG_DIR}/Contents/Resources/etc"
mkdir -v "${PKG_DIR}/Contents/Resources/etc/fonts"
mkdir -v "${PKG_DIR}/Contents/Resources/etc/fonts/conf.d"
mkdir -v "${PKG_DIR}/Contents/SharedSupport"
mkdir -v "${PKG_DIR}/Contents/SharedSupport/dictionaries"

echo
echo "---- Copying Skel Files ----"
if ! test -f "tools/osx-bundle.sed"; then
  echo
  echo "NOT FOUND: tools/osx-bundle.sed";
  exit 1;
fi

find ${SKEL_DIR} -type f -not -regex ".*.svn.*"
cp ${SKEL_DIR}/Contents/Resources/*.icns "${PKG_DIR}/Contents/Resources"
cp ${SKEL_DIR}/Contents/Resources/etc/fonts/fonts.dtd "${PKG_DIR}/Contents/Resources/etc/fonts"
cat ${SKEL_DIR}/Contents/Resources/etc/fonts/fonts.conf |sed -f tools/osx-bundle.sed > "${PKG_DIR}/Contents/Resources/etc/fonts/fonts.conf"
cp ${SKEL_DIR}/Contents/Resources/etc/fonts/conf.d/*.conf "${PKG_DIR}/Contents/Resources/etc/fonts/conf.d"
cat ${SKEL_DIR}/Contents/Info.plist |sed -f tools/osx-bundle.sed > "${PKG_DIR}/Contents/Info.plist"


echo
echo "---- Copying dictionaries ----"
if test -z "${DICT_DIR}"; then
  DICT_DIR="${HOME_DIR}/dict"
fi

if test -d "${DICT_DIR}"; then
  cp -v ${DICT_DIR}/* "${PKG_DIR}/Contents/SharedSupport/dictionaries"
else
  echo "WARNING: Dictionaries not found, please set $$DICT_DIR to a directiory"
  echo "         where the *.aff and *.dic files can be found"
fi


echo
echo "---- Copying automation/ files ----"
cd automation
make install \
	aegisubdatadir="../${PKG_DIR}/Contents/SharedSupport" \
	aegisubdocdir="../${PKG_DIR}/Contents/SharedSupport/doc"
cd "${SRCDIR}"


echo
echo "---- Copying Aegisub locale files ----"
# Let Aqua know that aegisub supports english.  English strings are
# internal so we don't need an aegisub.mo file.
mkdir -vp "${PKG_DIR}/Contents/Resources/en.lproj"

for i in `ls -1 po/*.mo|sed "s|po/\(.*\).mo|\1|"`; do
  if test -f "po/${i}.mo"; then
    mkdir -p "${PKG_DIR}/Contents/Resources/${i}.lproj";
    cp -v po/${i}.mo "${PKG_DIR}/Contents/Resources/${i}.lproj/aegisub.mo";
  else
    echo "${i}.mo not found!"
	exit 1
  fi;
done


echo
echo "---- Copying WX locale files ----"

for i in `ls -1 po/*.mo|sed "s|po/\(.*\).mo|\1|"`; do
#  WX_MO="${WX_PREFIX}/share/locale/${i}/LC_MESSAGES/wxstd.mo"
  WX_MO="${HOME_DIR}/wxstd/${i}.mo"

  if test -f "${WX_MO}"; then
	cp -v "${WX_MO}" "${PKG_DIR}/Contents/Resources/${i}.lproj/";
  else
	echo "WARNING: \"$i\" locale in aegisub but no WX catalog found!";
  fi;
done


echo
echo " ---- Copying binary ----"
cp -v src/${AEGISUB_BIN} "${PKG_DIR}/Contents/MacOS/aegisub"


echo
echo " ---- Build / install restart-helper ----"
echo cc -o "${PKG_DIR}/Contents/MacOS/restart-helper tools/osx-bundle-restart-helper.c"
${CC} -o "${PKG_DIR}/Contents/MacOS/restart-helper" tools/osx-bundle-restart-helper.c || exit $?


echo
echo "---- Libraries ----"
python tools/osx-fix-libs.py "${PKG_DIR}/Contents/MacOS/aegisub" || exit $?

echo
echo "Done Creating \"${PKG_DIR}\""
