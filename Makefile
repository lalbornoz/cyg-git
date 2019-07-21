.DEFAULT_GOAL			:= all
.PHONY:				clean install
CFLAGS				:= -g0 -O2 -pedantic -Wall -Werror -Wextra
PREFIX				:= /usr/local/bin

GIT_FNAME_POSIX			:= /usr/bin/git
CYG_GIT_WRAPPER_FNAME_WINDOWS	:= $(shell cygpath -w "$(PREFIX)/cyg-git-wrapper.exe" | sed 's,\\,&&,g')

all:				cyg-git.exe cyg-git-wrapper.exe
				
clean:				
				rm -f cyg-git.exe cyg-git-wrapper.exe
cyg-git.exe:			cyg-git.c
				x86_64-w64-mingw32-gcc -DCYG_GIT_WRAPPER_FNAME_WINDOWS=\"'$(CYG_GIT_WRAPPER_FNAME_WINDOWS)'\" $(CFLAGS) -o $@ $^
cyg-git-wrapper.exe:		cyg-git-wrapper.c
				gcc -DGIT_FNAME_POSIX=\"'$(GIT_FNAME_POSIX)'\" $(CFLAGS) -o $@ $^
install:			cyg-git.exe cyg-git-wrapper.exe
				install -m 0755 cyg-git.exe "$(PREFIX)/cyg-git.exe"
				install -m 0755 cyg-git-wrapper.exe "$(PREFIX)/cyg-git-wrapper.exe"
