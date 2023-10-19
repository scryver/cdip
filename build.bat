@echo off

set opts=-FC -GR- -EHa- -nologo -Zo -WX -W4 -std:c17 -Z7 -Zc:strictStrings- -wd4146
set code=%cd%\src

IF NOT EXIST gebouw mkdir gebouw

pushd gebouw

    cl %opts% %code%\test_experiment.c -Fetest-experiment

popd
