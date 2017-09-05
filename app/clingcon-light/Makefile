undefine CXX
-include FLAGS

CLINGOROOT?=/home/wv/opt/clingo-banane

CXX?=clang++-3.8
#CXX?=clang++
CXXFLAGS?=-std=c++11 -W -Wall -O3 -DNDEBUG
CPPFLAGS?=-I$(CLINGOROOT)/include
LDFLAGS?=-L$(CLINGOROOT)/lib -Wl,-rpath=$(CLINGOROOT)/lib
LDLIBS?=-lclingo

TARGET=clingoDL
SOURCE=main.cpp src/constraint.cpp

OBJECT=$(patsubst %,%.o,$(basename $(SOURCE)))
DEPEND=$(patsubst %,%.d,$(basename $(SOURCE)))

all: $(TARGET)

FLAGS:
	rm -f FLAGS
	echo "CXX:=$(CXX)" >> FLAGS
	echo "CXXFLAGS:=$(CXXFLAGS)" >> FLAGS
	echo "CPPFLAGS:=$(CPPFLAGS)" >> FLAGS
	echo "LDFLAGS:=$(LDFLAGS)" >> FLAGS
	echo "LDLIBS:=$(LDLIBS)" >> FLAGS

$(TARGET): $(OBJECT) FLAGS
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECT) $(LDFLAGS) $(LDLIBS)

.DELETE_ON_ERROR: %.Td
%.o: %.cpp
%.o %.Td: %.cpp %.d FLAGS
	$(CXX) -c -MT $@ -MMD -MP -MF $*.Td $(CXXFLAGS) $(CPPFLAGS) $<
	mv -f $*.Td $*.d

.PHONY: format
format:
	clang-format -style="{BasedOnStyle: llvm, IndentWidth: 4, SortIncludes: false, ColumnLimit: 256, AccessModifierOffset: -4, BreakBeforeBraces: Custom, BraceWrapping: {BeforeElse: true, BeforeCatch: true}, BreakConstructorInitializersBeforeComma: true, AlwaysBreakTemplateDeclarations: true, AlignAfterOpenBracket: AlwaysBreak, AllowShortBlocksOnASingleLine: true, IndentCaseLabels: true}" -i $(SOURCE)

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJECT) $(DEPEND) $(patsubst %,%.Td,$(basename $(SOURCE)))

.PRECIOUS: %.d
%.d: ;

include $(wildcard $(DEPEND))
