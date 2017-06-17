CXXFLAGS         = -std=c++11 -Wall -Wextra -Wpedantic -I./src -I/usr/include -Wno-gnu-anonymous-struct -g -O0
PP_FLAGS         = -D LT_DEBUG -D VX_DEV
LDLIBS           = -L/usr/lib -lm -lglfw -lGL -lpthread -lX11 -lXi -lXrandr -ldl

CXX              = clang++
SRC              = $(shell find src -mindepth 1 -name "*.cpp")
OBJ              = ${SRC:src/%.cpp=build/objects/%.o}
DEP              = $(OBJ:%.o=%.d)

BIN              = tr
BUILD_DIR        = ./build

all: $(BIN)

$(BIN): $(BUILD_DIR)/$(BIN)

$(BUILD_DIR)/$(BIN): $(OBJ)
	mkdir -p $(@D)
	@echo CC $^ -o $@
	@$(CXX) $(CXXFLAGS) $^ $(LDLIBS)  -o $@

# include all .d files.
-include $(DEP)

$(BUILD_DIR)/objects/%.o: src/%.cpp
	mkdir -p $(@D)
	@echo CC -MMD -c $< -o $@
	@$(CXX) $(PP_FLAGS) $(CXXFLAGS) -MMD -c $< -o $@

.PHONY: clean
clean:
	rm -rf build/objects
	rm build/$(BIN)

run: $(BIN)
	cd $(BUILD_DIR) && ./$(BIN)

cc_args: CXX = cc_args.py g++
cc_args: all
