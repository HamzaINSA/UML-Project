# Makefile AirWatcher
CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wpedantic
LDFLAGS  :=

SRC_DIR  := src
BUILD    := build
BIN      := airwatcher
TEST_BIN := test_airwatcher

# Sources sans le main (pour pouvoir partager avec les tests)
LIB_SRCS := \
    $(SRC_DIR)/model/DateTime.cpp \
    $(SRC_DIR)/model/AttributMesure.cpp \
    $(SRC_DIR)/model/Mesure.cpp \
    $(SRC_DIR)/model/Capteur.cpp \
    $(SRC_DIR)/model/Purificateur.cpp \
    $(SRC_DIR)/model/Utilisateur.cpp \
    $(SRC_DIR)/model/Agence.cpp \
    $(SRC_DIR)/model/Particulier.cpp \
    $(SRC_DIR)/model/Fournisseur.cpp \
    $(SRC_DIR)/model/ImpactPurificateur.cpp \
    $(SRC_DIR)/model/Rapport.cpp \
    $(SRC_DIR)/repository/DataReader.cpp \
    $(SRC_DIR)/service/PerformanceMonitor.cpp \
    $(SRC_DIR)/service/AirQualityService.cpp \
    $(SRC_DIR)/service/AdministrationService.cpp \
    $(SRC_DIR)/service/EnvironmentalService.cpp \
    $(SRC_DIR)/ui/ConsoleUI.cpp

LIB_OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD)/%.o, $(LIB_SRCS))
MAIN_OBJ := $(BUILD)/main.o
TEST_OBJ := $(BUILD)/tests/test_main.o

.PHONY: all clean run test

all: $(BIN)

$(BIN): $(LIB_OBJS) $(MAIN_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/tests/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(LIB_OBJS) $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

run: $(BIN)
	./$(BIN) data

clean:
	rm -rf $(BUILD) $(BIN) $(TEST_BIN)
