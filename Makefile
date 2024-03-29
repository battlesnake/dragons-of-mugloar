# Binaries to make
# Name "mugomatic" is tribute to Rogueomatic
bin := mugcli mugcollect muglearn mugomatic mugobasic

# Objects to make
obj := \
	Api.oxx Game.oxx \
	Menu.oxx \
	Locale.oxx \
	ExtractFeatures.oxx \
	LogEvent.oxx \
	LowerCase.oxx \
	Parallel.oxx \
	BasicAssist.oxx \
	Base64dec.oxx Rot13dec.oxx \
	$(patsubst lib/cpr/cpr/%.cpp, %.oxx, $(wildcard lib/cpr/cpr/*.cpp))

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
	-pipe \
	-Wall -Wno-logical-op-parentheses -Wno-parentheses -Wextra -Werror

# Use address- and undefined-behaviour- sanitizers when debugging
ifeq ($(O),0)
libs += -lasan -lubsan
CXXFLAGS += -fsanitize=address -fsanitize=undefined
endif

# Prefer clang where available
ifneq ($(shell which clang++ 2>/dev/null),)
CXX := clang++
endif

.PHONY: all
all: $(bin)

.PHONY: clean
clean:
	rm -f -- $(bin) *.d *.oxx

.PHONY: cli
cli: mugcli
	./mugcli

$(bin): $(obj)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(libs)

$(bin): %: %.oxx

%.oxx: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

-include $(wildcard *.d)
