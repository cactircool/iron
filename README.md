Assuming the Ninja build tool is installed along with cmake and llvm, run this command to build the project and generate the binary
Insert any build targets into the -DLLVM_TARGETS_TO_BUILD macro separated by semicolons:
```
    cmake . -B build -G Ninja -DLLVM_TARGETS_TO_BUILD=arm64-apple-darwin23.0.0 && cd build && ninja && cp iron ../iron
```

Now the iron binary file will be in the current directory for you to use

```
    ./iron [FILENAME]
```