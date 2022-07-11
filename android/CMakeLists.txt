# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.10.2)

# Declares and names the project.

project("DWIB")

set(LIBXML2_SOURCES

        libxml2/SAX.c
        libxml2/entities.c
        libxml2/encoding.c
        libxml2/error.c
        libxml2/parserInternals.c
        libxml2/parser.c
        libxml2/tree.c
        libxml2/hash.c
        libxml2/list.c
        libxml2/xmlIO.c
        libxml2/xmlmemory.c
        libxml2/uri.c
        libxml2/valid.c
        libxml2/xlink.c
        libxml2/debugXML.c
        libxml2/xpath.c
        libxml2/xpointer.c
        libxml2/xinclude.c
        libxml2/catalog.c
        libxml2/globals.c
        libxml2/threads.c
        libxml2/c14n.c
        libxml2/xmlstring.c
        libxml2/buf.c
        libxml2/xmlregexp.c
        libxml2/xmlschemas.c
        libxml2/xmlschemastypes.c
        libxml2/xmlunicode.c
        libxml2/xmlreader.c
        libxml2/relaxng.c
        libxml2/dict.c
        libxml2/SAX2.c
        libxml2/xmlwriter.c
        libxml2/legacy.c
        libxml2/chvalid.c
        libxml2/pattern.c
        libxml2/xmlsave.c
        libxml2/xmlmodule.c
        libxml2/schematron.c
        )

include_directories(libxml2/include)

include_directories(libxml2)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
             dwindows

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             dw.cpp dwcompat.c dwib.c dwib_lib.c ${LIBXML2_SOURCES} )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       dwindows

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )

target_compile_options(dwindows PUBLIC -DSTATIC_LIBXML -DLIBXML_THREAD_ENABLED)
#target_compile_options(dwindows PUBLIC -fsanitize=address -fno-omit-frame-pointer)
#set_target_properties(dwindows PROPERTIES LINK_FLAGS -fsanitize=address)
