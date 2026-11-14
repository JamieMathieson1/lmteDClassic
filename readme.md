# D Classic Compiler.

## Layout
### Generator
Encapsulates Parser, and reads values from there to gnerate the correct lines of code for the C++ compilation.
### Parser
Reads in the file / (maybe add multiple files as an arg input if you can do that with mkfiles.) After this it passes the values to the tokenizer.
### Tokenizer.
The tokenzier uses the values from the Parser to create the correct values in the generator. Stores each unique element type in a container. Each value is linked to a map which points to its location in its container, and has a key of its line number. So iterators, conditionals, functions, variables, and class stuff , are stored seperately.
### testFiles directory
Store the test code files in here to generate D classic code.

### unitTests
Folder for testing individual features in the code base. 

### makefile

should ideally be able to take in arguments about the generation. Also, we should probably add a command change that enables or disables optimisations.
This isn't hard, it also helps us modularise the code without the optimisations as it literally just means having less features used. 
## Intention of use