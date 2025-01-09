## How to use?

**Step 1.** Compiling the program.
```cmd 
mkdir build
cd build
cmake ..
make (on windows: cmake --build .)
cd ..
```

**Step 2.** Parsing the program by its name.
In the following command, the parser will parse ./tests/01.c
```cmd
./build/absint tests/easy1.c
```


on windows:


In a nutshell Compile and run:
cd 4-Abstract_Interpreter/build
cmake --build
cd ..
build\Debug\absint.exe tests\easy1.c



Chatgpt think this is better:
cd C:\Users\Tom\Documents\GitHub\lattice-theory-parallel-programming-uni.lu\4-Abstract_Interpreter
mkdir build 2>nul
cd build
cmake .. 
cmake --build . --target clean
cmake --build . 
Debug\absint.exe ..\tests\easy1.c


Rebuild the project: 

cmake --build . --target clean
cmake --build .

Run the program again:

Debug\absint.exe ..\tests\easy4.c
