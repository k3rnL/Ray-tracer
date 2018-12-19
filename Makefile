# @Author: daniel_b
# @Date:   2017-08-03T16:45:05+02:00
# @Last modified by:
# @Last modified time: 2017-11-24T00:18:40+01:00



SRCS 	= 	main.cpp \
					Map.cpp

OBJS	=		$(SRCS:.cpp=.o)

LDFLAGS	=	-lepoxy -lfse -lSDL2 -lGL -lGLU

CPPFLAGS	=	-std=c++11 -g

NAME 	=	MyGame

all: $(NAME)

$(NAME): $(OBJS)
	g++ -o $(NAME) $(OBJS) $(LDFLAGS)

clean:
	rm -rf $(OBJS)

fclean: clean
	rm -rf $(NAME)

re : fclean all
