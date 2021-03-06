#!/usr/bin/bash

if [ -z $1 ]
    then
        echo "Gimme a version number, dork."
        exit
    else
        version=$1
    fi

makedist_dir=makedist_temp
demo_dir=ika-$version
core_dir=core
src_dir=src
dll_dir=dll
xi_dir=xi

original_path=`pwd`
makedist_path=$original_path/$makedist_dir
demo_path=$makedist_path/$demo_dir
core_path=$makedist_path/$core_dir
src_path=$makedist_path/$src_dir
dll_path=$makedist_path/$dll_dir
xi_path=$original_path/$xi_dir

if [ $MACHTYPE==i686-pc-cygwin ]
    then

        if [ -e $makedist_path ]
            then
                echo Temporary directory still exists!  Deleting...
                rm -rf $makedist_path
            fi

        echo Creating temporary directories.

            mkdir $makedist_path
            mkdir $core_path
            mkdir $dll_path
            mkdir $src_path
            mkdir $demo_path

        echo Assembling core zip...

            cp engine/Release/*.exe $core_path
            #cp iked/Release/*.exe $core_path
            cp ikamap/Release/*.exe $core_path
            cp rho/bin/Release/* $core_path
            cp tools/*.exe $core_path
            pushd $core_path

                upx *
                zip $original_path/ika-core-$version.zip *

            popd

        echo Done.

        echo Assembling DLL zip

            cp 3rdparty/dlls/audiere.dll $dll_path
            cp 3rdparty/dlls/corona.dll $dll_path
            cp 3rdparty/dlls/msvcp71.dll $dll_path
            cp 3rdparty/dlls/msvcr71.dll $dll_path
            cp 3rdparty/dlls/python25.dll $dll_path
            cp 3rdparty/dlls/zlib.dll $dll_path
            cp 3rdparty/dlls/sdl.dll $dll_path

            pushd $dll_path

                zip $original_path/ika-dlls-$version.zip *.dll

            popd

        echo Done

        echo Assembling the main dist zip...
            cp -R dist/* $demo_path
            cp doc/index.html $demo_path/python_reference.html
            cp doc/ikamap.html $demo_path

            cp $core_path/* $demo_path
            cp $dll_path/*.dll $demo_path
            cp -R $xi_path $demo_path

            pushd $demo_path/..

                zip -r $original_path/ika-win-$version.zip $demo_dir/*

            popd
        echo Done.

        echo Creating NSIS installer.
            pushd $demo_path

            sed "s/@@VERSION@@/$version/g" < $original_path/ika.nis > $demo_path/ika.nis

            C:/Program\ Files/NSIS/makensis.exe ika.nis && \
                mv ika-install-$version.exe $original_path

            popd
        echo Done.

        echo Assembling source archive...
            pushd $src_path
                for x in common engine iked ikaMap rho xi; do
                    svn export -r head https://ika.svn.sourceforge.net/svnroot/ika/trunk/$x $x >> $makedist_path/makedist.log
                done

                # hack, since cvs won't pull it in on its own:
                cp $original_path/SConstruct $src_path
            popd

            pushd $src_path/..
                tar cjf $original_path/ika-src-$version.tar.bz2 $src_dir
            popd
        echo Done.
    else
        echo "This don't work on nonwindows yet."
        echo "TODO:  Correct this."
    fi
