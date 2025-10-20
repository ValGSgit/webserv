NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g3 -MD -MP
RM = rm -rf

# Directories
SRCDIR = src
OBJDIR = obj
INCDIR = includes

# Source files
SRCFILES = main.cpp \
		   config/ConfigParser.cpp \
		   cgi/CgiHandler.cpp \
		   http/HttpRequest.cpp \
		   http/HttpResponse.cpp \
		   http/HttpTemplates.cpp \
		   http/HttpHandler.cpp \
		   server/ServerManager.cpp \
		   utils/Utils.cpp \

SRCS = $(addprefix $(SRCDIR)/, $(SRCFILES))
OBJS = $(SRCS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

# Include flags
INCLUDES = -I$(INCDIR)

all: $(NAME)

%.d:

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LIBS) -o $(NAME)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	$(RM) $(OBJDIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all

test: $(NAME)
	./$(NAME) config/default.conf

$(OBJDIR)/testmain.o: $(SRCDIR)/testmain.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

#.PHONY: all clean fclean re test test_components test_server

VG_FLAGS = valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind_output.txt
VALGRIND_MASSIF = valgrind --tool=massif --massif-out-file=massif.out

# Memory leak detection
vg_basic: all
	$(VG_FLAGS) ./webserv config/default.conf

# Memory usage profiling
vg_memory: all
	$(VALGRIND_MASSIF) ./webserv config/default.conf

# Stress test with Valgrind
vg_stress: all
	$(VG_FLAGS) ./webserv config/advanced.conf 


# Add to .PHONY
.PHONY: all clean fclean re test test_components vg_basic vg_memory vg_stress
