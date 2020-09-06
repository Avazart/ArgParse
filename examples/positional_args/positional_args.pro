TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp

INCLUDEPATH+= ../ArgParse/

HEADERS += \
  "../../ArgParse/ArgumentParser.h" \
  "../../ArgParse/ArgumentParserDetail.h" \
  "../../ArgParse/TypeUtils.h" \
  "../../ArgParse/StringUtils.h" \
  "../../ArgParse/LatinView.h"
