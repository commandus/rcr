#!/bin/bash
# po/rcr-cli.ru_RU.UTF-8.po
TEMPLATE=po/rcr-cli.pot
for f in $(ls po/*.po) ; do
  regex="\/(.*)\.([a-z][a-z])_..\.UTF-8\.po"
  if [[ $f =~ $regex ]]; then
    fn="${BASH_REMATCH[1]}"
    code="${BASH_REMATCH[2]}"

    case $fn in
      'box') FM='cli/box.cpp';;
      'mkdb') FM='cli/mkdb.cpp';;
      'rcr-cli') FM='cli/rcr-cli.cpp cli/grpcClient.cpp SpreadSheetHelper.cpp';;
      'rcr-svc') FM='svc/* SpreadSheetHelper.cpp';;
      *) FM='cli/grpcClient.cpp';;
    esac

    xgettext -k_ -o $TEMPLATE $FM
    echo -n Merge $fn $FM ${code} ..
    msgmerge -U $f $TEMPLATE
    mkdir -p locale/${code}/LC_MESSAGES
    msgfmt -o locale/${code}/LC_MESSAGES/$fn.mo $f
#   sudo cp locale/${code}/LC_MESSAGES/*.mo /usr/share/locale/${code}/LC_MESSAGES/
#   sudo cp locale/${code}/LC_MESSAGES/*.mo /usr/local/share/locale/${code}/LC_MESSAGES/
  fi
done
