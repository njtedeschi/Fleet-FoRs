# Define where Fleet lives (directory containing src)
FLEET_ROOT=../../../

include $(FLEET_ROOT)/Fleet.mk


all:
	g++-10 MyDataTest.cpp -o my_test -O2 $(FLEET_FLAGS)  $(FLEET_INCLUDE) -I /usr/include/eigen3/ -I /usr/local/include/ $(FLEET_LIBS) -L/usr/local/lib/

debug:
	g++-10 MyDataTest.cpp -o my_test -g $(FLEET_FLAGS) $(FLEET_INCLUDE) -I /usr/include/eigen3/ -I /usr/local/include/ $(FLEET_LIBS) -L/usr/local/lib
