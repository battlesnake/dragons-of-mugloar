# Binaries to make
# Name "mugomatic" is tribute to Rogueomatic
bin := mugcli mugomatic

# Objects to make
obj := Api.oxx Game.oxx Menu.oxx b64dec.oxx $(patsubst lib/cpr/cpr/%.cpp, %.oxx, $(wildcard lib/cpr/cpr/*.cpp))

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
	-Wall -Wextra -Wno-unused-command-line-argument -Werror \
	-fsanitize=address \
	-fsanitize=undefined \
	-ffunction-sections -fdata-sections -Wl,--gc-sections \
	-pipe

.PHONY: all
all: $(bin)

.PHONY: clean
clean:
	rm -f -- $(bin) $(obj) $(obj:%.oxx=%.d)

.PHONY: cli
cli: mugcli
	./mugcli

$(bin): $(obj)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lcurl -lpthread -lm -lasan -lubsan

$(bin): %: %.oxx

%.oxx: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

-include $(wildcard *.d)
