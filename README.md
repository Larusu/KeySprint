# KeySprint

## Running Python Version 

in-progress

## Running Java Version 

in-progress 

## Running C++ version

From the project root:

```bash
make -C cpp keysprint
./cpp/keysprint
```

To use a specific word bank:

```bash
./cpp/keysprint data/words.json
```

See the [C++ usage guide](docs/cpp-word-bank-guide.md) for controls and current
integration limits. Shared vocabulary attribution is in [data/SOURCES.md](data/SOURCES.md).

### Clean build files

```bash
make -C cpp clean
```
