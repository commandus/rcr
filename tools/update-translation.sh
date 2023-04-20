#!/bin/bash
# po/rcr-cli.ru_RU.UTF-8.po
TEMPLATE=po/rcr-cli.pot
for f in $(ls po/rcr-cli.*.po) ; do
  regex="\.([a-z][a-z])_..\.UTF-8\.po"
  if [[ $f =~ $regex ]]; then
    code="${BASH_REMATCH[1]}"
    xgettext -k_ -o $TEMPLATE cli/*.cpp svc/*.cpp
    echo -n Merge ${code} ..
    msgmerge -U $f $TEMPLATE
    echo Copying ${code} ..
    mkdir -p locale/${code}/LC_MESSAGES
    msgfmt -o locale/${code}/LC_MESSAGES/rcr-cli.mo $f
#   sudo cp locale/ru/LC_MESSAGES/rcr-cli.mo /usr/local/share/locale/${code}/LC_MESSAGES/rcr-cli.mo
#   sudo cp locale/ru/LC_MESSAGES/rcr-cli.mo /usr/share/locale/${code}/LC_MESSAGES/rcr-cli.mo
  fi
done
