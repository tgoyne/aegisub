#!/bin/sh

maybe_append() {
  while read -r msg; do
    msgfile=$(echo $msg | cut -d'|' -f1)
    msgline=$(echo $msg | cut -d'|' -f2)
    msgid=$(echo $msg | cut -d'|' -f3-)

    if ! grep -Fq "msgid $msgid" aegisub.pot; then
      echo "\n#: $msgfile:$msgline\nmsgid $msgid\nmsgstr \"\"\n" >> aegisub.pot
    fi
  done
}

find ../src ../src/command -name \*.cpp -o -name \*.h \
  | xgettext --files-from=- -o - --c++ -k_ -kSTR_MENU -kSTR_DISP -kSTR_HELP -kwxT \
  | sed 's/SOME DESCRIPTIVE TITLE./Aegisub 3.0.0/' \
  | sed 's/YEAR/2005-2012/' \
  | sed "s/THE PACKAGE'S COPYRIGHT HOLDER/Rodrigo Braz Monteiro, Niels Martin Hansen, Thomas Goyne et. al./" \
  | sed 's/PACKAGE/Aegisub/' \
  | sed 's/VERSION/3.0.0/' \
  | sed 's/FIRST AUTHOR <EMAIL@ADDRESS>/Niels Martin Hansen <nielsm@aegisub.org>/' \
  | sed 's/CHARSET/UTF-8/' \
  > aegisub.pot

sed '/"text"/!d;s/^.*"text" : \("[^"]\+"\).*$/default_menu.json|0|\1/' ../src/libresrc/default_menu.json \
  | maybe_append

sed '/"text"/!d;s/^.*"text" : \("[^"]\+"\).*$/default_menu.json|0|\1/' ../src/libresrc/osx/default_menu.json \
  | maybe_append

grep '"[A-Za-z ]\+" : {' -n ../src/libresrc/default_hotkey.json \
  | sed 's/^\([0-9]\+:\).*\("[^"]\+"\).*$/default_hotkey.json|\1|\2/' \
  | maybe_append

find ../automation -name *.lua \
  | xargs grep tr\"[^\"]\*\" -o -n \
  | sed 's/\(.*\):\([0-9]\+\):tr\(".*"\)/\1|\2|\3/' \
  | sed 's/\\/\\\\\\\\/g' \
  | maybe_append

for i in 'Name' 'GenericName' 'Comment'
do
  grep ^$i -n ../desktop/aegisub.desktop.in \
    | sed 's/\([0-9]\+\):[^=]\+=\(.*\)$/aegisub.desktop|\1|"\2"/' \
    | maybe_append
done
