.SILENT:

JAVA       ?= java
# Use the compiler from the same JDK as the Java launcher.
JAVAC      ?= $(JAVA) -m jdk.compiler/com.sun.tools.javac.Main
JAVACFLAGS ?= -encoding UTF-8
JAVAFLAGS  ?= --enable-native-access=ALL-UNNAMED
JLINE       = jline-3.26.3.jar
SOURCES     = $(wildcard *.java)

ifeq ($(OS),Windows_NT)
    CLASSPATH = .;$(JLINE)
    CLEAN = del /f /q *.class
else
    CLASSPATH = .:$(JLINE)
    CLEAN = rm -f *.class
endif

all: compile

compile:
	$(JAVAC) $(JAVACFLAGS) -cp "$(JLINE)" $(SOURCES)

run: compile
	$(JAVA) $(JAVAFLAGS) -cp "$(CLASSPATH)" KeySprint $(ARGS)

clean:
	$(CLEAN)

.PHONY: all compile run clean
