TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp


INCLUDEPATH+= ../ArgParse/ \
    "D:/Projects/Libs/MSVC/googletest/googletest/include/"

HEADERS += \
  ../ArgParse/Arg.h \
  ../ArgParse/ArgumentParser.h \
  ../ArgParse/ArgumentParserDetail.h \

LIBS += -L"D:/Projects/Libs/MSVC/googletest/build/lib/Release" \
        -lgtest -lgtest_main
