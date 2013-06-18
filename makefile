CXX=g++
INC=
LIBS=-lcurl
FLG=-std=gnu++0x

SRC=$(shell ls newsparse/*.cpp)
OBJ=$(subst .cpp,.o,$(SRC))

all: $(OBJ)
	$(CXX) $(FLG) -o ndisp $(OBJ) $(INC) $(LIBS)

%.o: %.cpp
	@echo "Compiling $<..."
	@$(CXX) $(FLG) -o $@ -c $< $(INC) $(LIBS)

clean:
	@echo "Cleaning up"
	@rm -f $(OBJ)
	@echo "Done!"
