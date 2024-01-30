# Define where Fleet lives (directory containing src)
FLEET_ROOT=../../

include $(FLEET_ROOT)/Fleet.mk


all:
	g++-10 Main.cpp -o main -O2 $(FLEET_FLAGS) $(FLEET_INCLUDE) -I  /usr/include/eigen3/ $(FLEET_LIBS)

debug:
	g++-10 Main.cpp -o main -O2 $(FLEET_FLAGS) $(FLEET_INCLUDE) -I /usr/include/eigen3/ $(FLEET_LIBS) -g

training:
	g++-10 Train.cpp -o train -O2 $(FLEET_FLAGS) $(FLEET_INCLUDE) -I  /usr/include/eigen3/ $(FLEET_LIBS)
