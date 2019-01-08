# lang.pybind11

** python3 pybind11 maiken module **

Link phase module

## Prerequisites
  [maiken](https://github.com/Dekken/maiken)

## Usage

```yaml
mod:
- name: lang.pybind11

```

## Building

  Windows cl:

    mkn clean build -tSa -EHsc -d


  *nix gcc:

    mkn clean build -tSa "-O2 -fPIC" -d -l "-pthread -ldl"


## Testing

  Windows cl:

    mkn clean build -tSa -EHsc -dp test run

  *nix gcc:

    mkn clean build -tSa "-O2 -fPIC" -dp test -l "-pthread -ldl" run


## Environment Variables

    Key             PYTHON3_HOME
    Type            string
    Default         ""
    Description     If set - looks for python3/config in $PYTHON3_HOME/bin
