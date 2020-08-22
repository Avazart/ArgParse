TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp

HEADERS += \
  "../../ArgParse/Arg.h" \
  "../../ArgParse/ArgumentParser.h" \
  "../../ArgParse/ArgumentParserDetail.h" \
  "../../ArgParse/TypeUtils.h" \
  "../../ArgParse/StringUtils.h" \
  "../../ArgParse/LatinView.h"

INCLUDEPATH+= ../ArgParse/ \
    "D:/Projects/Libs/MSVC/googletest/googletest/include/"

LIBS += -L"D:/Projects/Libs/MSVC/googletest/build/lib/Release" \
        -lgtest -lgtest_main
