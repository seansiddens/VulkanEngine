CFLAGS = -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

.PHONY: all clean test format

all: VulkanEngine

VulkanEngine: *.cpp *.hpp
	g++ $(CFLAGS) -o VulkanEngine *.cpp $(LDFLAGS)

clean:
	rm -f VulkanEngine

test: all
	./VulkanEngine

format: 
	clang-format -i *.cpp *.hpp --style="{BasedOnStyle: Google, IndentWidth: 4, ColumnLimit: 100}"
