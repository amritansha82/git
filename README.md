# git

_A simple git client written in C++._

The following commands are supported:
- `init`: Initialize a new git repository
- `hash-object`: Create a git blob object
- `cat-file`: Print the contents of a blob object
- `write-tree`: Create a git tree object
- `ls-tree`: List the contents of a tree object
- `commit-tree`: Create a git commit object
- `clone`: Clone a remote repository

This implementation does not support branching (each commit can only point to a single parent commit), and does not have a dedicated staging area (`write-tree` creates a tree object from the current state of the working directory).

- Git Reference: https://git-scm.com/docs

## Building from source

Prerequisites:
- CMake ≥ 3.13
- C++ compiler with C++23 support (e.g. g++ 11+, clang 14+)

### Dependencies

#### Ubuntu / Debian

```bash
sudo apt update
sudo apt install libcurl4-openssl-dev libssl-dev zlib1g-dev
```

#### Fedora
```bash
sudo dnf install libcurl-devel openssl-devel zlib-devel
```

#### Arch
```bash
sudo pacman -S curl openssl zlib
```

### Building

```bash
cmake -B build -S .
cmake --build ./build
```

### Running

- Clone a remote repository
   
   ```bash
   ./build/git clone https://github.com/expressjs/express express
   ```

- Write a blob object

   ```bash
   ./build/git hash-object -w <file>
   ```

- View a blob object

   ```bash
   ./build/git cat-file -p <blob-sha>
   ```

- Write the current file tree to a tree object

   ```bash
   ./build/git write-tree
   ```

- List the contents of a tree object

   ```bash
   ./build/git ls-tree [--name-only|--object-only] <tree-sha>
   ```

- Create a commit object

   ```bash
   ./build/git commit-tree <tree-sha> -p <parent-commit-sha> -m <message>
   ```

## License

[MIT](LICENSE)