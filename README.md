# COP 3402 HW4 PL/0 Compiler

Authored By Caleb Rivera and Matthew Labrada

This assignment is an implementation of a recursive descent parser and intermediate code generator for the PL/0 programming language.

## Usage

To run the compiler, use the following commands:

```bash
gcc hw4compiler.c
./a.out <input_file> <output_file>
```

In the above commands, `<input_file>` is the name of the file containing the PL/0 source code, and `<output_file>` is the name of the file to which the compiler will write the output.

## Notes

- If the inputted program is syntactically correct, the compiler will generate an output file containing the source code, the status of the compilation, and the generated intermediate code. It will also create an elf.txt file containing the generated code.
- If the inputted program is syntactically incorrect, the compiler will write the error message to the output file and terminate.

## Example

### Input File

```pl0
var x, y, z;
procedure foo;
begin
    x := 1;
    y := 2;
    z := 3;
end;

begin
    call foo;
end.
```

### Output File

```txt
Source Program:
var x, y, z;
procedure foo;
begin
    x := 1;
    y := 2;
    z := 3;
end;

begin
    call foo;
end.

No errors, program is syntactically correct.

Generated Code:
7 0 30
7 0 6
6 0 4
1 0 1
4 1 3
1 0 2
4 1 4
1 0 3
4 1 5
2 0 0
6 0 4
5 0 3
9 0 3
``````