# Define where Fleet lives (directory containing src)
FLEET_ROOT=../../

include $(FLEET_ROOT)/Fleet.mk

# Default CPU name if not provided
CPU ?= default
# Usage
# make training/testing CPU=cpu_name

all:
	g++-10 Main.cpp -o main -O2 $(FLEET_FLAGS) $(FLEET_INCLUDE) -I  /usr/include/eigen3/ $(FLEET_LIBS)

debug:
	g++-10 Main.cpp -o main -O2 $(FLEET_FLAGS) $(FLEET_INCLUDE) -I /usr/include/eigen3/ $(FLEET_LIBS) -g

training:
	g++-10 Train.cpp -o train_$(CPU) -O2 $(FLEET_FLAGS) $(FLEET_INCLUDE) -I  /usr/include/eigen3/ $(FLEET_LIBS)

testing:
	g++-10 Test.cpp -o test_$(CPU) -O2 $(FLEET_FLAGS) $(FLEET_INCLUDE) -I  /usr/include/eigen3/ $(FLEET_LIBS)