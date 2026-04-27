CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread

TARGET = dbms

SRCS = main.cpp table.cpp database.cpp parser.cpp executor.cpp transaction.cpp lock_manager.cpp
OBJS = $(SRCS:.cpp=.o)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
