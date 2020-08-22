TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp


INCLUDEPATH+= ../ArgParse/

HEADERS += \
  ../ArgParse/Arg.h \
  ../ArgParse/ArgumentParser.h \
  ../ArgParse/ArgumentParserDetail.h \
  ../ArgParse/StringUtils.h \
  ../ArgParse/TypeUtils.h \
  ../ArgParse/LatinView.h
