run:
	g++ src/*.cpp src/glad/*.c -o bin/main.exe -Llib -Iinclude -lglfw3 -lopengl32 -lgdi32 -lwinmm -lopenvr_api -lpthread -static-libstdc++ -static-libgcc -static
	bin/main

executable:
	g++ src/*.cpp src/glad/*.c -o bin/main.exe -Llib -Iinclude -lglfw3 -lopengl32 -lgdi32 -lwinmm -lopenvr_api -lpthread -static-libstdc++ -static-libgcc -static

static:
	g++ src/rvr.cpp -c -o bin/rvr.o -Iinclude
	ar rvs bin/librvr.a bin/rvr.o

shared:
	g++ src/rvr.cpp src/glad/*.c -shared -o bin/rvr.dll -Iinclude -Llib -lglfw3 -lopengl32 -lgdi32 -lwinmm -lopenvr_api -lpthread -static-libstdc++ -static-libgcc -static -DRVR_EXTERN