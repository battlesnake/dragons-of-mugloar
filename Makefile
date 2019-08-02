# Binary to make
# Name is tribute to Rogue'O'Matic
bin := mugomatic

# Objects to make
obj := Api.oxx Game.oxx parse_number.oxx $(patsubst lib/cpr/cpr/%.cpp, %.oxx, $(wildcard lib/cpr/cpr/*.cpp))

# Add libcpr sources to search path for sources
vpath %.cpp lib/cpr/cpr/

# Use clang if available
ifneq ($(shell which clang++ 2>/dev/null),)
CXX := clang++
endif

# Optimisation level (usage e.g. make O=2)
O ?= 0

# Compiler flags
CXXFLAGS := \
	-std=c++17 \
	-Ilib/cpr/include \
	-Ilib/rapidjson/include \
	-MMD \
	-g -O$(O) \
	-Wall -Wextra -Wno-unused-command-line-argument -Werror \
	-ffunction-sections -fdata-sections -Wl,--gc-sections \
	-pipe

.PHONY: all
all: $(bin)

.PHONY: clean
clean:
	rm -f -- $(bin) $(obj) $(obj:%.oxx=%.d)

$(bin): $(obj)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lpthread -lm

$(obj): %.oxx: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

-include $(wildcard *.d)
