TARGETS=slowcont sync tilt

all: $(TARGETS)

%: %/midas.xml %/romeConfig.xml
	cd $@
	make clean
	romebuilder.exe midas.xml

