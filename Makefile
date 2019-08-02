# Binaries to make
# Name "mugomatic" is tribute to Rogueomatic
bin := mugcli mugomatic mugcollect

# Objects to make
obj := Api.oxx Game.oxx Menu.oxx LearnState.oxx LearnActionFeatures.oxx b64dec.oxx $(patsubst lib/cpr/cpr/%.cpp, %.oxx, $(wildcard lib/cpr/cpr/*.cpp))

libs := -lcurl -licuuc -lpthread -lm

# Add libcpr sources to search path for sources
vpath %.cpp lib/cpr/cpr/

# Optimisation level (usage e.g. make O=2)
O ?= 2

# Compiler flags
CXXFLAGS := \
	-std=c++17 \
	-Ilib/cpr/include \
	-Ilib/rapidjson/include \
	-Ilib/base-n/include \
	-MMD \
	-g -O$(O) \
	-ffunction-sections -fdata-sections -Wl,--gc-sections \
	-pipe

ifeq ($(O),0)
libs += -lasan -lubsan
CXXFLAGS += -fsanitize=address -fsanitize=undefined
endif

ifneq ($(shell which clang++ 2>/dev/null),)
CXX := clang++
CXXFLAGS += -Wall -Wextra -Wno-unused-command-line-argument -Werror
else
CXXFLAGS += -Wall -Wextra -Werror
endif

.PHONY: all
all: $(bin)

.PHONY: clean
clean:
	rm -f -- $(bin) $(obj) $(bin:%=%.oxx) $(obj:%.oxx=%.d)

.PHONY: cli
cli: mugcli
	./mugcli

$(bin): $(obj)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(libs)

$(bin): %: %.oxx

%.oxx: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

-include $(wildcard *.d)
