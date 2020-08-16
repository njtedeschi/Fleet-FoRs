# To use this, you must define FLEET_ROOT, which is the directory above src

# Include directories
FLEET_INCLUDE=-I$(FLEET_ROOT)/src/ -I$(FLEET_ROOT)/src/Inference -I$(FLEET_ROOT)/src/Hypotheses -I$(FLEET_ROOT)/src/VirtualMachine -I$(FLEET_ROOT)/src/Statistics -I$(FLEET_ROOT)/src/Grammar -I$(FLEET_ROOT)/src/Data -I$(FLEET_ROOT)/src/Containers
 
# Some standard/default flags
FLEET_FLAGS=-std=c++2a -Wall -fdiagnostics-color=auto -Wimplicit-fallthrough -Wall -Wextra -Wextra-semi -Wpedantic -Wvla -Wnull-dereference -Wswitch-enum -Wno-unused-parameter -fvar-tracking-assignments -Wduplicated-cond -Wduplicated-branches -Wsuggest-override -march=native -ftemplate-backtrace-limit=0 -fstack-protector-strong --param max-inline-insns-recursive=100000
## Might add -Werror ?

FLEET_LIBS=-lm -pthread

CLANG_FLAGS=-std=c++2a -Wall -fdiagnostics-color=auto -Wimplicit-fallthrough -Wall -Wextra -Wextra-semi -Wpedantic -Wvla -Wnull-dereference -Wswitch-enum -Wno-unused-parameter  -march=native 
