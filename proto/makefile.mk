BUILD_TARGET = ./build
LINK_TARGET = $(BUILD_TARGET)/proto.so

$(BUILD_TARGET) : 
	mkdir $(BUILD_TARGET)

clean : 
  rm -f $(BUILD_TARGET)/*.o
  rm -f $(BUILD_TARGET)/*.so
  rm -f $(BUILD_TARGET)/*.dep
  echo Clean done

# The rule for "all" is used to incrementally build your system.
# It does this by expressing a dependency on the results of that system,
# which in turn have their own rules and dependencies.
all : $(LINK_TARGET)
  echo All done

# Compile all .cpp files
$(LINK_TARGET) : $(BUILD_TARGET)/*.o
  g++ -g --shared -o $@ $^

# Here is a Pattern Rule, often used for compile-line.
# It says how to create a file with a .o suffix, given a file with a .cpp suffix.
# The rule's command uses some built-in Make Macros:
# $@ for the pattern-matched target
# $< for the pattern-matched dependency
$(BUILD_TARGET)/%.o : core/%.cpp
  g++ -g -o $@ -c $<

# These are Dependency Rules, which are rules without any command.
# Dependency Rules indicate that if any file to the right of the colon changes,
# the target to the left of the colon should be considered out-of-date.
# The commands for making an out-of-date target up-to-date may be found elsewhere
# (in this case, by the Pattern Rule above).
# Dependency Rules are often used to capture header file dependencies.
$(BUILD_TARGET)/%.dep : core/%.cpp
	g++ -M $(FLAGS) $< > $@
include $(OBJS:.o=.dep)

install: $(LINK_TARGET) headers/*.h
	cp headers/*h /usr/include
	cp $(LINK_TARGET) /usr/lib

test: $(LINK_TARGET) headers/*.h
	g++ -c -Iheaders test/test_proto.cpp
	g++ -g -o test/test_proto test/test_proto.o $(BUILD_TARGET)/*.o
	test/test_proto



