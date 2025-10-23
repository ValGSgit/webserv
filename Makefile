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

# Include flags
INCLUDES = -I$(INCDIR)

all: $(NAME)

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

.PHONY: all clean fclean re test test_config test_config_parser
