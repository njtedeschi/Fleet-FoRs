# Define where Fleet lives (directory containing src)
FLEET_ROOT=../../

include $(FLEET_ROOT)/Fleet.mk

# Default Grammar class if not provided
GRAMMAR ?= DEFAULT

# Default CPU name if not provided
CPU ?= default

# Usage
# make training/testing GRAMMAR=grammar_name CPU=cpu_name
# GRAMMAR currently only used for chapter 3 files
# grammar_name corresponds to label in Ch3GrammarSelection.h

all:
	g++-10 Main.cpp -o main -O2 $(FLEET_FLAGS) $(FLEET_INCLUDE) -I  /usr/include/eigen3/ $(FLEET_LIBS)

debug:
	g++-10 Main.cpp -o main -O2 $(FLEET_FLAGS) $(FLEET_INCLUDE) -I /usr/include/eigen3/ $(FLEET_LIBS) -g

training:
	g++-10 Train.cpp -o train_$(CPU) -O2 $(FLEET_FLAGS) $(FLEET_INCLUDE) -I  /usr/include/eigen3/ $(FLEET_LIBS)

testing:
	g++-10 Test.cpp -o test_$(CPU) -O2 $(FLEET_FLAGS) $(FLEET_INCLUDE) -I  /usr/include/eigen3/ $(FLEET_LIBS)

deserialize:
	g++-10 DeserializeModels.cpp -o deserialize -O2 $(FLEET_FLAGS) $(FLEET_INCLUDE) -I  /usr/include/eigen3/ $(FLEET_LIBS)

training3:
	g++-10 Ch3Train.cpp -o train_3_$(CPU) -O2 $(FLEET_FLAGS) $(FLEET_INCLUDE) -I  /usr/include/eigen3/ $(FLEET_LIBS) -DSELECTED_GRAMMAR=$(GRAMMAR)

testing3:
	g++-10 Ch3TestDistributions.cpp -o test_3_$(CPU) -O2 $(FLEET_FLAGS) $(FLEET_INCLUDE) -I  /usr/include/eigen3/ $(FLEET_LIBS) -DSELECTED_GRAMMAR=$(GRAMMAR)

deserialize3:
	g++-10 Ch3DeserializeModels.cpp -o deserialize_3 -O2 $(FLEET_FLAGS) $(FLEET_INCLUDE) -I  /usr/include/eigen3/ $(FLEET_LIBS) -DSELECTED_GRAMMAR=$(GRAMMAR)