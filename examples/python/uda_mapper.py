import argparse
from pathlib import Path
from typing import Any, override
import numpy as np
import json
import libtokamap


def map(mapper: libtokamap.Mapper, mapping: str, signal: str, shot: int):
    res = mapper.map(mapping, signal, {'shot': shot})
    if res.dtype == 'S1':
        res = res.tobytes().decode()
    print(f"{signal}: {res}")
    return res


def map_all(mapper: libtokamap.Mapper, mapping: str, key: str, shot: int):
    map(mapper, mapping, key, shot)


def main():

    parser = argparse.ArgumentParser()
    parser.add_argument("--shot", 
                        type=int, 
                        default=47125, 
                        help="Experimental shot number")

    parser.add_argument("--uda-host", 
                        type=str, 
                        default="uda2.mast.l", 
                        help="UDA server IP or DNS")
    parser.add_argument("--uda-port", 
                        type=int, 
                        default=59876, 
                        help="UDA server port number")
    parser.add_argument("--uda-plugin-name", 
                        type=str, 
                        default="UDA", 
                        help="Data source plugin name")

    parser.add_argument("--mapping", 
                        type=str, 
                        default="magnetics/ip/data", 
                        help="Mapping key string")
    parser.add_argument("--device", 
                        type=str, 
                        default="mastu", 
                        help="Device name (mapping folder name)")
    parser.add_argument("--mapping-directory-path", 
                        type=str, 
                        default="mastu_mappings", 
                        help="Path to the json mappings directory")
    parser.add_argument("--data-source-lib-path", 
                        type=str, 
                        default="libuda_data_source.so", 
                        help="Path to the data source shared library")
    parser.add_argument("--factory-name", 
                        type=str, 
                        default="uda_factory", 
                        help="Data source factory name")

    args = parser.parse_args()

    print("Calling LibTokaMap version:", libtokamap.__version__)
    print("Mapping options:")
    print(args)

    mapper = libtokamap.Mapper(args.mapping_directory_path)
    mapper.register_data_source_factory(args.factory_name, args.data_source_lib_path)

    plugin_args = {
            "host": args.uda_host,
            "port": args.uda_port,
            "plugin_name": args.uda_plugin_name
            }
    mapper.register_data_source(args.uda_plugin_name, args.factory_name, plugin_args)
    mapping = args.device

    try:
        map_all(mapper, mapping, args.mapping, args.shot)
    except Exception as e:
        print(f"{e}")


if __name__ == "__main__":
    import sys
    # import timeit
    # print(timeit.timeit("main()", number=10, setup="from __main__ import main"))
    main()
