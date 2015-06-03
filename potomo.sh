#!/bin/sh
xgettext -k_ indicator-systemtray.c -o indicator-systemtray.pot
echo >>indicator-systemtray.pot
cp indicator-systemtray.pot ./po/indicator-systemtray.pot
cd po
f=`find -name \*.po`
for po_file in $f
do
echo "Processing ${po_file}"
po_file=`echo ${po_file} | sed -e 's/.\///'`
lang=`echo ${po_file} | sed -e 's/.po$//'`
echo ${po_file}
echo ${lang}
msgmerge -U ${lang}.po indicator-systemtray.pot
if [ "$@" == "" ];
then
mo_dir=usr/share/locale/${lang}/LC_MESSAGES
else
mo_dir=$@/${lang}/LC_MESSAGES
fi
mkdir -p ${mo_dir}
msgfmt ${lang}.po -o ${mo_dir}/indicator-systemtray-unity.mo
done
cd ..
