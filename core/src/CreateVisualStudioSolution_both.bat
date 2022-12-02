@echo ** Creates MSVC projects from CMakelists.txt regardless file up-to-date... **
del cmakecache.txt
call CreateVisualStudioSolution_x86
del cmakecache.txt
call CreateVisualStudioSolution