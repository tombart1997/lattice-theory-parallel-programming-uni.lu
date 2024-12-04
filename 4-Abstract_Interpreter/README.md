## How to use?

**Step 1.** Compiling the program.
```cmd 
mkdir build
cd build
cmake ..
make 
cd ..
```

**Step 2.** Parsing the program by its name.
In the following command, the parser will parse ./tests/01.c
```cmd
./build/example_parser 1
```
```c
// ./tests/01.c
int a, b, c, d, e;

void main() {

  /*!npk a between 10 and 100*/
  /*!npk b between 0 and 47*/

  c = a + b;
  d = a * c;
  e = d - c;

}
```

**Step 3.** Understanding the result
```bash
Program: 01.c
Parsing succeeded!
NodeType: Integer, Value: 0

  NodeType: Declaration, Value: int

    NodeType: Variable, Value: a

    NodeType: Variable, Value: b

    NodeType: Variable, Value: c

    NodeType: Variable, Value: d

    NodeType: Variable, Value: e

  NodeType: BlockBody, Value: BlockBody

    NodeType: Pre-condition, Value: PreCon

      NodeType: Logic Operator, Value: <=

        NodeType: Integer, Value: 10

        NodeType: Variable, Value: a

      NodeType: Logic Operator, Value: >=

        NodeType: Integer, Value: 100

        NodeType: Variable, Value: a

    NodeType: Pre-condition, Value: PreCon

      NodeType: Logic Operator, Value: <=

        NodeType: Integer, Value: 0

        NodeType: Variable, Value: b

      NodeType: Logic Operator, Value: >=

        NodeType: Integer, Value: 47

        NodeType: Variable, Value: b

    NodeType: Assignment, Value: =

      NodeType: Variable, Value: c

      NodeType: Arithmetic Operator, Value: +

        NodeType: Variable, Value: a

        NodeType: Variable, Value: b

    NodeType: Assignment, Value: =

      NodeType: Variable, Value: d

      NodeType: Arithmetic Operator, Value: *

        NodeType: Variable, Value: a

        NodeType: Variable, Value: c

    NodeType: Assignment, Value: =

      NodeType: Variable, Value: e

      NodeType: Arithmetic Operator, Value: -

        NodeType: Variable, Value: d

        NodeType: Variable, Value: c
```