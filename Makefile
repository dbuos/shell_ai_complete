CXX = g++
CXXFLAGS = -std=c++11 -Wall -O2
TARGET = shell_complete

all: $(TARGET)

$(TARGET): shell_complete.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) shell_complete.cpp

clean:
	rm -f $(TARGET)

install: $(TARGET)
	@echo "To enable shell completion, add this to your ~/.zshrc:"
	@echo "  source $(PWD)/shell_complete.zsh"
	@echo ""
	@echo "Make sure ANTHROPIC_API_KEY is set in your environment"

.PHONY: all clean install
