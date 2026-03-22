CXX = clang++
CC  = clang

CXXFLAGS = -target x86_64-pc-windows-msvc -O2 -Wall -std=c++17
CFLAGS   = -target x86_64-pc-windows-msvc -O2 -std=c11

LDFLAGS  = -target x86_64-pc-windows-msvc -shared

INCLUDES = -I./include -I./include/lua
OBJ_DIR = obj

SRCS_CPP = src/main.cpp src/Utils.cpp LuaUtils.cpp
SRCS_LUA = $(wildcard src/lua/*.c)
SRCS_MH  = $(wildcard src/minhook/*.c) $(wildcard src/minhook/hde/*.c)

OBJS = $(addprefix $(OBJ_DIR)/, $(notdir $(SRCS_CPP:.cpp=.o))) \
	$(addprefix $(OBJ_DIR)/, $(notdir $(SRCS_LUA:.c=.o))) \
	$(addprefix $(OBJ_DIR)/, $(notdir $(SRCS_MH:.c=.o)))

LIBS = -luser32 -lkernel32
TARGET = TavernModLoader.dll

all: $(OBJ_DIR) $(TARGET)

$(OBJ_DIR):
	@if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o: src/lua/%.c
	@echo [C] $<
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/%.o: src/minhook/%.c
	@echo [C] $<
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/%.o: src/minhook/hde/%.c
	@echo [C] $<
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/%.o: src/%.cpp
	@echo [CPP] $<
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(TARGET): $(OBJS)
	@echo [Tavern] Linking...
	@$(CXX) $(LDFLAGS) $(OBJS) -o $(TARGET) $(LIBS)
	@echo [Tavern] Compiled : $(TARGET)

clean:
	@echo [Tavern] Cleaning...
	@if exist $(OBJ_DIR) rmdir /s /q $(OBJ_DIR)
	@if exist $(TARGET) del $(TARGET)

re: clean all

PROXY_STEAM_SRC = proxy/proxy_steam.cpp
PROXY_MS_SRC    = proxy/proxy_ms.cpp
PROXY_STEAM     = xinput1_3.dll
PROXY_MS        = xinput1_4.dll

proxy:
	@echo [Tavern] Compiling Steam proxy...
	@$(CXX) $(CXXFLAGS) $(LDFLAGS) $(PROXY_STEAM_SRC) -o $(PROXY_STEAM) -lkernel32
	@echo [Tavern] Compiled : $(PROXY_STEAM)
	@echo [Tavern] Compiling MS Store proxy...
	@$(CXX) $(CXXFLAGS) $(LDFLAGS) $(PROXY_MS_SRC) -o $(PROXY_MS) -lkernel32
	@echo [Tavern] Compiled : $(PROXY_MS)

.PHONY: all clean re proxy
