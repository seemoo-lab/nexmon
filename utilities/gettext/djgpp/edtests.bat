@echo off
echo Editing test scripts in tests/ subdirectory for DJGPP...
test -f ./tests/gettext-1
if not errorlevel 1 mv -f ./tests/gettext-1 ./tests/gettext.1
test -f ./tests/gettext-2
if not errorlevel 1 mv -f ./tests/gettext-2 ./tests/gettext.2
test -f ./tests/msgcmp-1
if not errorlevel 1 mv -f ./tests/msgcmp-1 ./tests/msgcmp.1
test -f ./tests/msgcmp-2
if not errorlevel 1 mv -f ./tests/msgcmp-2 ./tests/msgcmp.2
test -f ./tests/msgfmt-1
if not errorlevel 1 mv -f ./tests/msgfmt-1 ./tests/msgfmt.1
test -f ./tests/msgfmt-2
if not errorlevel 1 mv -f ./tests/msgfmt-2 ./tests/msgfmt.2
test -f ./tests/msgfmt-3
if not errorlevel 1 mv -f ./tests/msgfmt-3 ./tests/msgfmt.3
test -f ./tests/msgfmt-4
if not errorlevel 1 mv -f ./tests/msgfmt-4 ./tests/msgfmt.4
test -f ./tests/msgmerge-1
if not errorlevel 1 mv -f ./tests/msgmerge-1 ./tests/msgmerge.1
test -f ./tests/msgmerge-2
if not errorlevel 1 mv -f ./tests/msgmerge-2 ./tests/msgmerge.2
test -f ./tests/msgmerge-3
if not errorlevel 1 mv -f ./tests/msgmerge-3 ./tests/msgmerge.3
test -f ./tests/msgmerge-4
if not errorlevel 1 mv -f ./tests/msgmerge-4 ./tests/msgmerge.4
sed -f ./djgpp/tscript.sed ./tests/msgmerge.4 > msgmerge.4
update msgmerge.4 ./tests/msgmerge.4
rm -f msgmerge.4
test -f ./tests/msgmerge-5
if not errorlevel 1 mv -f ./tests/msgmerge-5 ./tests/msgmerge.5
sed -f ./djgpp/tscript.sed ./tests/msgmerge.5 > msgmerge.5
update msgmerge.5 ./tests/msgmerge.5
rm -f msgmerge.5
test -f ./tests/msgunfmt-1
if not errorlevel 1 mv -f ./tests/msgunfmt-1 ./tests/msgunfmt.1
test -f ./tests/xgettext-1
if not errorlevel 1 mv -f ./tests/xgettext-1 ./tests/xgettext.1
sed -f ./djgpp/tscript.sed ./tests/xgettext.1 > xgettext.1
update xgettext.1 ./tests/xgettext.1
rm -f xgettext.1
test -f ./tests/xgettext-2
if not errorlevel 1 mv -f ./tests/xgettext-2 ./tests/xgettext.2
sed -f ./djgpp/tscript.sed ./tests/xgettext.2 > xgettext.2
update xgettext.2 ./tests/xgettext.2
rm -f xgettext.2
test -f ./tests/xgettext-3
if not errorlevel 1 mv -f ./tests/xgettext-3 ./tests/xgettext.3
test -f ./tests/xgettext-4
if not errorlevel 1 mv -f ./tests/xgettext-4 ./tests/xgettext.4
sed -f ./djgpp/tscript.sed ./tests/xgettext.4 > xgettext.4
update xgettext.4 ./tests/xgettext.4
rm -f xgettext.4
test -f ./tests/xgettext-5
if not errorlevel 1 mv -f ./tests/xgettext-5 ./tests/xgettext.5
sed -f ./djgpp/tscript.sed ./tests/xgettext.5 > xgettext.5
update xgettext.5 ./tests/xgettext.5
rm -f xgettext.5
test -f ./tests/xgettext-6
if not errorlevel 1 mv -f ./tests/xgettext-6 ./tests/xgettext.6
sed -f ./djgpp/tscript.sed ./tests/xgettext.6 > xgettext.6
update xgettext.6 ./tests/xgettext.6
rm -f xgettext.6
test -f ./tests/xgettext-7
if not errorlevel 1 mv -f ./tests/xgettext-7 ./tests/xgettext.7
sed -f ./djgpp/tscript.sed ./tests/xgettext.7 > xgettext.7
update xgettext.7 ./tests/xgettext.7
rm -f xgettext.7
test -f ./tests/xgettext-8
if not errorlevel 1 mv -f ./tests/xgettext-8 ./tests/xgettext.8
sed -f ./djgpp/tscript.sed ./tests/xgettext.8 > xgettext.8
update xgettext.8 ./tests/xgettext.8
rm -f xgettext.8
test -f ./tests/xgettext-9
if not errorlevel 1 mv -f ./tests/xgettext-9 ./tests/xgettext.9
test -f ./tests/xg-test1.ok.po
if not errorlevel 1 mv -f ./tests/xg-test1.ok.po ./tests/xg-test1.ok-po
test -f ./tests/plural-1
if not errorlevel 1 mv -f ./tests/plural-1 ./tests/plural.1
sed -f ./djgpp/tscript.sed ./tests/plural.1 > plural.1
update plural.1 ./tests/plural.1
rm -f plural.1
test -f ./tests/plural-2
if not errorlevel 1 mv -f ./tests/plural-2 ./tests/plural.2
touch ./tests/stamp-test
echo Done.
