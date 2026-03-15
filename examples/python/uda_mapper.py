# import argparse
from pathlib import Path
from typing import Any, override
import numpy as np
import json
import libtokamap


def map(mapper: libtokamap.Mapper, mapping: str, signal: str):
    res = mapper.map(mapping, signal, {'shot': 45272})
    if res.dtype == 'S1':
        res = res.tobytes().decode()
    print(f"{signal}: {res}")
    return res


def map_all(mapper: libtokamap.Mapper, mapping: str):
    map(mapper, mapping, "magnetics/ip/data")


def main(args):
    cxxlibs = False
    if len(args) == 2:
        if args[1] == "--help":
            print(f"Usage: python {args[0]}")
            sys.exit(0)
        else:
            print(f"Usage: python {args[0]}")
            sys.exit(1)

    print("Calling LibTokaMap version:", libtokamap.__version__)

    mapping_directory = Path("/Users/sdixon/uda/iter-mapping-workshop/libtokamap/python/mastu_mappings")
    mapper = libtokamap.Mapper(str(mapping_directory))

    build_root = Path("/Users/sdixon/uda/iter-mapping-workshop/libtokamap/python") #root / "build" / "examples" / "simple_mapper"
    factory_library = build_root / ("libuda_data_source" + libtokamap.LibrarySuffix)
    mapper.register_data_source_factory("uda_factory", str(factory_library))
    mapper.register_data_source("UDA", "uda_factory", {"host": "uda2.mast.l", "port": 59876, "plugin_name": "UDA"})

    mapping = "mastu"
    try:
        map_all(mapper, mapping)
    except Exception as e:
        print(f"{e}")


if __name__ == "__main__":
    import sys
    # import timeit
    # print(timeit.timeit("main(sys.argv)", number=10, setup="from __main__ import main"))
    main(sys.argv)
