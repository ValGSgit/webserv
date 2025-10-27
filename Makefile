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
		   utils/Utils.cpp

SRCS = $(addprefix $(SRCDIR)/, $(SRCFILES))
OBJS = $(SRCS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

VALGRIND_FLAGS = --leak-check=full \
                 --show-leak-kinds=all \
                 --track-origins=yes \
                 --track-fds=yes \
                 --trace-children=yes \
                 --show-reachable=yes \
                 --error-limit=no \
                 --keep-debuginfo=yes \
                 --read-var-info=yes \
                 --verbose

# Extra flags for extreme checking
VALGRIND_EXTREME_FLAGS = $(VALGRIND_FLAGS) \
                         --expensive-definedness-checks=yes \
                         --read-inline-info=yes \
                         --malloc-fill=0x42 \
                         --free-fill=0x69

# Include flags
INCLUDES = -I$(INCDIR)

all: $(NAME)

vg: $(NAME)
	valgrind $(VALGRIND_FLAGS) ./$(NAME) webserv.conf > valgrind_log.txt

vg_extreme: $(NAME)
	valgrind $(VALGRIND_EXTREME_FLAGS) ./$(NAME) webserv.conf > valgrind_extreme_log.txt

%.d:

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

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

test_config: test_config_parser
	./test_config_parser config/default.conf

test_config_parser: tests/scripts/test_config_parser.cpp $(filter-out $(OBJDIR)/main.o, $(OBJS))
	$(CXX) $(CXXFLAGS) $(INCLUDES) tests/scripts/test_config_parser.cpp $(filter-out $(OBJDIR)/main.o, $(OBJS)) -o test_config_parser

$(OBJDIR)/testmain.o: $(SRCDIR)/testmain.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

#
.PHONY: all clean fclean re test test_config test_config_parser

